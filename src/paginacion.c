#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "paginacion.h"
#include "frame_allocator.h"

#define MIN_DISK_DELAY_MS 1
#define MAX_DISK_DELAY_MS 5

extern pthread_mutex_t g_page_tables_mutex;
extern page_table_t *g_page_tables[];

page_table_t* paginacion_init(int num_pages) {
    page_table_t *table = (page_table_t*)malloc(sizeof(page_table_t));
    if (!table) return NULL;

    table->num_pages = num_pages;
    table->entries = (page_table_entry_t*)malloc(sizeof(page_table_entry_t) * num_pages);
    if (!table->entries) {
        free(table);
        return NULL;
    }

    for (int i = 0; i < num_pages; i++) {
        table->entries[i].valid = 0;
        table->entries[i].frame_number = -1;
    }
    return table;
}

void paginacion_destroy(page_table_t *table) {
    if (table) {
        free(table->entries);
        free(table);
    }
}

void paginacion_invalidate_entry(page_table_t *table, int vpn) {
    if (table && vpn >= 0 && vpn < table->num_pages) {
        table->entries[vpn].valid = 0;
        table->entries[vpn].frame_number = -1;
    }
}

void invalidate_tlb_entry_global(int vpn) {
    (void)vpn;
}

int paginacion_translate(page_table_t *table, int vpn, uint64_t *pa_base, config_t *config, int thread_id) {
    if (vpn < 0 || vpn >= table->num_pages) return 0;

    if (table->entries[vpn].valid) {
        *pa_base = (uint64_t)table->entries[vpn].frame_number * config->page_size;
        return 1;
    }

    // Page Fault
    struct timespec ts = {0, (rand() % (MAX_DISK_DELAY_MS - MIN_DISK_DELAY_MS + 1) + MIN_DISK_DELAY_MS) * 1000000};
    nanosleep(&ts, NULL);

    int victim_vpn = -1, victim_thread_id = -1;
    int allocated_frame = allocate_frame(&victim_vpn, &victim_thread_id);

    if (allocated_frame == -1) {
        fprintf(stderr, "Error: Fallo al asignar frame.\n");
        return 0;
    }

    if (victim_vpn != -1) {
        if (!config->unsafe) pthread_mutex_lock(&g_page_tables_mutex);
        if (victim_thread_id != -1 && g_page_tables[victim_thread_id] != NULL) {
            paginacion_invalidate_entry(g_page_tables[victim_thread_id], victim_vpn);
        }
        if (!config->unsafe) pthread_mutex_unlock(&g_page_tables_mutex);
        invalidate_tlb_entry_global(victim_vpn);
    }

    table->entries[vpn].frame_number = allocated_frame;
    table->entries[vpn].valid = 1;

    frame_allocator_map_frame(allocated_frame, vpn, thread_id);

    *pa_base = (uint64_t)allocated_frame * config->page_size;
    return 1;
}
