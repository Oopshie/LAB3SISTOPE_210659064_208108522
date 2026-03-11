# Simulador de Memoria Virtual Concurrente

Este proyecto es una implementación en C de un simulador de memoria virtual concurrente, desarrollado como parte del Laboratorio 3 de Sistemas Operativos. El simulador modela los esquemas de **segmentación** y **paginación** y utiliza `pthreads` para simular la ejecución concurrente de múltiples procesos.

## Requisitos

- Un compilador de C (se recomienda `gcc`).
- Un entorno tipo Unix que soporte `pthreads`.
- `make` (opcional, para facilitar la compilación).

## Instrucciones de Compilación

Para compilar el proyecto, sitúate en el directorio raíz y ejecuta el siguiente comando. Este compilará todos los módulos (`segmentación`, `paginación`, `TLB`, `frame allocator`, etc.) en un único ejecutable llamado `simulator`.

```bash
gcc -std=c11 -pthread -Wall -Wextra -Iinclude src/simulator.c src/segmentacion.c src/workloads.c src/tlb.c src/frame_allocator.c src/paginacion.c -o simulator
```

## Instrucciones de Ejecución

El simulador se ejecuta desde la terminal. El modo de operación (`--mode`) es obligatorio.

```bash
./simulator --mode <seg|page> [OPCIONES]
```

Para ver todas las opciones disponibles, consulta el código en `src/simulator.c` o la documentación del laboratorio. Para obtener un reporte detallado de las métricas al finalizar, utiliza el flag `--stats`.

### Ejemplos de Comandos

A continuación se muestran los comandos para replicar los experimentos definidos en el laboratorio.

#### Experimento 1: Segmentación con Segfaults

Este experimento demuestra la correcta detección de violaciones de segmento.

```bash
./simulator --mode seg --threads 1 --workload uniform --ops-per-thread 10000 --segments 4 --seg-limits 1024,2048,4096,8192 --seed 100 --stats
```

#### Experimento 2: Impacto de la TLB

Este experimento compara el rendimiento de la paginación con y sin la TLB.

**Con TLB (tamaño 16):**
```bash
./simulator --mode page --threads 1 --workload 80-20 --ops-per-thread 50000 --pages 128 --frames 64 --tlb-size 16 --seed 200 --stats
```

**Sin TLB:**
```bash
./simulator --mode page --threads 1 --workload 80-20 --ops-per-thread 50000 --pages 128 --frames 64 --tlb-size 0 --seed 200 --stats
```

#### Experimento 3: Thrashing

Este experimento demuestra el colapso del rendimiento del sistema (thrashing) cuando múltiples hilos compiten por una cantidad muy limitada de frames de memoria.

**Carga normal (1 hilo):**
```bash
./simulator --mode page --threads 1 --workload uniform --ops-per-thread 10000 --pages 64 --frames 8 --tlb-size 16 --seed 300 --stats
```

**Carga alta (8 hilos, provoca thrashing):**
```bash
./simulator --mode page --threads 8 --workload uniform --ops-per-thread 10000 --pages 64 --frames 8 --tlb-size 16 --seed 300 --stats
```
