#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "segmentacion.h"

// Estructura para pasar datos a los hilos
typedef struct {
    int thread_id;
    int ops;
    int num_segs;
} thread_data_t;

void* simular_proceso(void* arg) {
    thread_data_t *data = (thread_data_t*)arg;
    struct segment_table *tabla = init_segment_table(data->num_segs);
    
    int ok = 0, faults = 0;

    for (int i = 0; i < data->ops; i++) {
        // Generación simple (Modo Uniforme): segmento y offset aleatorios [cite: 331]
        int seg_id = rand() % data->num_segs;
        uint64_t offset = rand() % (tabla->segments[seg_id].limit + 500); // Forzamos algunos fallos

        uint64_t pa;
        if (traducir_segmento(tabla, seg_id, offset, &pa)) {
            ok++;
        } else {
            faults++;
        }
    }

    printf("Thread %d terminado. OK: %d, Segfaults: %d\n", data->thread_id, ok, faults);
    
    free_segment_table(tabla);
    pthread_exit(NULL);
}

int main() {
    int n_threads = 4; // --threads [cite: 212]
    int ops = 1000;    // --ops-per-thread [cite: 212]
    pthread_t hilos[n_threads];
    thread_data_t info[n_threads];

    srand(42); // --seed [cite: 52]

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