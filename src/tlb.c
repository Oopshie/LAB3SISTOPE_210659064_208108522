#include "tlb.h"
#include <stdio.h>

// Inicializa la TLB marcando todas las entradas como inválidas
void init_tlb(struct tlb *tlb_ptr) {
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb_ptr->entries[i].valid = false;
    }
    tlb_ptr->next_replace_idx = 0; // El primero en entrar será el primero en salir
}

// Busca un segmento en la TLB
bool search_tlb(struct tlb *tlb_ptr, int tag, uint64_t *pa_base) {
    for (int i = 0; i < TLB_SIZE; i++) {
        // Si la entrada es válida y el tag coincide, ¡tenemos un TLB Hit!
        if (tlb_ptr->entries[i].valid && tlb_ptr->entries[i].tag == tag) {
            *pa_base = tlb_ptr->entries[i].physical_base;
            return true;
        }
    }
    return false; // TLB Miss
}

// Actualiza la TLB con una nueva traducción (Algoritmo FIFO)
void update_tlb(struct tlb *tlb_ptr, int tag, uint64_t pa_base) {
    int idx = tlb_ptr->next_replace_idx;

    // Sobrescribimos la entrada más antigua
    tlb_ptr->entries[idx].tag = tag;
    tlb_ptr->entries[idx].physical_base = pa_base;
    tlb_ptr->entries[idx].valid = true;

    // Movemos el índice al siguiente (circularmente)
    tlb_ptr->next_replace_idx = (idx + 1) % TLB_SIZE;
}