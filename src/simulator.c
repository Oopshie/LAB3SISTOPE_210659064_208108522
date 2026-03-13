#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

#include "simulator.h"
#include "workloads.h"
#include "frame_allocator.h"
#include "paginacion.h"
#include "tlb.h"

#define MAX_THREADS 64

// --- Variables Globales para Estadísticas y Sincronización ---
pthread_mutex_t g_stats_mutex;
long long g_total_translations_ok = 0;
long long g_total_segfaults = 0;
long long g_total_page_faults = 0;
long long g_total_tlb_hits = 0;
long long g_total_tlb_misses = 0;
long long g_total_evictions = 0; // Para contar evicciones en paginación

// Variables globales para paginación (para invalidación entre hilos)
page_table_t* g_page_tables[MAX_THREADS];
struct tlb* g_tlbs[MAX_THREADS];
pthread_mutex_t g_page_tables_mutex;


// --- Declaraciones de Funciones ---
void run_segmentation_simulation(config_t *config);
void* segmentation_thread_main(void *arg);
void run_pagination_simulation(config_t *config);
void* pagination_thread_main(void* arg);


// --- Lógica de Simulación de Segmentación ---

typedef struct {
    int thread_id;
    config_t *config;
} seg_thread_data_t;

/* Descripción: Función principal de ejecución para los hilos en modo segmentación.
                Cada hilo inicializa su propia tabla de segmentos, genera direcciones virtuales 
                según el workload y las traduce a físicas, contabilizando éxitos y fallos (segfaults).
    Entradas: arg: Puntero a estructura seg_thread_data_t que contiene el ID del hilo y la configuración.
    Salida: NULL (vía pthread_exit).
 */
void* segmentation_thread_main(void *arg) {
    seg_thread_data_t *data = (seg_thread_data_t *)arg;
    config_t *config = data->config;
    long long translations_ok = 0;
    long long segfaults = 0;

    struct segment_table *table = init_segment_table(config->num_segments, config->seg_limits_str);
    if (!table) {
        fprintf(stderr, "Thread %d: Fallo al inicializar la tabla de segmentos.\n", data->thread_id);
        pthread_exit(NULL);
    }

    for (int i = 0; i < config->ops_per_thread; i++) {
        uint64_t seg_id, offset, pa;
        generate_address(config, &seg_id, &offset);
        if (traducir_segmento(table, (int)seg_id, offset, &pa)) {
            translations_ok++;
        } else {
            segfaults++;
        }
    }

    if (!config->unsafe) pthread_mutex_lock(&g_stats_mutex);
    g_total_translations_ok += translations_ok;
    g_total_segfaults += segfaults;
    if (!config->unsafe) pthread_mutex_unlock(&g_stats_mutex);

    if (config->stats) {
        printf("Thread %d Metricas: { Traducciones OK: %lld, Segfaults: %lld }\n",
               data->thread_id, translations_ok, segfaults);
    }

    free_segment_table(table);
    free(data);
    pthread_exit(NULL);
}

/*  Descripción: Orquesta la simulación completa en modo segmentación.
                Inicializa mutex, mide el tiempo de ejecución, crea los hilos solicitados, 
                espera a que terminen (join) y reporta las métricas globales por consola.
    Entradas: config: Estructura con los parámetros de la simulación (hilos, operaciones, etc.).
    Salida: void.
 */
void run_segmentation_simulation(config_t *config) {
    pthread_t threads[config->num_threads];
    pthread_mutex_init(&g_stats_mutex, NULL);
    g_total_translations_ok = 0;
    g_total_segfaults = 0;

    if (config->stats) {
        printf("-> Iniciando simulacion en modo: SEGMENTACION\n");
        printf("============================================================\n");
        printf("Configuracion: %d hilos, %d ops/hilo, workload=%s, seed=%d\n",
               config->num_threads, config->ops_per_thread,
               config->workload == WORKLOAD_UNIFORM ? "uniform" : "80-20", config->seed);
        printf("============================================================\n\n");
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < config->num_threads; i++) {
        seg_thread_data_t *thread_data = malloc(sizeof(seg_thread_data_t));
        thread_data->thread_id = i;
        thread_data->config = config;
        pthread_create(&threads[i], NULL, segmentation_thread_main, thread_data);
    }
    for (int i = 0; i < config->num_threads; i++) pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9;

    if (config->stats) {
        long long total_ops = g_total_translations_ok + g_total_segfaults;
        printf("============================================================\n");
        printf("Metricas Globales (Segmentacion):\n");
        printf("  - Traducciones OK: %lld\n", g_total_translations_ok);
        printf("  - Segfaults: %lld\n", g_total_segfaults);
        printf("------------------------------------------------------------\n");
        printf("Tiempo total: %.2f segundos\n", time_taken);
        if (time_taken > 0) printf("Throughput: %.2f ops/seg\n", total_ops / time_taken);
        if (total_ops > 0) printf("Tiempo promedio por traduccion: %.2f ns\n", (time_taken * 1e9) / total_ops);
        printf("============================================================\n\n");
    }
    pthread_mutex_destroy(&g_stats_mutex);
}


