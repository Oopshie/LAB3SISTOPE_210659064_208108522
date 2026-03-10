#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

// Tamaño de la TLB (típicamente pequeño para mantener la velocidad)
#define TLB_SIZE 16

// Una entrada individual en la TLB
struct tlb_entry {
    int tag;                // El ID del segmento (o número de página)
    uint64_t physical_base; // La dirección física base ya traducida
    bool valid;             // Indica si la entrada tiene datos válidos
};

// Estructura de la TLB
struct tlb {
    struct tlb_entry entries[TLB_SIZE];
    int next_replace_idx; // Índice para el algoritmo de reemplazo (ej. FIFO)
};

// Prototipos de funciones
void init_tlb(struct tlb *tlb_ptr);
bool search_tlb(struct tlb *tlb_ptr, int tag, uint64_t *pa);
void update_tlb(struct tlb *tlb_ptr, int tag, uint64_t pa);

#endif // TLB_H