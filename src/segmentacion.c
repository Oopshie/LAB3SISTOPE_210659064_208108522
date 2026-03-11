#define _POSIX_C_SOURCE 200809L
#include "segmentacion.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * Inicializa la tabla de segmentos para un hilo.
 * Parsea los límites desde un string y asigna las bases de forma contigua.
 */
struct segment_table* init_segment_table(int num_segments, const char *limits_str) {
    struct segment_table *table = (struct segment_table*)malloc(sizeof(struct segment_table));
    if (!table) return NULL;

    table->num_segments = num_segments;
    table->segments = (struct segment_entry*)malloc(sizeof(struct segment_entry) * num_segments);
    if (!table->segments) {
        free(table);
        return NULL;
    }

    // strktok modifica el string, así que trabajamos sobre una copia
    char *limits_copy = strdup(limits_str);
    char *token = strtok(limits_copy, ",");

    uint64_t current_base = 0;
    for (int i = 0; i < num_segments; i++) {
        uint64_t limit = 4096; // Valor por defecto si no hay más tokens
        if (token != NULL) {
            limit = strtoul(token, NULL, 10);
            token = strtok(NULL, ",");
        }

        table->segments[i].limit = limit;
        table->segments[i].base = current_base;

        // El siguiente segmento empieza donde termina el actual
        current_base += limit;
    }

    free(limits_copy); // Liberar la copia del string de límites
    return table;
}

/**
 * Traduce una dirección virtual (seg_id, offset) a física (PA).
 * Sigue la fórmula: PA = base[seg_id] + offset.
 */
int traducir_segmento(struct segment_table *table, int seg_id, uint64_t offset, uint64_t *pa) {
    // 1. Validar que el ID del segmento exista en la tabla
    if (seg_id < 0 || seg_id >= table->num_segments) {
        return 0; // Fallo: ID de segmento inválido
    }

    struct segment_entry entry = table->segments[seg_id];

    // 2. Validación obligatoria: ¿El offset es menor al límite? 
    if (offset >= entry.limit) {
        // Si no se cumple, es un segfault simulado 
        return 0; // Fallo: Violación de segmento (Segfault)
    }

    // 3. Cálculo de la dirección física final
    *pa = entry.base + offset;

    return 1; // Éxito: traducción correcta
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