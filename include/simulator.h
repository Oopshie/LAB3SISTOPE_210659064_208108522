#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <pthread.h>
#include <stdint.h>
#include "segmentacion.h"
#include "tlb.h"

// Enum para los modos de operación
typedef enum {
    MODE_SEGMENTATION,
    MODE_PAGINATION
} simulator_mode_t;

// Enum para los tipos de workload
typedef enum {
    WORKLOAD_UNIFORM,
    WORKLOAD_80_20
} workload_t;

// Estructura para almacenar la configuración de la simulación
typedef struct {
    simulator_mode_t mode;
    int num_threads;
    int ops_per_thread;
    workload_t workload;
    int seed;
    int unsafe;
    int stats;

    // Parámetros de segmentación
    int num_segments;
    char *seg_limits_str;

    // Parámetros de paginación
    int num_pages;
    int num_frames;
    int page_size;
    int tlb_size;
} config_t;

#endif // SIMULATOR_H
