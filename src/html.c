enum fragment_type {
    NONE,  // NONE means that there was nothing more to parse.
    START_TAG, END_TAG,
    TEXT,
};

struct parsed_fragment {
    enum fragment_type type;
    // The union has one member for each type except for NONE.
    union {
        struct str tag;         // START_TAG or END_TAG
        struct str text;        // TEXT
    };
};

struct response_header {
    uint16_t code;
    size_t content_length;
};

enum display {
    DISPLAY_NONE,
    DISPLAY_INLINE,
    DISPLAY_BLOCK,
};

struct tag_info {
    struct str name;
    bool empty;  // Self-closing
    enum display display;
};

// These tags must be sorted by name to make searching fast.
static struct tag_info
all_tags[] = {
    {
        .name = STR("a"),
        .empty = false,
        .display = DISPLAY_INLINE,
    },
    {
        .name = STR("body"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("div"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h1"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h2"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h3"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h4"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h5"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("h6"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("head"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("html"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("li"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("link"),
        .empty = true,
        .display = DISPLAY_INLINE,
    },
    {
        .name = STR("meta"),
        .empty = true,
        .display = DISPLAY_INLINE,
    },
    {
        .name = STR("ol"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("p"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
    {
        .name = STR("span"),
        .empty = false,
        .display = DISPLAY_INLINE,
    },
    {
        .name = STR("title"),
        .empty = false,
        .display = DISPLAY_NONE,
    },
    {
        .name = STR("ul"),
        .empty = false,
        .display = DISPLAY_BLOCK,
    },
};

static struct tag_info *
get_tag_info(struct str name) {
    for (size_t i = 0; i < ARR_LEN(all_tags); i++) {
        if (str_eq(all_tags[i].name, name)) {
            return all_tags + i;
        }
    }
    static struct tag_info unknown = {0};
    return &unknown;
}

static struct str
read_tag_name(struct parse_state *s) {
    struct str name = {NULL, 0};
    size_t o = s->offset;
    while (o < s->code.len) {
        char c = s->code.ptr[o];
        if (!name.ptr && c != '<' && c > ' ' && c != '/') {
            name.ptr = s->code.ptr + o;
        } else if (name.ptr && (c <= ' ' || c == '>')) {
            name.len = o - (name.ptr - s->code.ptr);
            break;
        }
        o++;
    }
    s->offset = o;
    return name;
}

static struct parsed_fragment
parse_response(struct parse_state *s, char *buf, size_t buf_len) {
    struct parsed_fragment f;
    if (s->offset == s->code.len) {
        f.type = NONE;
    } else if (s->code.ptr[s->offset] == '<') {
        bool inside_quotes = false;
        bool end_tag = false;
        char seen_chars = 0;
        size_t end = s->offset;
        while (end < s->code.len) {
            char c = s->code.ptr[end];
            if (c == '"') {
                inside_quotes = !inside_quotes;
            } else if (!inside_quotes) {
                if (c == '>') {
                    end++;
                    break;
                } else if (c == '/' && (seen_chars & 0x40) == 0) {
                    // Only if we have not seen a letter int the tag yet.
                    end_tag = true;
                }
            }
            end++;
            seen_chars |= c;
        }
        f.type = end_tag ? END_TAG : START_TAG;
        f.tag.ptr = s->code.ptr + s->offset;
        f.tag.len = end - s->offset;
        s->offset = end;
    } else {
        size_t end = s->offset;
        bool last_was_space = false;
        size_t len = 0;
        while (end < s->code.len && len < buf_len) {
            if (s->code.ptr[end] == '<') {
                break;
            }
            if ((unsigned char)s->code.ptr[end] <= ' ') {
                if (!last_was_space) {
                    buf[len] = ' ';
                    last_was_space = true;
                    len++;
                }
            } else {
                buf[len] = s->code.ptr[end];
                last_was_space = false;
                len++;
            }
            end++;
        }
        f.type = TEXT;
        f.text.ptr = buf;
        f.text.len = len;
        s->offset = end;
    }
    return f;
}
