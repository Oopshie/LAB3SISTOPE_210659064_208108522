#include <stdlib.h>
#include <stdio.h>
#include "workloads.h"

static next_address_func_t next_address_generator;

// --- Implementaciones de Workloads para Segmentación ---

static void workload_uniform_seg(config_t *config, uint64_t *seg_id, uint64_t *offset) {
    *seg_id = rand() % config->num_segments;
    // Genera un offset en un rango amplio para simular accesos válidos e inválidos.
    *offset = rand() % (config->page_size * 2);
}

static void workload_80_20_seg(config_t *config, uint64_t *seg_id, uint64_t *offset) {
    int twenty_percent_segments = (int)(config->num_segments * 0.2);
    if (twenty_percent_segments == 0) twenty_percent_segments = 1;

    // 80% de los accesos al primer 20% de los segmentos
    if ((rand() % 100) < 80) {
        *seg_id = rand() % twenty_percent_segments;
    } else {
    // 20% de los accesos al 80% restante de los segmentos
        int remaining_segments = config->num_segments - twenty_percent_segments;
        if (remaining_segments > 0) {
            *seg_id = twenty_percent_segments + (rand() % remaining_segments);
        } else {
            *seg_id = rand() % config->num_segments;
        }
    }
    *offset = rand() % (config->page_size * 2);
}

// --- Implementaciones de Workloads para Paginación ---

static void workload_uniform_page(config_t *config, uint64_t *virtual_address, uint64_t *unused) {
    (void)unused; // Evitar warning de no usado
    uint64_t max_addr = (uint64_t)config->num_pages * config->page_size;
    *virtual_address = rand() % max_addr;
}

static void workload_80_20_page(config_t *config, uint64_t *virtual_address, uint64_t *unused) {
    (void)unused; // Evitar warning de no usado
    int twenty_percent_pages = (int)(config->num_pages * 0.2);
    if (twenty_percent_pages == 0) twenty_percent_pages = 1;

    uint64_t vpn;
    // 80% de los accesos al primer 20% de las páginas
    if ((rand() % 100) < 80) {
        vpn = rand() % twenty_percent_pages;
    } else {
    // 20% de los accesos al 80% restante de las páginas
        int remaining_pages = config->num_pages - twenty_percent_pages;
        if (remaining_pages > 0) {
            vpn = twenty_percent_pages + (rand() % remaining_pages);
        } else {
            vpn = rand() % config->num_pages;
        }
    }

    uint64_t offset = rand() % config->page_size;
    *virtual_address = (vpn * config->page_size) + offset;
}


// --- Funciones Públicas ---

void init_workloads(config_t *config) {
    if (config->mode == MODE_SEGMENTATION) {
        if (config->workload == WORKLOAD_80_20) {
            next_address_generator = workload_80_20_seg;
        } else {
            next_address_generator = workload_uniform_seg;
        }
    } else { // MODE_PAGINATION
        if (config->workload == WORKLOAD_80_20) {
            next_address_generator = workload_80_20_page;
        } else {
            next_address_generator = workload_uniform_page;
        }
    }
}

void generate_address(config_t *config, uint64_t *addr1, uint64_t *addr2) {
    next_address_generator(config, addr1, addr2);
}
