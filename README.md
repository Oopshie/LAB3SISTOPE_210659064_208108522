Debe incluir:
• Descripci´on breve del proyecto
• Requisitos (compilador C11, pthreads)
• Instrucciones de compilaci´on: make
• Instrucciones de ejecuci´on: make run (ejecuta ejemplo por defecto)
• Instrucciones de reproducci´on: make reproduce (ejecuta los 3 experimentos)
• Ejemplos de comandos



gcc -std=c11 -pthread -Wall -Wextra -Iinclude src/simulator.c src/segmentacion.c -o simulator.exe

./simulator.exe