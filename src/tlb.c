#include <stdlib.h>
#include "tlb.h"

/*  Descripción: Inicializa la TLB con un tamaño dinámico
    Entradas: tlb_ptr: Puntero a la estructura tlb que se va a inicializar.
              size: Cantidad de entradas que tendrá el caché (puede ser 0 para deshabilitarlo).
    Salida: void.
 */
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

/*  Descripción: Libera la memoria dinámica utilizada por las entradas de la TLB.
    Entradas: tlb_ptr: Puntero a la estructura tlb a destruir.
    Salida: void.
 */
void destroy_tlb(struct tlb *tlb_ptr) {
    if (tlb_ptr && tlb_ptr->entries) {
        free(tlb_ptr->entries);
    }
}

/*  Descripción: Busca una traducción en la TLB para un número de página virtual (VPN) dado.
                Si encuentra una entrada válida que coincida con el tag, se considera un "TLB Hit".
    Entradas: tlb_ptr: Puntero a la TLB del hilo actual.
              tag: Número de página virtual (VPN) que se desea buscar.
              frame_number: Puntero donde se escribirá el número de marco si hay un acierto.
    Salida: true si es un TLB Hit, false si es un TLB Miss.
*/
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

/* Descripción: Inserta o actualiza una traducción en la TLB utilizando el algoritmo 
                de reemplazo FIFO (First-In, First-Out). Cuando el caché está lleno, sobrescribe 
                la entrada más antigua según el índice circular.
    Entradas: tlb_ptr: Puntero a la TLB del hilo.
              tag: Número de página virtual (VPN) a insertar.
              frame_number: Número de marco físico asociado a la página.
    Salida: void.
 */
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

/*  Descripción: Invalida una entrada específica en la TLB (Obligatorio tras una evicción)
    Entradas: tlb_ptr: Puntero a la TLB donde se realizará la invalidación.
              tag: Número de página virtual (VPN) que debe marcarse como inválido.
    Salida: void.
*/
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