#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "segmentacion.h"
#include "tlb.h"

// Estructura para pasar datos a los hilos
typedef struct {
    int thread_id;
    int ops;
    int num_segs;
} thread_data_t;

void* simular_proceso(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    struct segment_table *tabla = init_segment_table(data->num_segs);
    
    //inicializar la TLB 
    struct tlb tlb_instance;
    init_tlb(&tlb_instance);

    int ok = 0, faults = 0; // Contadores para estadísticas
    int tlb_hits = 0, tlb_misses = 0; // Para estadísticas de TLB

    for (int i = 0; i < data->ops; i++) {
        // Generación simple (Modo Uniforme): segmento y offset aleatorios [cite: 331]
        int seg_id = rand() % data->num_segs;
        uint64_t offset = rand() % (tabla->segments[seg_id].limit + 500); // Forzamos algunos fallos

        uint64_t pa_base;
        uint64_t pa_final;

        // Primero intentamos buscar en la TLB
        if (search_tlb(&tlb_instance, seg_id, &pa_base)) {
            tlb_hits++;

            if(offset< tabla->segments[seg_id].limit){
                pa_final = pa_base + offset;
                ok++;
            } else {
                faults++;
            }
        }
        // si no está (TLB Miss),hacemos la traducción normal
        else {
            tlb_misses++;
            if (traducir_segmento(tabla, seg_id, offset, &pa_final)) {
                ok++;
                // Actualizamos la TLB con esta nueva traducción
                update_tlb(&tlb_instance, seg_id, tabla->segments[seg_id].base);     
            } else {
                faults++;
            }
        }

    }

    printf("Thread %d terminado.\n", data->thread_id);
    printf("  -> Accesos OK: %d | Segfaults: %d\n", ok, faults);
    printf("  -> TLB Hits: %d | TLB Misses: %d\n\n", tlb_hits, tlb_misses);
    free_segment_table(tabla);
    pthread_exit(NULL);
}

int main() {
    int n_threads = 4; // --threads 
    int ops = 1000;    // --ops-per-thread
    pthread_t hilos[n_threads];
    thread_data_t info[n_threads];

    srand(42); // --seed


    for (int i = 0; i < n_threads; i++) {
        info[i].thread_id = i;
        info[i].ops = ops;
        info[i].num_segs = 4;
        pthread_create(&hilos[i], NULL, simular_proceso, &info[i]); // [cite: 15]
    }

    for (int i = 0; i < n_threads; i++) {
        pthread_join(hilos[i], NULL);
    }

    return 0;
}