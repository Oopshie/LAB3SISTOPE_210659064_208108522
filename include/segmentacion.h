#ifndef SEGMENTACION_H
#define SEGMENTACION_H

#include <stdint.h>

// Cada entrada de la tabla representa un "cuarto" de memoria 
struct segment_entry {
    uint64_t base;  // Dirección física donde inicia el segmento 
    uint64_t limit; // Tamaño máximo permitido (en bytes) 
};

// La tabla que tendrá cada hilo (proceso simulado) 
struct segment_table {
    struct segment_entry *segments; // Arreglo de entradas 
    int num_segments;               // Cantidad de segmentos 
};

struct segment_table* init_segment_table(int num_segs);
int traducir_segmento(struct segment_table *table, int seg_id, uint64_t offset, uint64_t *pa);
void free_segment_table(struct segment_table *table);

#endif