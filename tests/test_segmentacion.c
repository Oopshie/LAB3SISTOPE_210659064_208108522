#include <stdio.h>
#include <assert.h>
#include "../include/segmentacion.h"

int main() {
    printf("--- Iniciando Test de Segmentación ---\n");
    // Inicializamos con 2 segmentos: Seg0 (limit 100), Seg1 (limit 200)
    struct segment_table *table = init_segment_table(2, "100,200");
    uint64_t pa;

    // Prueba 1: Acceso válido
    assert(traducir_segmento(table, 0, 50, &pa) == 1);
    printf("Prueba 1 (Válida): OK. PA calculada: %llu\n", (unsigned long long)pa);

    // Prueba 2: Violación de límite (Segfault)
    assert(traducir_segmento(table, 0, 150, &pa) == 0);
    printf("Prueba 2 (Segfault): OK. Acceso bloqueado correctamente.\n");

    // Prueba 3: ID de segmento inexistente
    assert(traducir_segmento(table, 5, 10, &pa) == 0);
    printf("Prueba 3 (ID Inválido): OK.\n");

    free_segment_table(table);
    printf("--- Test de Segmentación Finalizado con Éxito ---\n");
    return 0;
}