#include <stdlib.h>
#include <stdio.h>
#include "workloads.h"

static next_address_func_t next_address_generator;

// --- Implementaciones de Workloads para Segmentación ---

/*  Descripción: Genera una dirección virtual para el modo segmentación usando una 
                 distribución uniforme. Selecciona un segmento al azar y un desplazamiento (offset) 
                 que puede exceder el límite para inducir fallos de segmento.
    Entradas: config: Estructura de configuración global.
              seg_id: Puntero donde se guardará el ID del segmento generado.
              offset: Puntero donde se guardará el desplazamiento generado.
    Salida: void.
*/
static void workload_uniform_seg(config_t *config, uint64_t *seg_id, uint64_t *offset) {
    *seg_id = rand() % config->num_segments;
    // Genera un offset en un rango amplio para simular accesos válidos e inválidos.
    *offset = rand() % (config->page_size * 2);
}

/*  Descripción: Implementa la distribución 80-20 para segmentación. El 80% de los 
                 accesos se concentran en el primer 20% de los segmentos disponibles, simulando 
                 el principio de localidad de referencia.
    Entradas: config: Configuración global del simulador.
              seg_id: Puntero para el ID del segmento.
              offset: Puntero para el desplazamiento.
    Salida: void.
*/
static void workload_80_20_seg(config_t *config, uint64_t *seg_id, uint64_t *offset) {
    int twenty_percent_segments = (int)(config->num_segments * 0.2);
    if (twenty_percent_segments == 0) twenty_percent_segments = 1;

    // 80% de los accesos al primer 20% de los segmentos
    if ((rand() % 100) < 80) {
        *seg_id = rand() % twenty_percent_segments;
    } else {
    // 20% de los accesos al 80% restante de los segmentos
        int remaining_segments = config->num_segments - twenty_percent_segments;
        if (remaining_segments > 0) {
            *seg_id = twenty_percent_segments + (rand() % remaining_segments);
        } else {
            *seg_id = rand() % config->num_segments;
        }
    }
    *offset = rand() % (config->page_size * 2);
}

// --- Implementaciones de Workloads para Paginación ---

/*  Descripción: Genera una dirección virtual uniforme para el modo paginación. 
                El acceso se distribuye equitativamente sobre todo el espacio de direccionamiento 
                virtual definido por el número de páginas.
    Entradas: config: Configuración global.
              virtual_address: Puntero donde se guardará la dirección virtual generada.
              unused: Parámetro no utilizado (mantiene compatibilidad de firma).
    Salida: void.
*/
static void workload_uniform_page(config_t *config, uint64_t *virtual_address, uint64_t *unused) {
    (void)unused; // Evitar warning de no usado
    uint64_t max_addr = (uint64_t)config->num_pages * config->page_size;
    *virtual_address = rand() % max_addr;
}

/*  Descripción: Genera accesos siguiendo la regla 80-20 para paginación. Concentra 
                la mayor parte de las referencias en un conjunto pequeño de páginas (hot set),  
                lo que permite evaluar la efectividad de la TLB.
    Entradas: config: Configuración global.
              virtual_address: Puntero para la dirección virtual resultante.
              unused: Parámetro no utilizado.
    Salida: void.
 */
static void workload_80_20_page(config_t *config, uint64_t *virtual_address, uint64_t *unused) {
    (void)unused; // Evitar warning de no usado
    int twenty_percent_pages = (int)(config->num_pages * 0.2);
    if (twenty_percent_pages == 0) twenty_percent_pages = 1;

    uint64_t vpn;
    // 80% de los accesos al primer 20% de las páginas
    if ((rand() % 100) < 80) {
        vpn = rand() % twenty_percent_pages;
    } else {
    // 20% de los accesos al 80% restante de las páginas
        int remaining_pages = config->num_pages - twenty_percent_pages;
        if (remaining_pages > 0) {
            vpn = twenty_percent_pages + (rand() % remaining_pages);
        } else {
            vpn = rand() % config->num_pages;
        }
    }

    uint64_t offset = rand() % config->page_size;
    *virtual_address = (vpn * config->page_size) + offset;
}


// --- Funciones Públicas ---

/*  Descripción: Inicializa el generador de carga seleccionando la función 
                 específica según el modo (segmentación/paginación) y el tipo de workload 
                 (uniforme/80-20) definidos en la configuración.
    Entradas: config: Estructura con las preferencias del usuario.
    Salida: void.
 */
void init_workloads(config_t *config) {
    if (config->mode == MODE_SEGMENTATION) {
        if (config->workload == WORKLOAD_80_20) {
            next_address_generator = workload_80_20_seg;
        } else {
            next_address_generator = workload_uniform_seg;
        }
    } else { // MODE_PAGINATION
        if (config->workload == WORKLOAD_80_20) {
            next_address_generator = workload_80_20_page;
        } else {
            next_address_generator = workload_uniform_page;
        }
    }
}

/*  Descripción: Interfaz pública para obtener la siguiente dirección de la simulación. 
                Llama dinámicamente a la función de generación configurada previamente.
    Entradas: config: Configuración global.
              addr1: Puntero para el primer componente de la dirección (seg_id o virtual_addr).
              addr2: Puntero para el segundo componente (offset o unused).
    Salida: void.
*/
void generate_address(config_t *config, uint64_t *addr1, uint64_t *addr2) {
    next_address_generator(config, addr1, addr2);
}
