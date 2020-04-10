#include <xcb/xcb.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unistd.h>

struct UiXContext {
    xcb_connection_t *conn;
    char *buf;
    uint32_t win;
    uint32_t width;
    uint32_t height;
    xcb_gcontext_t gc;
    xcb_intern_atom_reply_t *win_close_message;
    FT_Library freetype;
    FT_Face font;
} global_ui_x;

static void ui_init(const char *window_title) {
    struct UiXContext *xc = &global_ui_x;
    xc->conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(xc->conn)) {
        fprintf(stderr, "X connection error\n");
        exit(1);
    }

    const xcb_setup_t *setup = xcb_get_setup(xc->conn);
    xcb_screen_iterator_t scr_iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t *screen = scr_iter.data;

    xc->win = xcb_generate_id(xc->conn);
    xc->width = 576;
    xc->height = 576;

    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values[] = {
        XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_KEY_RELEASE
    };
    xcb_create_window(xc->conn,
                      XCB_COPY_FROM_PARENT,
                      xc->win,
                      screen->root,
                      0, 0, xc->width, xc->height,
                      0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      mask, values);

    xc->gc = xcb_generate_id(xc->conn);
    xcb_create_gc(xc->conn, xc->gc, xc->win, 0, NULL);

    // We want to get an event when the window is closed.
    {
        xcb_intern_atom_cookie_t protocols = xcb_intern_atom_unchecked(xc->conn, 1, 12, "WM_PROTOCOLS");
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xc->conn, protocols, 0);
        xcb_intern_atom_cookie_t delwin = xcb_intern_atom_unchecked(xc->conn, 0, 16, "WM_DELETE_WINDOW");
        xc->win_close_message = xcb_intern_atom_reply(xc->conn, delwin, 0);
        xcb_change_property(xc->conn, XCB_PROP_MODE_REPLACE, xc->win, reply->atom, XCB_ATOM_ATOM, 32, 1, &xc->win_close_message->atom);
        free(reply);
    }

    // Set window title
    {
        xcb_intern_atom_cookie_t wmname = xcb_intern_atom_unchecked(xc->conn, 1, 7, "WM_NAME");
        xcb_intern_atom_reply_t *namereply = xcb_intern_atom_reply(xc->conn, wmname, 0);
        xcb_change_property(xc->conn, XCB_PROP_MODE_REPLACE, xc->win, namereply->atom, XCB_ATOM_STRING, 8, strlen(window_title), window_title);
        free(namereply);
    }

    xcb_map_window(xc->conn, xc->win);
    xcb_flush(xc->conn);

    FT_Init_FreeType(&xc->freetype);
    FT_New_Face(xc->freetype, "font", 0, &xc->font);
    FT_Set_Char_Size(xc->font, 0, 14 * 64, 0, 72);

    xc->buf = malloc(xc->width * xc->height * 4);
}

static enum UiKey ui_key(int code) {
    switch (code) {
    case 113: return UI_LEFT;
    case 114: return UI_RIGHT;
    default: return UI_NOKEY;
    }
}

static struct UiEvent ui_read_input(void) {
    struct UiXContext *xc = &global_ui_x;
    struct UiEvent event = {0};
    for (;;) {
        xcb_generic_event_t *gev = xcb_poll_for_event(xc->conn);
        if (gev == NULL) {
            break;
        }
        switch (gev->response_type & ~0x80) {
        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *ev = (xcb_key_press_event_t *)gev;
            event.keyup = 0;
            event.keydown = ev->detail;
            ui_set_key(ui_key(ev->detail), UI_PRESSED);
        } break;
        case XCB_KEY_RELEASE: {
            xcb_key_release_event_t *ev = (xcb_key_release_event_t *)gev;
            event.keydown = 0;
            event.keyup = ev->detail;
            ui_set_key(ui_key(ev->detail), UI_NOT_PRESSED);
        } break;
        case XCB_EXPOSE:
            event.render = true;
            break;
        case XCB_CONFIGURE_NOTIFY: {
            xcb_configure_notify_event_t *ev = (xcb_configure_notify_event_t *)gev;
            xc->width = ev->width;
            xc->height = ev->height;
            free(xc->buf);
            xc->buf = malloc(xc->width * xc->height * 4);
            event.resize = true;
        } break;
        case XCB_CLIENT_MESSAGE:
            event.quit = true;
            break;
        }
    }
    return event;
}

static void ui_show() {
    struct UiXContext *xc = &global_ui_x;
    xcb_put_image(xc->conn,
                  XCB_IMAGE_FORMAT_Z_PIXMAP,
                  xc->win,
                  xc->gc,
                  xc->width, xc->height,
                  0, 0,
                  0,
                  24,
                  xc->width * xc->height * 4,
                  xc->buf);
}

static void ui_sleep_ms(int ms) {
    usleep(ms * 1000);
}

static int ui_window_width(void) {
    struct UiXContext *xc = &global_ui_x;
    return xc->width;
}

static int ui_window_height(void) {
    struct UiXContext *xc = &global_ui_x;
    return xc->height;
}

static void ui_fill(int x, int y, int w, int h, uint32_t color) {
    struct UiXContext *xc = &global_ui_x;
    uint32_t *buf = (uint32_t *)xc->buf + y * xc->width + x;
    for (size_t j = 0; j < h; j++) {
        for (size_t i = 0; i < w; i++) {
            buf[i] = color;
        }
        buf += xc->width;
    }
}

static void ui_draw_text(const char *text, size_t len, int x, int y) {
    struct UiXContext *xc = &global_ui_x;
    uint32_t *buf = (uint32_t*)xc->buf;
    for (size_t i = 0; i < len; i++) {
        uint32_t ch = text[i];
        if ((ch & 0x80) && len > i + 1) {
            ch = ((ch & 0x1f) << 6) + (text[i+1] & 0x3f);
            i++;
        }
        FT_Load_Char(xc->font, ch, FT_LOAD_RENDER);
        FT_Bitmap *bm = &xc->font->glyph->bitmap;
        unsigned char *glyph = bm->buffer;
        for (size_t gy = 0; gy < bm->rows; gy++) {
            size_t bm_i = 0;
            for (size_t j = 0; j < bm->width; j++) {
                uint8_t value = glyph[bm_i];
                uint32_t *pixel = &buf[x + xc->font->glyph->bitmap_left + j + (y - xc->font->glyph->bitmap_top) * xc->width];
                uint8_t negval = ~value;
                *pixel = negval | negval << 8 | negval << 16;
                bm_i++;
            }
            buf += xc->width;
            glyph += bm->pitch;
        }
        x += xc->font->glyph->advance.x >> 6;
        y += xc->font->glyph->advance.y >> 6;
        buf -= xc->width * bm->rows;
    }
}

struct TextSize {
    int16_t x, y, off_x, off_y;
};

static struct TextSize ui_measure_text(struct Str text) {
}
