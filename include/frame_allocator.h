#ifndef FRAME_ALLOCATOR_H
#define FRAME_ALLOCATOR_H

#include <pthread.h>

// Función para inicializar el asignador de frames con un número total de frames.
void frame_allocator_init(int num_frames, int unsafe_mode);

// Función para destruir el asignador de frames y liberar recursos.
void frame_allocator_destroy();

// Asigna un frame libre. Si no hay frames libres, desaloja uno usando FIFO.
// Devuelve el número del frame asignado (o -1 si hay error).
// Los punteros de salida `victim_...` se llenan si ocurre un desalojo.
int allocate_frame(int *victim_vpn, int *victim_thread_id);

// Registra que un frame ahora está siendo usado por una página específica (vpn) de un hilo (thread_id).
// Esta función también añade el frame a la cola FIFO para el desalojo.
void frame_allocator_map_frame(int frame_number, int vpn, int thread_id);


#endif // FRAME_ALLOCATOR_H
