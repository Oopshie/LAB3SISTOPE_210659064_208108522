#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "paginacion.h"
#include "frame_allocator.h"

// --- Definiciones necesarias para el Linker (Faltaban estas) ---
// Como paginacion.c las usa como 'extern', debemos definirlas aquí
pthread_mutex_t g_page_tables_mutex = PTHREAD_MUTEX_INITIALIZER;
page_table_t *g_page_tables[64]; // MAX_THREADS es usualmente 64

int main() {
    printf("--- Iniciando Test de Paginacion ---\n");
    
    // Configuración de prueba
    config_t config = {
        .page_size = 4096, 
        .num_frames = 2, 
        .unsafe = 0
    }; 
    
    frame_allocator_init(config.num_frames, 0);
    page_table_t *pt = paginacion_init(4);
    uint64_t pa_base;

    // Prueba 1: Traducción con Page Fault (debe asignar frame 0)
    assert(paginacion_translate(pt, 0, &pa_base, &config, 0) == 1);
    printf("Traduciendo VPN 0 (Esperando Page Fault)...\n");
    
    // Prueba 2: Traducción exitosa (Corregido %lu a %llu)
    assert(paginacion_translate(pt, 0, &pa_base, &config, 0) == 1);
    printf("Prueba 2 (Pagina en memoria): OK. PA Base: %llu\n", pa_base);

    paginacion_destroy(pt);
    frame_allocator_destroy();
    printf("--- Test de Paginacion Finalizado con Exito ---\n");
    return 0;
}