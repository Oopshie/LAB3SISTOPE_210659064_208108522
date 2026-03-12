#include <stdlib.h>
#include "tlb.h"

// Inicializa la TLB con un tamaño dinámico
void init_tlb(struct tlb *tlb_ptr, int size) {
    tlb_ptr->size = size;
    tlb_ptr->next_replace_idx = 0;
    if (size > 0) {
        tlb_ptr->entries = (struct tlb_entry*)malloc(sizeof(struct tlb_entry) * size);
        for (int i = 0; i < size; i++) {
            tlb_ptr->entries[i].valid = false;
        }
    } else {
        tlb_ptr->entries = NULL;
    }
}

// Libera la memoria de las entradas de la TLB
void destroy_tlb(struct tlb *tlb_ptr) {
    if (tlb_ptr && tlb_ptr->entries) {
        free(tlb_ptr->entries);
    }
}

// Busca un VPN (tag) en la TLB
bool search_tlb(struct tlb *tlb_ptr, int tag, uint64_t *frame_number) {
    if (tlb_ptr->size == 0) return false;

    for (int i = 0; i < tlb_ptr->size; i++) {
        // Si la entrada es válida y el tag coincide, ¡tenemos un TLB Hit!
        if (tlb_ptr->entries[i].valid && tlb_ptr->entries[i].tag == tag) {
            *frame_number = tlb_ptr->entries[i].frame_number;
            return true;
        }
    }
    return false; // TLB Miss
}

// Actualiza la TLB con una nueva traducción (Algoritmo FIFO)
void update_tlb(struct tlb *tlb_ptr, int tag, uint64_t frame_number) {
    if (tlb_ptr->size == 0) return;

    int idx = tlb_ptr->next_replace_idx;

    // Sobrescribimos la entrada más antigua
    tlb_ptr->entries[idx].tag = tag;
    tlb_ptr->entries[idx].frame_number = frame_number;
    tlb_ptr->entries[idx].valid = true;

    // Movemos el índice al siguiente (circularmente)
    tlb_ptr->next_replace_idx = (idx + 1) % tlb_ptr->size;
}

// Invalida una entrada específica en la TLB (Obligatorio tras una evicción)
void tlb_invalidate(struct tlb *tlb_ptr, int tag) {
    // SEGURO: Si la TLB está deshabilitada, no hay nada que invalidar
    if (tlb_ptr->size == 0 || tlb_ptr->entries == NULL) return;

    for (int i = 0; i < tlb_ptr->size; i++) {
        if (tlb_ptr->entries[i].valid && tlb_ptr->entries[i].tag == tag) {
            tlb_ptr->entries[i].valid = false;
            // No hacemos break porque un tag podría estar repetido si no se maneja bien,
            // aunque en una TLB ideal solo hay uno.
        }
    }
}