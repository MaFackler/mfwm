
void X11Base::init() {
    this->display = XOpenDisplay(NULL);
    this->screen = XDefaultScreen(this->display);
    this->root = XRootWindow(this->display, this->screen);

    u64 cursor = XCreateFontCursor(this->display, 2);

    {
        XSetWindowAttributes attribs = {};
        attribs.event_mask = SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask | EnterWindowMask | FocusChangeMask | PropertyChangeMask | PointerMotionMask | NoEventMask;
        attribs.cursor = cursor;
        XChangeWindowAttributes(this->display, this->root, CWEventMask | CWCursor, &attribs);
        XSelectInput(this->display, this->root, attribs.event_mask);
    }
    XSync(this->display, 0);
    this->depth = XDefaultDepth(this->display, this->screen);
    this->gc = XDefaultGC(this->display, this->screen);
    this->colormap = XDefaultColormap(this->display, this->screen);
    XSetFillStyle(this->display, this->gc, FillSolid);
}


X11Window X11Base::create_window(u32 width, u32 height) {
    XSetWindowAttributes attribs = {};
    attribs.background_pixel = ParentRelative;
    attribs.event_mask = ButtonPressMask | ExposureMask;
    Window window = XCreateWindow(this->display,
                                  this->root,
                                  0, 0,
                                  width,
                                  height,
                                  0,
                                  this->depth,
                                  CopyFromParent,
                                  XDefaultVisual(this->display, this->screen),
                                  CWEventMask | CWBackPixel,
                                  &attribs);
    XMapWindow(this->display, window);
    Drawable draw = XCreatePixmap(this->display, window, width, height, this->depth);
    X11Window res = {window, draw, width, height};
    return res;
};

void X11Base::destroy_window(X11Window &window) {
    XFreePixmap(this->display, window.draw);
    // TODO: window close?
}

X11Color X11Base::add_color(u8 r, u8 g, u8 b) {
    X11Color res = mf_vec_size(this->colors);
    XColor *color = mf_vec_add(this->colors);
    color->red = r * 255;
    color->green = g * 255;
    color->blue = b * 255;
    color->flags = DoRed | DoGreen | DoBlue;
    XAllocColor(this->display, this->colormap, color);
    return res;
}

void X11Base::fill_rect(X11Window &window, X11Color color) {
    XSetForeground(this->display, this->gc, this->colors[color].pixel);
    XSetBackground(this->display, this->gc, this->colors[color].pixel);
    XFillRectangle(this->display, window.draw, this->gc, 0, 0, window.width, window.height);
    XCopyArea(this->display, window.draw, window.window, this->gc, 0, 0, window.width, window.height, 0, 0);
}

