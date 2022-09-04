#pragma once
#include <vector>

typedef u32 X11Color;

struct X11Window {
    Window window;
    Drawable draw;
    u32 width = 0;
    u32 height = 0;
};

struct X11Base {
    Display *display;
    Window root;
    i32 screen;
    u32 depth;
    Colormap colormap = {};
    GC gc;

    XColor *colors;


    XFontStruct *font;
    int font_height;
};

void x11_init(X11Base *x11);
void x11_shutdown(X11Base *x11);
void x11_window_grab_key(X11Base *x11, Window window, KeySym sym, u32 mod);
X11Color x11_add_color(X11Base *x11, u8 r, u8 g, u8 b);
X11Color x11_add_color(X11Base *x11, u32 color);

X11Window x11_create_window(X11Base *x11, u32 width, u32 height);
void x11_destroy_window(X11Base *x11, X11Window &window);

// TODO: why reference?
void x11_fill_rect(X11Base *x11, X11Window &window, i32 x, i32 y, u32 w, u32 h, X11Color color);
void x11_draw_text(X11Base *x11, X11Window &window, i32 x, i32 y, const char *text, u64 n, X11Color color);

void x11_get_window_name(X11Base *x11, Window window, char *data, u32 n);
i32 x11_get_text_width(X11Base *x11, const char *text, u32 n);
