#ifndef TLB_H
#define TLB_H

#include <stdint.h>
#include <stdbool.h>

// Una entrada individual en la TLB
struct tlb_entry {
    int tag;                // El ID del segmento (o número de página virtual, VPN)
    uint64_t frame_number;  // El número de frame físico correspondiente
    bool valid;             // Indica si la entrada tiene datos válidos
};

// Estructura de la TLB, ahora con tamaño dinámico
struct tlb {
    struct tlb_entry *entries; // Puntero a un arreglo de entradas
    int size;                  // Tamaño de la TLB (número de entradas)
    int next_replace_idx;      // Índice para el algoritmo de reemplazo (ej. FIFO)
};

// Prototipos de funciones actualizados
void init_tlb(struct tlb *tlb_ptr, int size);
void destroy_tlb(struct tlb *tlb_ptr);
bool search_tlb(struct tlb *tlb_ptr, int tag, uint64_t *frame_number);
void update_tlb(struct tlb *tlb_ptr, int tag, uint64_t frame_number);

#endif // TLB_H