// --- Lógica de Simulación de Paginación ---

typedef struct {
    int thread_id;
    config_t *config;
} pag_thread_data_t;

/*  Descripción: Función principal de ejecución para los hilos en modo paginación.
                Gestiona la jerarquía de memoria: busca en la TLB, maneja fallos de página llamando 
                al sistema de traducción y actualiza el caché (TLB). Contabiliza hits, misses y faults.
    Entradas: arg: Puntero a pag_thread_data_t con la información de contexto del hilo.
    Salida: NULL (vía pthread_exit).
 */
void* pagination_thread_main(void* arg) {
    pag_thread_data_t *data = (pag_thread_data_t*)arg;
    config_t *config = data->config;
    int thread_id = data->thread_id;

    long long tlb_hits = 0;
    long long tlb_misses = 0;
    long long page_faults = 0;

    page_table_t* pt = paginacion_init(config->num_pages);
    struct tlb* tlb = malloc(sizeof(struct tlb));
    init_tlb(tlb, config->tlb_size); // Usar tamaño dinámico

    if (!config->unsafe) pthread_mutex_lock(&g_page_tables_mutex);
    g_page_tables[thread_id] = pt;
    g_tlbs[thread_id] = tlb;
    if (!config->unsafe) pthread_mutex_unlock(&g_page_tables_mutex);

    for (int i = 0; i < config->ops_per_thread; i++) {
        uint64_t virtual_addr, unused, pa_base, final_pa, frame_number;
        generate_address(config, &virtual_addr, &unused);

        int vpn = virtual_addr / config->page_size;
        int offset = virtual_addr % config->page_size;

        if (search_tlb(tlb, vpn, &frame_number)) {
            tlb_hits++;
            pa_base = frame_number * config->page_size;
        } else {
            tlb_misses++;
            if (!pt->entries[vpn].valid) {
                page_faults++;
            }
            paginacion_translate(pt, vpn, &pa_base, config, thread_id);
            update_tlb(tlb, vpn, pa_base / config->page_size);
        }
        final_pa = pa_base + offset;
        (void)final_pa; // Evitar warning de variable no usada
    }

    if (!config->unsafe) pthread_mutex_lock(&g_stats_mutex);
    g_total_tlb_hits += tlb_hits;
    g_total_tlb_misses += tlb_misses;
    g_total_page_faults += page_faults;
    if (!config->unsafe) pthread_mutex_unlock(&g_stats_mutex);
    
    destroy_tlb(tlb); // Liberar memoria de la TLB
    free(tlb);
    paginacion_destroy(pt);
    free(data);
    pthread_exit(NULL);
}

/*  Descripción: Orquesta la simulación completa en modo paginación.
                Configura el asignador de marcos (frame allocator), inicializa los mutex de 
                sincronización global, lanza los hilos y al finalizar calcula las métricas 
                de rendimiento como el Hit Rate y el Throughput.
    Entradas: config: Estructura de configuración global del simulador.
    Salida: void.
 */
