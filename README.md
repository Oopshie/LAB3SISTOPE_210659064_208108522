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

El proyecto utiliza un Makefile para gestionar la compilación de todos los módulos (segmentación, paginación, TLB, frame allocator, etc.).

Para compilar el ejecutable principal:

make all

Este comando genera el binario simulator en la raíz del proyecto.

## Instrucciones de Ejecución

El simulador se ejecuta desde la terminal. El modo de operación (`--mode`) es obligatorio.

```bash
./simulator --mode <seg|page> [OPCIONES]
```

Para ver todas las opciones disponibles, consulta el código en `src/simulator.c` o la documentación del laboratorio. Para obtener un reporte detallado de las métricas al finalizar, utiliza el flag `--stats`.

### Comandos del Makefile

para facilitar la evaluación, se han implementado los siguientes targets: 

make run: Ejecuta un ejemplo por defecto en modo segmentación con 4 threads.

make run_tests: Compila y ejecuta las pruebas unitarias de segmentación, paginación y concurrencia situadas en tests/

make clean: Elimina archivos objeto, ejecutables y reportes generados

### Reproducción de experimentos obligatorios

Para cumplir con el requisito de Artifact Reproducible, el proyecto incluye un comando de automatización que genera los resultados necesarios para el informe técnico.

make reproduce: Ejecuta los 3 experimentos definidos en el enunciado y generará los archivos JSON correspondientes en la carpeta out/

Experimentos incluidos: 
1. Experimento 1: Detección de Segfaults en modo segmentación.
2. Experimento 2: Medición del impacto de la TLB en el rendimiento (con y sin caché)
3. Experimento 3: Observación de Thrashing en entonos multihilo con frames limitados.

### Artefactos Generados

Tras la ejecución (especialmente con make reproduce), se generará un archivo en formato JSON, cuyo contenido consiste en la configuración completa, métricas globales (hit rate, page faults, evictions) y tiempo total de ejecución para permitir la comparación de resultados.

### Estructura del proyecto
src/: Código fuente de los módulos del simulador.
include/: cabeceras y definiciones de estructuras.
tests/: Pruebas unitarias para validación de lógica.
out/: Carpeta destinada a los reportes y artefactos JSON.

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
