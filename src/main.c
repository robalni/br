#include <stdlib.h>

#include "common.c"
#include "error.c"
#include "url.c"
#include "http.c"
#include "html.c"
#include "net.c"
#include "ui.c"
#include "x.c"

static void load_page(struct Str url_str) {
    struct url url;
    if (parse_url(&url, url_str) != OK) {
        fprintf(stderr, "Bad url.\n");
        return;
    }

    struct connection conn;
    net_connect(&conn, url);
    send_http_request(conn, url);

    char *page_content = malloc(0x2000);
    size_t recvd_len = 0;
    struct parse_state parser = {
        .code = {page_content, recvd_len},
    };
    size_t headers_len = 0;
    size_t content_len = 0;  // Specified in HTTP header.
    bool has_read_first_line = false;
    bool has_read_headers = false;
    // Receive data into page_content and parse as much as possible
    // each time.
    for (;;) {
        if (recvd_len >= 0x2000) {
            break;
        }
        if (has_read_headers && recvd_len >= headers_len + content_len) {
            break;
        }
        size_t len_to_recv = content_len
            ? MIN(0x2000 - recvd_len, content_len - recvd_len + headers_len)
            : 0x1000;
        int ret = recv(conn.sock, page_content + recvd_len, len_to_recv, 0);
        fprintf(stderr, "recvd %d\n", ret);
        if (ret == -1) {
            perror("recv");
            return;
        } else if (ret == 0) {
            break;
        }
        recvd_len += ret;
        parser.code.len = recvd_len;

        bool parsed_some = false;
        do {
            if (has_read_headers) {
            } else if (!has_read_first_line) {
                struct http_status status;
                if (parse_status_line(&status, &parser)) {
                    has_read_first_line = true;
                    parsed_some = true;
                }
            } else {
                struct http_header h;
                while (parse_header(&h, &parser) && h.name.len > 0) {
                    fprintf(stderr, "HEADER [%.*s] [%.*s]\n",
                            (int)h.name.len, h.name.data,
                            (int)h.value.len, h.value.data);
                    parsed_some = true;
                    if (str_ieq(h.name, STR("content-length"))) {
                        uint32_t len;
                        if (!str_to_u32(h.value, &len, 10)) {
                            return;
                        }
                        content_len = len;
                    }
                }
                if (h.name.len == 0) {
                    has_read_headers = true;
                    headers_len = parser.offset;
                }
            }
        } while (parsed_some && !has_read_headers);
    }
    fprintf(stderr, "%.*s\n", (int)parser.code.len, parser.code.data);

    bool should_linebreak = false;
    int margin = 0;  // Only makes sense if should linebreak.
    int y = 20;
    int x = 0;
    bool last_was_text = false;
    bool last_was_space = true;
    int16_t line_height = 16;
    struct TextSize last_size;
#define TAG_STACK_SIZE 100
    const struct tag_info *tag_stack[TAG_STACK_SIZE];
    size_t tag_stack_len = 0;

    uint8_t hidden = 0;  // Number of DISPLAY_NONE tags we are inside.

    for (;;) {
        char text[100];
        struct parsed_fragment f
            = parse_response(&parser, text, sizeof text);
        if (f.type == NONE) {
            break;
        }
        switch (f.type) {
        case TEXT:
            fprintf(stderr, "%.*s",
                    (int)f.text.len, f.text.data);
            if ((f.text.len > 1 || !str_eq(f.text, STR(" "))) && !hidden) {
                if (last_was_space && f.text.len > 0 && f.text.data[0] == ' ') {
                    f.text = str_after(f.text, 1);
                }
                if (should_linebreak) {
                    x = 0;
                    y += line_height + margin;
                }
                should_linebreak = false;
                margin = 0;
                last_size = ui_measure_text(f.text);
                ui_draw_text(f.text.data, f.text.len, x, y);
                x += last_size.off_x;
                y += last_size.off_y;
                last_was_text = true;
                last_was_space = f.text.data[f.text.len - 1] == ' ';
            }
            break;
        case START_TAG: {
            struct parse_state tag_parser = {.code = f.tag};

            const struct tag_info *tag = get_tag_info(read_tag_name(&tag_parser));

            if (!tag->empty) {
                if (tag_stack_len >= TAG_STACK_SIZE) {
                    SHOW_E("Too many tags");
                    return;
                }
                tag_stack[tag_stack_len] = tag;
                tag_stack_len++;

                if (tag->display == DISPLAY_NONE) {
                    hidden++;
                }
            }

            fprintf(stderr, "<%.*s%s>",
                    (int)tag->name.len, tag->name.data,
                    tag->empty ? " /" : "");
            if (tag->display == DISPLAY_BLOCK) {
                margin = MAX(margin, 16);
                should_linebreak = true;
            }
            last_was_text = false;
            last_was_space = true;
        } break;
        case END_TAG: {
            struct parse_state tag_parser = {.code = f.tag};
            const struct tag_info *
                end_tag = get_tag_info(read_tag_name(&tag_parser));
            const struct tag_info *tag = tag_stack[tag_stack_len - 1];
            if (tag != end_tag) {
                LOG_W("Tags not properly nested");
            }
            fprintf(stderr, "</%.*s>",
                    (int)tag->name.len, tag->name.data);
            if (end_tag->display == DISPLAY_NONE) {
                hidden--;
            }
            if (tag->display == DISPLAY_BLOCK) {
                margin = MAX(margin, 16);
                should_linebreak = true;
            }
            last_was_text = false;
            last_was_space = true;

            if (tag_stack_len > 0) {
                tag_stack_len--;
            } else {
                LOG_W("Too many closing tags");
            }
        } break;
        }
    }
}

int main(int argc, char **argv) {
    ui_init("Browser of the Web");

    const char *url_str = NULL;
    if (argc == 2) {
        url_str = argv[1];
        load_page(to_str(url_str));
    }

    for (;;) {
        struct UiEvent ev = ui_read_input();
        if (ev.render) {
            ui_fill(0, 0, ui_window_width(), ui_window_height(), 0xffffff);
            ui_fill(10, 30, ui_window_width() - 20, 2, 0x773322);
            ui_draw_text(argv[1], strlen(argv[1]), 10, 20);
            struct Str text = STR("Welcome to my browser :) :) :) :) :) :) :) :) :) :) :) :) :) :) :)");
            ui_draw_text(text.data, text.len, 10, 58);
            struct Str text2 = STR("It doesn't javascript :) :) :) :)");
            ui_draw_text(text2.data, text2.len, 10, 82);
            ui_show();
        }
        if (ev.quit) {
            break;
        }
        ui_sleep_ms(1);
    }

    return 0;
}
