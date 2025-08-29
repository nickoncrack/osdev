#pragma once

#include <stdio.h>
#include <stdlib.h>

#define assert(c) __assert(c, __FILE__, __LINE__)

inline void __assert(int condition, char *file, uint32_t line) {
    if (!condition) {
        fprintf(stdout, "Assertion failed in %s:%d\n", file, line);
        exit(3);
    }
}