void run_pagination_simulation(config_t *config) {
    if (config->num_threads > MAX_THREADS) {
        fprintf(stderr, "Error: El número de hilos excede el máximo de %d\n", MAX_THREADS);
        return;
    }

    pthread_t threads[config->num_threads];
    pthread_mutex_init(&g_stats_mutex, NULL);
    pthread_mutex_init(&g_page_tables_mutex, NULL);
    
    g_total_tlb_hits = 0;
    g_total_tlb_misses = 0;
    g_total_page_faults = 0;

    frame_allocator_init(config->num_frames, config->unsafe);

    if (config->stats) {
        printf("-> Iniciando simulacion en modo: PAGINACION\n");
        printf("============================================================\n");
        printf("Configuracion: %d hilos, %d ops/hilo, %d frames, %d paginas/hilo, tlb=%d\n",
               config->num_threads, config->ops_per_thread, config->num_frames, config->num_pages, config->tlb_size);
        printf("============================================================\n\n");
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < config->num_threads; i++) {
        pag_thread_data_t *thread_data = malloc(sizeof(pag_thread_data_t));
        thread_data->thread_id = i;
        thread_data->config = config;
        pthread_create(&threads[i], NULL, pagination_thread_main, thread_data);
    }
    for (int i = 0; i < config->num_threads; i++) pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = end.tv_sec - start.tv_sec + (end.tv_nsec - start.tv_nsec) / 1e9;

    frame_allocator_destroy();

    if (config->stats) {
        long long total_tlb_lookups = g_total_tlb_hits + g_total_tlb_misses;
        double hit_rate = total_tlb_lookups > 0 ? (double)g_total_tlb_hits / total_tlb_lookups * 100.0 : 0.0;
        
        printf("============================================================\n");
        printf("Metricas Globales (Paginacion):\n");
        printf("  - TLB Hits: %lld\n", g_total_tlb_hits);
        printf("  - TLB Misses: %lld\n", g_total_tlb_misses);
        printf("  - TLB Hit Rate: %.2f%%\n", hit_rate);
        printf("  - Page Faults: %lld\n", g_total_page_faults);
        printf("------------------------------------------------------------\n");
        printf("Tiempo total: %.2f segundos\n", time_taken);
        if (time_taken > 0) printf("Throughput: %.2f ops/seg\n", (double)config->num_threads * config->ops_per_thread / time_taken);
        printf("============================================================\n\n");
    }
    
    pthread_mutex_destroy(&g_stats_mutex);
    pthread_mutex_destroy(&g_page_tables_mutex);
}

/*  Descripción: Genera un reporte final en formato JSON compatible con la rúbrica.
                Captura toda la configuración y las métricas acumuladas durante la ejecución 
                para guardarlas en "out/summary.json", permitiendo la reproducibilidad del experimento.
    Entradas: config: Configuración utilizada en la simulación.
               runtime_sec: Tiempo total de ejecución medido en segundos.
    Salida: void.
 */
void save_summary(config_t *config, double runtime_sec) {
    FILE *fp = fopen("out/summary.json", "w");
    if (!fp) {
        perror("Error al crear out/summary.json");
        return;
    }

    long long total_ops = (long long)config->num_threads * config->ops_per_thread;
    long long total_tlb_lookups = g_total_tlb_hits + g_total_tlb_misses;
    double hit_rate = total_tlb_lookups > 0 ? (double)g_total_tlb_hits / total_tlb_lookups : 0.0;
    double throughput = runtime_sec > 0 ? (double)total_ops / runtime_sec : 0.0;
    double avg_time_ns = total_ops > 0 ? (runtime_sec * 1e9) / total_ops : 0.0;

    fprintf(fp, "{\n");
    fprintf(fp, "  \"mode\": \"%s\",\n", config->mode == MODE_SEGMENTATION ? "seg" : "page");
    fprintf(fp, "  \"config\": {\n");
    fprintf(fp, "    \"threads\": %d,\n", config->num_threads);
    fprintf(fp, "    \"ops_per_thread\": %d,\n", config->ops_per_thread);
    fprintf(fp, "    \"workload\": \"%s\",\n", config->workload == WORKLOAD_80_20 ? "80-20" : "uniform");
    fprintf(fp, "    \"seed\": %d,\n", config->seed);
    fprintf(fp, "    \"unsafe\": %s,\n", config->unsafe ? "true" : "false");
    fprintf(fp, "    \"pages\": %d,\n", config->num_pages);
    fprintf(fp, "    \"frames\": %d,\n", config->num_frames);
    fprintf(fp, "    \"page_size\": %d,\n", config->page_size);
    fprintf(fp, "    \"tlb_size\": %d,\n", config->tlb_size);
    fprintf(fp, "    \"tlb_policy\": \"fifo\",\n");
    fprintf(fp, "    \"evict_policy\": \"fifo\"\n");
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"metrics\": {\n");
    fprintf(fp, "    \"tlb_hits\": %lld,\n", g_total_tlb_hits);
    fprintf(fp, "    \"tlb_misses\": %lld,\n", g_total_tlb_misses);
    fprintf(fp, "    \"hit_rate\": %.3f,\n", hit_rate);
    fprintf(fp, "    \"page_faults\": %lld,\n", g_total_page_faults);
    fprintf(fp, "    \"evictions\": %lld,\n", g_total_page_faults > (long long)config->num_frames ? g_total_page_faults - config->num_frames : 0);
    fprintf(fp, "    \"avg_translation_time_ns\": %.2f,\n", avg_time_ns);
    fprintf(fp, "    \"throughput_ops_sec\": %.2f\n", throughput);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"runtime_sec\": %.3f\n", runtime_sec);
    fprintf(fp, "}\n");

    fclose(fp);
    printf("-> Resumen guardado en out/summary.json\n");
}

