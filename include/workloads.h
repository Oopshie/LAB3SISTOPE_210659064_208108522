#ifndef WORKLOADS_H
#define WORKLOADS_H

#include "simulator.h"

// Un puntero a una función que genera una dirección.
// Esto nos permite seleccionar la función de workload una vez al inicio.
typedef void (*next_address_func_t)(config_t *config, uint64_t *addr1, uint64_t *addr2);

// Prototipos de funciones
void init_workloads(config_t *config);
void generate_address(config_t *config, uint64_t *addr1, uint64_t *addr2);

#endif // WORKLOADS_H
