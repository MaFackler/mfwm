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

    void init();
    void shutdown();
    X11Window create_window(u32 width, u32 height);
    void destroy_window(X11Window &window);
    X11Color add_color(u8 r, u8 g, u8 b);

    void fill_rect(X11Window &window, X11Color color);
};
