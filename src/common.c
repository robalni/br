#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef enum {false, true} bool;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct Str {
    const char *data;
    size_t len;
};

static struct Str
to_str(const char *s) {
    return (struct Str){s, strlen(s)};
}

static bool
str_eq(struct Str a, struct Str b) {
    if (a.len != b.len) {
        return false;
    }
    for (size_t i = 0; i < a.len; i++) {
        if (a.data[i] != b.data[i]) {
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
str_ieq(struct Str a, struct Str b) {
    if (a.len != b.len) {
        return false;
    }
    for (size_t i = 0; i < a.len; i++) {
        if (char_to_lower(a.data[i]) != char_to_lower(b.data[i])) {
            return false;
        }
    }
    return true;
}

static struct Str
str_after(struct Str s, size_t n) {
    if (n > s.len) {
        n = s.len;
    }
    return (struct Str){s.data + n, s.len - n};
}

static struct Str
str_until_char(struct Str s, char c) {
    size_t i = 0;
    while (i < s.len && s.data[i] != c) {
        i++;
    }
    return (struct Str){s.data, i};
}

// Turns a string literal into a struct Str.
#define STR(s) \
    (struct Str){(s), sizeof (s) - 1}

#define ARR_LEN(a) \
    (sizeof (a) / sizeof *(a))

enum result {
    OK, INPUT_ERROR, OTHER_ERROR,
};

static bool
str_to_u32(struct Str s, uint32_t *n, int base) {
    uint32_t result = 0;
    for (size_t i = 0; i < s.len; i++) {
        char c = s.data[i];
        if (c < '0' || c > '9') {
            return false;
        }
        result *= base;
        result += c - '0';
    }
    *n = result;
    return true;
}
