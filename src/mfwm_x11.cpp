
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
}

void x11_shutdown(X11Base *x11) {
    XFreeGC(x11->display, x11->gc);
    XCloseDisplay(x11->display);
}

void x11_window_grab_key(X11Base *x11, Window window, KeySym sym, u32 mod) {
    KeyCode code = XKeysymToKeycode(x11->display, sym);
    XGrabKey(x11->display, code, mod, window, 1, GrabModeAsync, GrabModeAsync); 
}

X11Window x11_create_window(X11Base *x11, u32 width, u32 height) {
    XSetWindowAttributes attribs = {};
    attribs.background_pixel = ParentRelative;
    attribs.event_mask = ButtonPressMask | ExposureMask;
    Window window = XCreateWindow(x11->display,
                                  x11->root,
                                  0, 0,
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

void x11_destroy_window(X11Base *x11, X11Window &window) {
    XFreePixmap(x11->display, window.draw);
}

X11Color x11_add_color(X11Base *x11, u8 r, u8 g, u8 b) {
    X11Color res = mf_vec_size(x11->colors);
    XColor *color = mf_vec_add(x11->colors);
    color->red = r * 255;
    color->green = g * 255;
    color->blue = b * 255;
    color->flags = DoRed | DoGreen | DoBlue;
    XAllocColor(x11->display, x11->colormap, color);
    return res;
}

void x11_fill_rect(X11Base *x11, X11Window &window, X11Color color) {
    XSetForeground(x11->display, x11->gc, x11->colors[color].pixel);
    XSetBackground(x11->display, x11->gc, x11->colors[color].pixel);
    XFillRectangle(x11->display, window.draw, x11->gc, 0, 0, window.width, window.height);
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
