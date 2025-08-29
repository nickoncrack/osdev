#pragma once

#include <common.h>

#define PIT_FREQUENCY 1193

void pit_install(uint32_t freq);

void ksleep(uint32_t ms);
uint32_t pit_get_ticks();

uint64_t rdtsc();