#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/syscall.h>

void serial_puts(char *s) {
    int len = strlen(s);

    asm volatile(
        "mov $2, %%ebx\n"
        "int $0x7F"
        :: "a"(SYS_WRITE), "c"(len), "d"((uint32_t) s)
        : "ebx"
    );

    return;
}

static void vsprintf(char *dst, const char *s, va_list args) {
    int i = 0;
    int j = 0;

    while (1) {
        char c = s[i];
        if (c == 0) break;

        if (c == '%') {
            i++;
            char next = s[i];

            if (next == 'd') {
                int arg = va_arg(args, int);
                j += itoa(dst + j, arg);
            } else if (next == 'u') {
                uint32_t arg = va_arg(args, uint32_t);
                j += itoa(dst + j, arg);
            } else if (next == 'x') {
                uint32_t arg = va_arg(args, uint32_t);
                j += int2hex(dst + j, arg);
            } else if (next == 'c') {
                char c = va_arg(args, int);
                dst[j++] = c;
            } else if (next == 's') {
                char *ptr = va_arg(args, char*);
                j += strcpy(dst + j, ptr);
            } else if (next == '%') {
                dst[j++] = '%';
            }
        } else {
            dst[j++] = c;
        }

        i++;
    }

    dst[j] = 0; // null-terminate the string
}

void sprintf(char *dst, const char *restrict s, ...) {
    va_list args;
    va_start(args, s);
    vsprintf(dst, s, args);
    va_end(args);

    return;
}

static void vfprintf(file_t *fp, const char *s, va_list args) {
    char buffer[1024];
    vsprintf(buffer, s, args);
    _Syscall_write(fp, buffer);
}

// static void vfprintf(file_t *fp, const char *s, va_list args) {
//     int i = 0;

//     while (1) {
//         char c = s[i];
//         if (c == 0) break;

//         if (c == '%') {
//             i++;
//             char next = s[i];

//             if (next == 'd') {
//                 int arg = va_arg(args, int);
//                 char ptr[10];
//                 itoa(ptr, arg);
//                 _Syscall_write(fp, ptr);
//             } else if (next == 'u') {
//                 uint32_t arg = va_arg(args, uint32_t);
//                 char ptr[10];
//                 itoa(ptr, arg);
//                 _Syscall_write(fp, ptr);
//             } else if (next == 'x') {
//                 uint32_t arg = va_arg(args, uint32_t);
//                 char ptr[8];
//                 int2hex(ptr, arg);
//                 _Syscall_write(fp, ptr);
//             } else if (next == 'c') {
//                 char c = va_arg(args, int);
//                 _Syscall_write(fp, &c);
//             } else if (next == 's') {
//                 char *ptr = va_arg(args, char*);
//                 _Syscall_write(fp, ptr);
//             } else if (next == '%') {
//                 _Syscall_write(fp, "%");
//             }
//         } else {
//             _Syscall_write(fp, &c);
//         }

//         i++;
//     }
// }

void fprintf(file_t *f, const char *restrict s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(f, s, args);
    va_end(args);

    return;
}

// just reads keyboard buffer
void scanf(char *dst, uint32_t size) {
    asm volatile(
        "int $0x7F" // execution will pause here
        :: "a"(SYS_READ), "b"((uint32_t) stdin), "c"(size), "d"((uint32_t) dst)
    );

    return;
}

void puts(const char *s) {
    if (s == (void *) 0) return;

    _Syscall_write(stdout, s);

    return;
}

static inline uint32_t _listdir_int(uint32_t path, uint32_t buffer, uint32_t size, uint32_t node_count) {
    uint32_t ret = 0;

    asm volatile(
        "int $0x7F"
        : "=a"(ret)
        : "a"(SYS_LISTDIR), "b"(path), "c"(buffer), "d"(size), "S"(node_count)
        : "memory"
    );

    asm volatile("" ::: "memory"); // force memory barrier
    return ret;
}

uint32_t listdir(char *dir, node_t **dst) {
    volatile uint32_t count = 0;
    node_t *buffer;

    if (_listdir_int((uint32_t) dir, 0, 0, (uint32_t) &count) != 0) {
        return 0; // error
    }

    uint32_t buf_size = 128 * count;
    buffer = malloc(buf_size);

    if (_listdir_int((uint32_t) dir, (uint32_t) buffer, buf_size, 0) != 0) {
        free(*dst);
        return 0; // error
    }

    *dst = buffer;

    return count;
}