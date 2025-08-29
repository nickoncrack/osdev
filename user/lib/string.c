#include <string.h>

void memset(void *dst, uint8_t c, uint32_t n) {
    asm volatile(
        "cld\n"
        "rep stosb"
        : "+D"(dst), "+c"(n)
        : "a"(c)
        : "memory"
    );
}

void memsetd(void *dst, uint32_t v, uint32_t n) {
    asm volatile(
        "cld\n"
        "rep stosl"
        : "+D"(dst), "+c"(n)
        : "a"(v)
        : "memory"
    );
}

void memcpy(void *dst, void *src, uint32_t n) {
    uint32_t dwords = n / 4;
    uint32_t bytes = n % 4;

    asm volatile(
        "cld\n"
        "rep movsd\n"
        "mov %[bytes], %%ecx\n"
        "rep movsb"
        : "+D"(dst), "+S"(src), "+c"(dwords)
        : [bytes] "r"(bytes)
        : "memory"
    );
}

void memmove(void *dst, void *src, uint32_t n) {
    uint8_t *d = (uint8_t *) dst;
    uint8_t *s = (uint8_t *) src;

    if (d > s && d < s + n) {
        d += n;
        s += n;
        while (n--) {
            *(--d) = *(--s);
        }
    } else {
        while (n--) {
            *d++ = *s++;
        }
    }
}

int strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';

    return i;
}

int strncpy(char *dst, const char *src, int n) {
    if (strlen(src) < n) {
        n = strlen(src);
    }

    int i;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }

    return i;
}

int strlen(const char *s) {
    int i = 0;
    while (*s != '\0') {
        i++;
        s++;
    }

    return i;
}

int strcmp(char *s, const char *s1) {
    int len = strlen(s);

    if (len != strlen(s1)) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        if (s[i] != s1[i]) {
            return 0;
        }
    }

    return 1;
}

int strncmp(char *s, const char *s1, int n) {
    for (int i = 0; i < n; i++) {
        if (s[i] != s1[i]) {
            return 0;
        }
    }

    return 1;
}

int itoa(char *dst, int n) {
    uint32_t start = 0;

    if (n == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return 0;
    }

    if (n < 0) {
        dst[start++] = '-';
        n = -n;
    }

    char buff[16];
    buff[15] = 0;
    uint32_t i = 14;

    while (n > 0) {
        uint32_t rem = n % 10;
        buff[i--] = '0' + rem;
        n /= 10;
    }

    strcpy(dst + start, buff + i + 1);
    return start + 14 - i;
}

int int2hex(char *dst, uint32_t n) {
    uint32_t start = 0;

    if (n == 0) {
        dst[0] = '0';
        dst[1] = '\0';
        return 0;
    }

    if (n < 0) {
        dst[start++] = '-';
        n = -n;
    }

    char buff[16];
    buff[15] = 0;
    uint32_t i = 14;

    while (n > 0) {
        uint32_t rem = n % 16;
        if (rem <= 9) {
            buff[i--] = '0' + rem;
        } else {
            buff[i--] = 'A' + (rem - 10);
        }
        n /= 16;
    }

    strcpy(dst + start, buff + i + 1);
    return start + 14 + i;
}

int split_string(char *str, char delim, char **tokens, int max_tokens) {
    int count = 0;
    char *start = str;

    if (!str || !tokens || max_tokens <= 0) return 0;

    while (*str && count < max_tokens) {
        if (*str == delim) {
            *str = '\0';
            tokens[count++] = start;
            start = str + 1;
        }
        str++;
    }

    if (count < max_tokens && *start != '\0')
        tokens[count++] = start;

    return count;
}