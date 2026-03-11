#ifndef PAGINACION_H
#define PAGINACION_H

#include <stdint.h>
#include "simulator.h"

// Estructura de una entrada en la tabla de páginas
typedef struct {
    int frame_number; // Número de frame físico donde está la página
    int valid;        // 1 si la página está en memoria, 0 si no
} page_table_entry_t;

// Estructura de la tabla de páginas (una por hilo/proceso)
typedef struct {
    page_table_entry_t *entries; // Arreglo de entradas, indexado por VPN
    int num_pages;               // Número de páginas virtuales
} page_table_t;

// Prototipos de funciones
page_table_t* paginacion_init(int num_pages);
void paginacion_destroy(page_table_t *table);
int paginacion_translate(page_table_t *table, int vpn, uint64_t *pa, config_t *config, int thread_id);
void paginacion_invalidate_entry(page_table_t *table, int vpn);

#endif // PAGINACION_H
