#pragma once

#include <common.h>

void memset(void *dst, uint8_t c, uint32_t n);
void memcpy(void *dst, void *src, uint32_t num);
void memmove(void *dst, void *src, uint32_t n);

int strcpy(char *dst, const char *src);
int strncpy(char *dst, const char *src, int n);
int strcmp(char *s, const char *s1);
int strncmp(char *s, const char *s1, int n);
int strlen(const char *s);

int itoa(char *dst, int n);
int ltoa(char *dst, int64_t n);
int uitoa(char *dst, uint32_t n);
int ultoa(char *dst, uint64_t n);
int int2hex(char *dst, uint32_t n);

int split_string(char *str, char delim, char **tokens, int max_tokens);