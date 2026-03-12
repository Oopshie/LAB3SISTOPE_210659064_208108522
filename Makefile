CC = gcc
CFLAGS = -std=c11 -pthread -Wall -Wextra -Iinclude
SRC = src/simulator.c src/segmentacion.c src/workloads.c src/tlb.c src/frame_allocator.c src/paginacion.c
OBJ = $(SRC:.c=.o)
TARGET = simulator

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Regla explícita para crear los .o usando la carpeta include
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET) tests/*.exe
	rm -rf out/*.json out/*.out

# Target: make run  
run: all
	mkdir -p out
	./$(TARGET) --mode seg --threads 4 --workload uniform --ops-per-thread 5000 --stats

# Target: make reproduce (Ejecuta los 3 experimentos obligatorios) 
reproduce: all
	mkdir -p out
	@echo "--- Ejecutando Experimento 1: Segmentación ---"
	./$(TARGET) --mode seg --threads 1 --workload uniform --ops-per-thread 10000 --segments 4 --seg-limits 1024,2048,4096,8192 --seed 100 --stats
	mv out/summary.json out/exp1_summary.json 

	@echo "--- Ejecutando Experimento 2: Impacto TLB ---"
	# 2.1 Sin TLB
	./$(TARGET) --mode page --threads 1 --workload 80-20 --ops-per-thread 50000 --pages 128 --frames 64 --page-size 4096 --tlb-size 0 --seed 200 --stats
	mv out/summary.json out/exp2_no_tlb_summary.json
	# 2.2 Con TLB
	./$(TARGET) --mode page --threads 1 --workload 80-20 --ops-per-thread 50000 --pages 128 --frames 64 --page-size 4096 --tlb-size 32 --seed 200 --stats 
	mv out/summary.json out/exp2_with_tlb_summary.json 

	@echo "--- Ejecutando Experimento 3: Thrashing ---"
	# 3.1 Un hilo
	./$(TARGET) --mode page --threads 1 --workload uniform --ops-per-thread 10000 --pages 64 --frames 8 --page-size 4096 --tlb-size 16 --seed 300 --stats 
	# 3.2 Ocho hilos (Thrashing)
	./$(TARGET) --mode page --threads 8 --workload uniform --ops-per-thread 10000 --pages 64 --frames 8 --page-size 4096 --tlb-size 16 --seed 300 --stats 

	@echo "--- Proceso finalizado. Archivos generados en out/ ---"

# Reglas para los Tests Unitarios
test_seg: tests/test_segmentacion.c src/segmentacion.c
	$(CC) $(CFLAGS) tests/test_segmentacion.c src/segmentacion.c -o tests/test_seg.exe

test_page: tests/test_paginacion.c src/paginacion.c src/frame_allocator.c src/tlb.c
	$(CC) $(CFLAGS) tests/test_paginacion.c src/paginacion.c src/frame_allocator.c src/tlb.c -o tests/test_page.exe

test_conc: tests/test_concurrencia.c src/frame_allocator.c
	$(CC) $(CFLAGS) tests/test_concurrencia.c src/frame_allocator.c -o tests/test_conc.exe

# Target: make run_tests (Ejecuta los tests unitarios)
run_tests: test_seg test_page test_conc
	@echo "--- Iniciando Pruebas Unitarias ---"
	./tests/test_seg.exe && ./tests/test_page.exe && ./tests/test_conc.exe
	@echo "--- Todas las pruebas pasaron con éxito ---"