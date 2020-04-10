struct url {
    struct Str proto;  // Before "://"
    struct Str rest;   // After "://"
};

static enum result parse_url(struct url *out, struct Str url) {
    out->proto = str_until_char(url, ':');
    if (url.len >= out->proto.len + 3) {
        const char *c = url.data + out->proto.len;
        if (c[0] == ':' && c[1] == '/' && c[2] == '/') {
            out->rest = str_after(url, out->proto.len + 3);
            return OK;
        }
    }
    return INPUT_ERROR;
}
