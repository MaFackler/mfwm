
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
    
    // Get visual info
    XVisualInfo *infos;
    XRenderPictFormat *fmt;
    i32 n, i;
    XVisualInfo temp = {};
    temp.screen = x11->screen;
    temp.depth = 32;
    temp.c_class = TrueColor;

    u64 mask = VisualScreenMask | VisualDepthMask | VisualClassMask;
    infos = XGetVisualInfo(x11->display, mask, &temp, &n);

    for (i = 0; i < n; ++i) {
        fmt = XRenderFindVisualFormat(x11->display,
                                      infos[i].visual);
        if (fmt->type = PictTypeDirect && fmt->direct.alphaMask) {
            x11->visual = infos[i].visual;
            x11->depth = infos[i].depth;
            x11->colormap = XCreateColormap(x11->display, x11->root, x11->visual, AllocNone);
            x11->alpha_supported = true;
            LOG("SUPPORT ALPHA");
            break;
        }
    }

    if (!x11->alpha_supported) {
        x11->visual = XDefaultVisual(x11->display, x11->screen);
        x11->depth = XDefaultDepth(x11->display, x11->screen);
        x11->colormap = XDefaultColormap(x11->display, x11->screen);
    }

    XFree(infos);
    
    MF_Assert(x11->visual);
    MF_Assert(x11->depth);
    MF_Assert(x11->colormap);
    

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
    XCloseDisplay(x11->display);
}

void x11_window_grab_key(X11Base *x11, Window window, KeySym sym, u32 mod) {
    KeyCode code = XKeysymToKeycode(x11->display, sym);
    XGrabKey(x11->display, code, mod, window, 1, GrabModeAsync, GrabModeAsync); 
}

X11Window x11_window_create(X11Base *x11, u32 x, u32 y, u32 width, u32 height) {
    XSetWindowAttributes attribs = {};
    attribs.background_pixmap = ParentRelative;
    attribs.background_pixel = 0;
    attribs.border_pixel = 0;
    attribs.colormap = x11->colormap;
    attribs.event_mask = ButtonPressMask | ExposureMask;
    Window window = XCreateWindow(x11->display,
                                  x11->root,
                                  x, y,
                                  width,
                                  height,
                                  0,
                                  x11->depth,
                                  InputOutput,
                                  x11->visual,
                                  CWOverrideRedirect | CWBackPixel | CWBorderPixel | CWColormap |CWEventMask,
                                  &attribs);
    XMapWindow(x11->display, window);
    Drawable draw = XCreatePixmap(x11->display, window, width, height, x11->depth);
    GC gc = XCreateGC(x11->display, draw, 0, NULL);
    XSetFillStyle(x11->display, gc, FillSolid);
    X11Window res = {window, draw, gc, width, height};
    return res;
};

void x11_window_destroy(X11Base *x11, X11Window &window) {
    XFreeGC(x11->display, window.gc);
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
    size_t res = x11->colors.size();
    XColor &color = x11->colors.emplace_back();
    color.red = r * 255;
    color.green = g * 255;
    color.blue = b * 255;
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(x11->display, x11->colormap, &color);
    return color;
}

XColor x11_add_color(X11Base *x11, u32 color) {
    u8 r = (color >> 24) & 0xFF;
    u8 g = (color >> 16) & 0xFF;
    u8 b = (color >> 8) & 0xFF;
    XColor res = x11_add_color(x11, r, g, b);
    return res;
}

void x11_fill_rect(X11Base *x11, X11Window &window, i32 x, i32 y, u32 w, u32 h, XColor color) {
    XSetForeground(x11->display, window.gc, color.pixel);
    XSetBackground(x11->display, window.gc, color.pixel);
    XFillRectangle(x11->display, window.draw, window.gc, x, y, w, h);
    XCopyArea(x11->display, window.draw, window.window, window.gc, 0, 0, window.width, window.height, 0, 0);
}

void x11_draw_text(X11Base *x11, X11Window &window, i32 x, i32 y, const char *text, u64 n, XColor color) {
    XSetForeground(x11->display, window.gc, color.pixel);
    XDrawString(x11->display, window.draw, window.gc, x, y, text, n);
    XCopyArea(x11->display, window.draw, window.window, window.gc, 0, 0, window.width, window.height, 0, 0);
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
