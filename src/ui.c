#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>

static Display *display;
static Window window;
static GC gc;
static Pixmap page_pixmap;

static XftDraw *draw;
static XftFont *default_font;

struct event {
};

static void
ui_init(void) {
    display = XOpenDisplay(NULL);
    if (!display) {
        SHOW_E("Could not open display.");
        return;
    }

    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    window = XCreateWindow(display, RootWindow(display, screen),
                           0, 0, 800, 700, 0,
                           depth, InputOutput, visual,
                           0, NULL);

    int mask = ButtonPressMask | ExposureMask;
    XSelectInput(display, window, mask);

    XMapWindow(display, window);

    gc = DefaultGC(display, screen);

    page_pixmap = XCreatePixmap(display, window, 800, 700, depth);

    Colormap cmap = DefaultColormap(display, screen);
    draw = XftDrawCreate(display, page_pixmap, visual, cmap);
    default_font = XftFontOpenName(display, screen, "monospace");
    XftColor color = {0, {0x1000, 0x1000, 0x1000, 0xffff}};
    XftDrawRect(draw, &color, 0, 0, 800, 700);
}

static struct event
ui_read_input(void) {
    XEvent event;
    XNextEvent(display, &event);
    if (event.type == Expose) {
        XExposeEvent e = event.xexpose;
        XCopyArea(display, page_pixmap, window, gc,
                   e.x, e.y, e.width, e.height, e.x, e.y);
    }
    return (struct event) {};
}

static void
ui_draw_text(struct str text, int x, int y) {
    XftColor color = {0, {0xffff, 0xffff, 0xffff, 0xffff}};
    XftDrawStringUtf8(draw, &color, default_font, x, y,
            (const FcChar8 *)text.ptr, text.len);
}

struct text_size {
    int16_t x, y, off_x, off_y;
};

static struct text_size
ui_measure_text(struct str text) {
    XGlyphInfo ext;
    XftTextExtentsUtf8(display, default_font,
            (const FcChar8 *)text.ptr, text.len, &ext);
    return (struct text_size){
        .x = ext.x,
        .y = ext.y,
        .off_x = ext.xOff,
        .off_y = ext.yOff,
    };
}
