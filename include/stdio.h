#pragma once

#include <common.h>
#include <stdarg.h>
#include <fs/skbdfs.h>

extern file_t *stdout;

void printf(const char *fmt, ...);
void fprintf(file_t *fp, const char *s, ...);