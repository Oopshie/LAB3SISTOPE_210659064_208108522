#include "segmentacion.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/**
 * Inicializa la tabla de segmentos para un hilo.
 * El PDF pide asignar valores razonables de base y limit
 */
struct segment_table* init_segment_table(int num_segments) {
    struct segment_table *table = (struct segment_table*)malloc(sizeof(struct segment_table));
    table->num_segments = num_segments; 
    table->segments = (struct segment_entry*)malloc(sizeof(struct segment_entry) * num_segments); 


    // Inicializamos con valores de ejemplo (esto variará según el workload)
    for (int i = 0; i < num_segments; i++) {
        table->segments[i].base = i * 10000; // Bases separadas 
        table->segments[i].limit = 4096;    // Límite por defecto 
    }

    return table;
}

/**
 * Traduce una dirección virtual (seg_id, offset) a física (PA).
 * Sigue la fórmula: PA = base[seg_id] + offset[cite: 100, 101].
 */
int traducir_segmento(struct segment_table *table, int seg_id, uint64_t offset, uint64_t *pa) {
    // 1. Validar que el ID del segmento exista en la tabla
    if (seg_id < 0 || seg_id >= table->num_segments) {
        return 0; 
    }

    struct segment_entry entry = table->segments[seg_id];

    // 2. Validación obligatoria: ¿El offset es menor al límite? 
    if (offset >= entry.limit) {
        // Si no se cumple, es un segfault simulado 
        return 0; 
    }

    // 3. Cálculo de la dirección física final 
    // $PA = base + offset$
    *pa = entry.base + offset;

    return 1; // Éxito: translation_ok 
}

/**
 * Libera la memoria de la tabla cuando el hilo termina. 
 */
void free_segment_table(struct segment_table *table) {
    if (table) {
        free(table->segments);
        free(table);
    }
}