#include <stdio.h>
#include <pthread.h>
#include <stdint.h> 
#include "frame_allocator.h"

void* thread_work(void* arg) {
    int vpn, tid;
    intptr_t thread_id = (intptr_t)arg; 
    
    int frame = allocate_frame(&vpn, &tid);
    if (frame != -1) {
        printf("Hilo %ld obtuvo Frame %d\n", (long)thread_id, frame);
    }
    return NULL;
}

int main() {
    printf("--- Iniciando Test de Concurrencia ---\n");
    frame_allocator_init(5, 0);
    pthread_t hilos[10];

    for(intptr_t i=0; i<10; i++) { // Paso 3: Usar intptr_t en el ciclo
        pthread_create(&hilos[i], NULL, thread_work, (void*)i);
    }

    for(int i=0; i<10; i++) {
        pthread_join(hilos[i], NULL);
    }

    frame_allocator_destroy();
    printf("--- Test de Concurrencia Finalizado ---\n");
    return 0;
}