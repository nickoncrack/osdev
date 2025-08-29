#pragma once

#define SYS_EXIT        0x00
#define SYS_WRITE       0x01
#define SYS_READ        0x02
#define SYS_OPEN        0x03
#define SYS_MALLOC      0x04
#define SYS_FREE        0x05
#define SYS_LISTDIR     0x06
#define SYS_LOAD        0x07
#define SYS_DPROC       0x08 // destroy current process
#define SYS_NEWTASK     0x09
#define SYS_GETSTATE    0x0A
#define SYS_GETVBEINFO  0x0B
#define SYS_RQBUF       0x0C // request buffer
#define SYS_BUFREADY    0x0D


#define _Syscall_write(fp, s) { \
    asm volatile( \
        "int $0x7F" \
        :: "a"(SYS_WRITE), "b"((uint32_t) fp), "c"(strlen(s)), "d"((uint32_t) s) \
    ); \
}