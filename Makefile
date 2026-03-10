CC = gcc
CFLAGS = -std=c11 -pthread -Wall -Wextra -Iinclude
SRC = src/simulator.c src/segmentacion.c
OBJ = $(SRC:.c=.o)
TARGET = simulator

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Regla explícita para crear los .o usando la carpeta include
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)


gcc -std=c11 -pthread -Wall -Wextra -Iinclude src/simulator.c src/segmentacion.c src/tlb.c -o simulator.exe