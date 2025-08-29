#pragma once

#include <stdint.h>

#define NULL ((void *) 0)

void *malloc(uint32_t size);
void free(void *ptr);

void exit(uint32_t ret);