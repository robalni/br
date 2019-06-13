struct http_status {
    uint16_t code;
    struct str desc;
};

struct http_header {
    struct str name;
    struct str value;
};

struct parse_state {
    struct str code;
    size_t offset;
};

// Parses the first line of an HTTP response and updates s->offset to
// the next line if successful.
static bool
parse_status_line(struct http_status *hs, struct parse_state *s) {
    struct str line = {s->code.ptr + s->offset, 0};
    while (s->offset + line.len < s->code.len
            && (unsigned char)line.ptr[line.len] >= 0x20) {
        line.len++;
    }
    // If we hit the end instead of newline then we need more data
    // before we can continue.
    if (s->offset + line.len >= s->code.len) {
        return false;
    }

    s->offset += line.len + 2;
    // TODO: the real thing
    *hs = (struct http_status){200, STR("")};
    return true;
}

// Parses one header in an HTTP response and updates s->offset to the
// next line if successful.
static bool
parse_header(struct http_header *h, struct parse_state *s) {
    struct str line = {s->code.ptr + s->offset, 0};
    while (s->offset + line.len < s->code.len
            && (unsigned char)line.ptr[line.len] >= 0x20) {
        line.len++;
    }
    // If we hit the end instead of newline then we need more data
    // before we can continue.
    if (s->offset + line.len >= s->code.len) {
        return false;
    }

    size_t i = 0;

    h->name.ptr = line.ptr;
    while (i < line.len && line.ptr[i] != ':') {
        i++;
    }
    h->name.len = i;
    if (i < line.len) {
        i++;
    }
    while (i < line.len && line.ptr[i] == ' ') {
        i++;
    }
    h->value.ptr = line.ptr + i;
    while (i < line.len && line.ptr[i] != '\r' && line.ptr[i] != '\n') {
        i++;
    }
    h->value.len = i - (h->value.ptr - line.ptr);
    if (s->offset + i + 1 < s->code.len) {
        if (line.ptr[i] == '\r' && line.ptr[i + 1] == '\n') {
            i += 2;
        }
    }
    s->offset += i;
    return true;
}
