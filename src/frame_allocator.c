#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "frame_allocator.h"

// Estructura para mantener información sobre cada frame
typedef struct {
    int is_free;
    int vpn;
    int thread_id;
} frame_info_t;

// --- Variables Globales Estáticas (privadas a este archivo) ---

// Tabla que representa todos los frames de la memoria física
static frame_info_t *g_frame_table;
static int g_num_frames;

// Cola para la política de desalojo FIFO
static int *g_fifo_queue;
static int g_fifo_head;
static int g_fifo_tail;
static int g_fifo_count;

// Mutex para proteger el acceso a las estructuras de datos del asignador
static pthread_mutex_t g_allocator_mutex;
static int g_unsafe_mode;

// --- Implementación de las Funciones ---

void frame_allocator_init(int num_frames, int unsafe_mode) {
    g_num_frames = num_frames;
    g_unsafe_mode = unsafe_mode;

    // Reservar memoria para la tabla de frames
    g_frame_table = (frame_info_t*)malloc(sizeof(frame_info_t) * num_frames);
    for (int i = 0; i < num_frames; i++) {
        g_frame_table[i].is_free = 1; // Marcar todos los frames como libres
        g_frame_table[i].vpn = -1;
        g_frame_table[i].thread_id = -1;
    }

    // Reservar memoria para la cola FIFO
    g_fifo_queue = (int*)malloc(sizeof(int) * num_frames);
    g_fifo_head = 0;
    g_fifo_tail = 0;
    g_fifo_count = 0;

    // Inicializar el mutex
    if (!g_unsafe_mode) {
        pthread_mutex_init(&g_allocator_mutex, NULL);
    }
}

void frame_allocator_destroy() {
    free(g_frame_table);
    free(g_fifo_queue);
    if (!g_unsafe_mode) {
        pthread_mutex_destroy(&g_allocator_mutex);
    }
}

int allocate_frame(int *victim_vpn, int *victim_thread_id) {
    if (!g_unsafe_mode) pthread_mutex_lock(&g_allocator_mutex);

    // Por defecto, no hay víctima
    *victim_vpn = -1;
    *victim_thread_id = -1;

    // 1. Buscar un frame libre
    for (int i = 0; i < g_num_frames; i++) {
        if (g_frame_table[i].is_free) {
            g_frame_table[i].is_free = 0; // Marcarlo como ocupado
            if (!g_unsafe_mode) pthread_mutex_unlock(&g_allocator_mutex);
            return i; // Devolver el número de frame libre
        }
    }

    // 2. Si no hay frames libres, desalojar usando FIFO
    if (g_fifo_count == 0) { // No debería pasar si todos los frames están ocupados
        if (!g_unsafe_mode) pthread_mutex_unlock(&g_allocator_mutex);
        return -1; // Error
    }

    // Obtener la víctima del frente de la cola
    int victim_frame = g_fifo_queue[g_fifo_head];
    g_fifo_head = (g_fifo_head + 1) % g_num_frames;
    g_fifo_count--;

    // Llenar la información de la víctima para el que llama
    *victim_vpn = g_frame_table[victim_frame].vpn;
    *victim_thread_id = g_frame_table[victim_frame].thread_id;

    if (!g_unsafe_mode) pthread_mutex_unlock(&g_allocator_mutex);
    
    // Devolver el frame desalojado para ser reutilizado
    return victim_frame;
}

void frame_allocator_map_frame(int frame_number, int vpn, int thread_id) {
    if (frame_number < 0 || frame_number >= g_num_frames) return;

    if (!g_unsafe_mode) pthread_mutex_lock(&g_allocator_mutex);

    // Actualizar la tabla de frames con la nueva información de la página
    g_frame_table[frame_number].vpn = vpn;
    g_frame_table[frame_number].thread_id = thread_id;

    // Añadir el frame al final de la cola FIFO
    g_fifo_queue[g_fifo_tail] = frame_number;
    g_fifo_tail = (g_fifo_tail + 1) % g_num_frames;
    g_fifo_count++;

    if (!g_unsafe_mode) pthread_mutex_unlock(&g_allocator_mutex);
}
