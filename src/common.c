#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef enum {false, true} bool;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct str {
    const char *ptr;
    size_t len;
};

static struct str
to_str(const char *s) {
    return (struct str){s, strlen(s)};
}

static bool
str_eq(struct str a, struct str b) {
    if (a.len != b.len) {
        return false;
    }
    for (size_t i = 0; i < a.len; i++) {
        if (a.ptr[i] != b.ptr[i]) {
            return false;
        }
    }
    return true;
}

static char
char_to_lower(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static bool
str_ieq(struct str a, struct str b) {
    if (a.len != b.len) {
        return false;
    }
    for (size_t i = 0; i < a.len; i++) {
        if (char_to_lower(a.ptr[i]) != char_to_lower(b.ptr[i])) {
            return false;
        }
    }
    return true;
}

static struct str
str_after(struct str s, size_t n) {
    if (n > s.len) {
        n = s.len;
    }
    return (struct str){s.ptr + n, s.len - n};
}

static struct str
str_until_char(struct str s, char c) {
    size_t i = 0;
    while (i < s.len && s.ptr[i] != c) {
        i++;
    }
    return (struct str){s.ptr, i};
}

// Turns a string literal into a struct str.
#define STR(s) \
    (struct str){(s), sizeof (s) - 1}

#define ARR_LEN(a) \
    (sizeof (a) / sizeof *(a))

enum result {
    OK, INPUT_ERROR, OTHER_ERROR,
};

static bool
str_to_u32(struct str s, uint32_t *n, int base) {
    uint32_t result = 0;
    for (size_t i = 0; i < s.len; i++) {
        char c = s.ptr[i];
        if (c < '0' || c > '9') {
            return false;
        }
        result *= base;
        result += c - '0';
    }
    *n = result;
    return true;
}