// --- Punto de Entrada Principal ---

/*  Descripción: Punto de entrada del programa. 
                Parsea los argumentos de la línea de comandos (getopt_long), inicializa los 
                workloads y lanza la simulación correspondiente (segmentación o paginación).
                Finalmente, mide el tiempo total y activa la generación del resumen JSON.
    Entradas: argc: Cantidad de argumentos.
              argv: Vector de cadenas de texto con los argumentos.
    Salida: 0 si la ejecución fue exitosa.
 */
int main(int argc, char *argv[]) {
    config_t config;
    config.mode = -1;
    config.num_threads = 1;
    config.ops_per_thread = 1000;
    config.workload = WORKLOAD_UNIFORM;
    config.seed = 42;
    config.unsafe = 0;
    config.stats = 0;
    config.num_segments = 4;
    config.seg_limits_str = "4096,4096,4096,4096";
    config.num_pages = 64;
    config.num_frames = 32;
    config.page_size = 4096;
    config.tlb_size = 16;

    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'}, {"threads", required_argument, 0, 't'},
        {"ops-per-thread", required_argument, 0, 'o'}, {"workload", required_argument, 0, 'w'},
        {"seed", required_argument, 0, 's'}, {"unsafe", no_argument, 0, 'u'},
        {"stats", no_argument, 0, 'S'}, {"segments", required_argument, 0, 'g'},
        {"seg-limits", required_argument, 0, 'l'}, {"pages", required_argument, 0, 'p'},
        {"frames", required_argument, 0, 'f'}, {"page-size", required_argument, 0, 'z'},
        {"tlb-size", required_argument, 0, 'T'}, {0, 0, 0, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "m:t:o:w:s:uSg:l:p:f:z:T:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm':
                if (strcmp(optarg, "seg") == 0) config.mode = MODE_SEGMENTATION;
                else if (strcmp(optarg, "page") == 0) config.mode = MODE_PAGINATION;
                break;
            case 't': config.num_threads = atoi(optarg); break;
            case 'o': config.ops_per_thread = atoi(optarg); break;
            case 'w': if (strcmp(optarg, "80-20") == 0) config.workload = WORKLOAD_80_20; break;
            case 's': config.seed = atoi(optarg); break;
            case 'u': config.unsafe = 1; break;
            case 'S': config.stats = 1; break;
            case 'g': config.num_segments = atoi(optarg); break;
            case 'l': config.seg_limits_str = optarg; break;
            case 'p': config.num_pages = atoi(optarg); break;
            case 'f': config.num_frames = atoi(optarg); break;
            case 'z': config.page_size = atoi(optarg); break;
            case 'T': config.tlb_size = atoi(optarg); break;
            default: exit(EXIT_FAILURE);
        }
    }

    srand(config.seed);
    init_workloads(&config);

    struct timespec global_start, global_end;
    clock_gettime(CLOCK_MONOTONIC, &global_start);

    if (config.mode == MODE_SEGMENTATION) {
        run_segmentation_simulation(&config);
    } else if (config.mode == MODE_PAGINATION) {
        run_pagination_simulation(&config);
    } else {
        fprintf(stderr, "Error: El modo de operacion es obligatorio.\n");
        fprintf(stderr, "Uso: %s --mode <seg|page> [opciones]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &global_end);
    double total_runtime = (global_end.tv_sec - global_start.tv_sec) + 
                           (global_end.tv_nsec - global_start.tv_nsec) / 1e9;

    save_summary(&config, total_runtime);

    return 0;

}