#include <stdio.h>
#include <stdlib.h>

__attribute__((section(".text._start")))
void _start() {
    fprintf(stdout, "Hello from test program\n");
    
    exit(0);
}