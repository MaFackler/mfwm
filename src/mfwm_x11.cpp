
void x11_init(X11Base *x11) {
    x11->display = XOpenDisplay(NULL);
    x11->screen = XDefaultScreen(x11->display);
    x11->root = XRootWindow(x11->display, x11->screen);

    u64 cursor = XCreateFontCursor(x11->display, 2);

    {
        XSetWindowAttributes attribs = {};
        attribs.event_mask = SubstructureNotifyMask |
            SubstructureRedirectMask | KeyPressMask |
            EnterWindowMask | FocusChangeMask |
            PropertyChangeMask | PointerMotionMask |
            NoEventMask | ButtonPressMask | 
            StructureNotifyMask;
        attribs.cursor = cursor;
        XChangeWindowAttributes(x11->display, x11->root, CWEventMask | CWCursor, &attribs);
        XSelectInput(x11->display, x11->root, attribs.event_mask);
    }
    XSync(x11->display, 0);
    x11->depth = XDefaultDepth(x11->display, x11->screen);
    x11->gc = XDefaultGC(x11->display, x11->screen);
    x11->colormap = XDefaultColormap(x11->display, x11->screen);
    XSetFillStyle(x11->display, x11->gc, FillSolid);

    i32 n = 0;
    char **fonts = XListFonts(x11->display, "*", 256, &n);
    MF_Assert(n > 0);
    // TODO: move to xft for better fonts
    //x11->font = XftFontOpenName(x11->display, x11->screen, fontname);
    x11->font = XLoadQueryFont(x11->display, fonts[0]);
    MF_Assert(x11->font);
    x11->font_height = x11->font->ascent + x11->font->descent;
    XFreeFontNames(fonts);
}

void x11_shutdown(X11Base *x11) {
    XFreeGC(x11->display, x11->gc);
    XCloseDisplay(x11->display);
}

void x11_window_grab_key(X11Base *x11, Window window, KeySym sym, u32 mod) {
    KeyCode code = XKeysymToKeycode(x11->display, sym);
    XGrabKey(x11->display, code, mod, window, 1, GrabModeAsync, GrabModeAsync); 
}

X11Window x11_window_create(X11Base *x11, u32 x, u32 y, u32 width, u32 height) {
    XSetWindowAttributes attribs = {};
    attribs.background_pixel = ParentRelative;
    attribs.event_mask = ButtonPressMask | ExposureMask;
    Window window = XCreateWindow(x11->display,
                                  x11->root,
                                  x, y,
                                  width,
                                  height,
                                  0,
                                  x11->depth,
                                  CopyFromParent,
                                  XDefaultVisual(x11->display, x11->screen),
                                  CWEventMask | CWBackPixel,
                                  &attribs);
    XMapWindow(x11->display, window);
    Drawable draw = XCreatePixmap(x11->display, window, width, height, x11->depth);
    X11Window res = {window, draw, width, height};
    return res;
};

void x11_window_destroy(X11Base *x11, X11Window &window) {
    XFreePixmap(x11->display, window.draw);
}

void x11_window_focus(X11Base *x11, Window window) {
    XSetInputFocus(x11->display, window, PointerRoot, CurrentTime);
}

void x11_window_move(X11Base *x11, Window window, i32 x, i32 y) {
    XMoveWindow(x11->display, window, x, y);
}

void x11_window_hide(X11Base *x11, Window window) {
    x11_window_move(x11, window, -4000, 0);
}

void x11_window_set_border(X11Base *x11, Window window, i32 width, XColor color) {
    XSetWindowBorder(x11->display, window, color.pixel);
    XWindowChanges c = {};
    c.border_width = width;
    XConfigureWindow(x11->display, window, CWBorderWidth, &c);
}


XColor x11_add_color(X11Base *x11, u8 r, u8 g, u8 b) {
    size_t res = mf_vec_size(x11->colors);
    XColor *color = mf_vec_add(x11->colors);
    color->red = r * 255;
    color->green = g * 255;
    color->blue = b * 255;
    color->flags = DoRed | DoGreen | DoBlue;
    XAllocColor(x11->display, x11->colormap, color);
    return *color;
}

XColor x11_add_color(X11Base *x11, u32 color) {
    u8 r = (color >> 24) & 0xFF;
    u8 g = (color >> 16) & 0xFF;
    u8 b = (color >> 8) & 0xFF;
    XColor res = x11_add_color(x11, r, g, b);
    return res;
}

void x11_fill_rect(X11Base *x11, X11Window &window, i32 x, i32 y, u32 w, u32 h, XColor color) {
    XSetForeground(x11->display, x11->gc, color.pixel);
    XSetBackground(x11->display, x11->gc, color.pixel);
    XFillRectangle(x11->display, window.draw, x11->gc, x, y, w, h);
    XCopyArea(x11->display, window.draw, window.window, x11->gc, 0, 0, window.width, window.height, 0, 0);
}

void x11_draw_text(X11Base *x11, X11Window &window, i32 x, i32 y, const char *text, u64 n, XColor color) {
    XSetForeground(x11->display, x11->gc, color.pixel);
    XDrawString(x11->display, window.draw, x11->gc, x, y, text, n);
    XCopyArea(x11->display, window.draw, window.window, x11->gc, 0, 0, window.width, window.height, 0, 0);
}

void x11_get_window_name(X11Base *x11, Window window, char *data, u32 n) {
#if 0
    Atom atom_name = XInternAtom(state.x11.display, "_NET_WM_NAME", false);
    Atom type;
    int format;
    unsigned long n_read, n_open;
    unsigned char *data;
    XGetWindowProperty(state.x11.display,
                       window,
                       atom_name,
                       0,
                       sizeof(Atom),
                       false,
                       XA_ATOM,
                       &type,
                       &format,
                       &n_read,
                       &n_open,
                       &data);
    printf("data %s\n", data);
#else
    XTextProperty name;
    XGetTextProperty(x11->display, window, &name, XA_WM_NAME);
    data[0] = 0;
    if (name.nitems) {
        u32 size = MF_Min(name.nitems, n - 1);
        strncpy(data, (char *) name.value, size);
        data[size] = 0;
        XFree(name.value);
    }
#endif
}

i32 x11_get_text_width(X11Base *x11, const char *text, u32 n) {
    return XTextWidth(x11->font, text, n);
}
