struct http_status {
    uint16_t code;
    struct Str desc;
};

struct http_header {
    struct Str name;
    struct Str value;
};

struct parse_state {
    struct Str code;
    size_t offset;
};

// Parses the first line of an HTTP response and updates s->offset to
// the next line if successful.
static bool parse_status_line(struct http_status *hs, struct parse_state *s) {
    struct Str line = {s->code.data + s->offset, 0};
    while (s->offset + line.len < s->code.len
            && (unsigned char)line.data[line.len] >= 0x20) {
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
static bool parse_header(struct http_header *h, struct parse_state *s) {
    struct Str line = {s->code.data + s->offset, 0};
    while (s->offset + line.len < s->code.len
            && (unsigned char)line.data[line.len] >= 0x20) {
        line.len++;
    }
    // If we hit the end instead of newline then we need more data
    // before we can continue.
    if (s->offset + line.len >= s->code.len) {
        return false;
    }

    size_t i = 0;

    h->name.data = line.data;
    while (i < line.len && line.data[i] != ':') {
        i++;
    }
    h->name.len = i;
    if (i < line.len) {
        i++;
    }
    while (i < line.len && line.data[i] == ' ') {
        i++;
    }
    h->value.data = line.data + i;
    while (i < line.len && line.data[i] != '\r' && line.data[i] != '\n') {
        i++;
    }
    h->value.len = i - (h->value.data - line.data);
    if (s->offset + i + 1 < s->code.len) {
        if (line.data[i] == '\r' && line.data[i + 1] == '\n') {
            i += 2;
        }
    }
    s->offset += i;
    return true;
}
