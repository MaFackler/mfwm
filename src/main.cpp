#include <stdio.h>
#include <X11/Xlib.h>

#include <mf.h>

static bool running = true;
FILE *log_file = NULL;
static Display *display;

#define LOG(msg) fprintf(log_file, msg "\n");
#define LOGF(msg, ...) fprintf(log_file, msg "\n", __VA_ARGS__);
#define ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
#define ERRORF(msg, ...) fprintf(stderr, msg, __VA_ARGS__); exit(EXIT_FAILURE);

void map_notify(XEvent &e) {
}

void map_request(XEvent &e) {
   XSelectInput(display,
                e.xmap.window,
                EnterWindowMask | FocusChangeMask | PointerMotionMask);
   XMapWindow(display, e.xmap.window);
   XSync(display, 0);
}

void expose(XEvent &e) {
}

void button_press(XEvent &e) {
}

void key_press(XEvent &e) {
    running = false;
}

int main() {
    log_file = fopen("./mfwm.log", "w");

    Window root;
    i32 screen;
    display = XOpenDisplay(NULL);
    screen = XDefaultScreen(display);
    root = XRootWindow(display, screen);

    u64 cursor = XCreateFontCursor(display, 2);

    {
        XSetWindowAttributes attribs = {};
        attribs.event_mask = SubstructureNotifyMask | SubstructureRedirectMask | KeyPressMask | EnterWindowMask | FocusChangeMask | PropertyChangeMask | PointerMotionMask | NoEventMask;
        attribs.cursor = cursor;
        XChangeWindowAttributes(display, root, CWEventMask | CWCursor, &attribs);
        XSelectInput(display, root, attribs.event_mask);
    }
    XSync(display, 0);

    // Init draw
    // TODO: get actual size
    //constexpr u32 width = 800;
    //constexpr u32 height = 600;

    // Bar
    u32 depth = XDefaultDepth(display, screen);
    constexpr u32 bar_width = 800;
    constexpr u32 bar_height = 100;
    Window statusbar;
    XSetWindowAttributes attribs = {};
    attribs.background_pixel = ParentRelative;
    attribs.event_mask = ButtonPressMask | ExposureMask;
    statusbar = XCreateWindow(display,
                              root,
                              0, 0,
                              bar_width,
                              bar_height,
                              0,
                              depth,
                              CopyFromParent,
                              XDefaultVisual(display, screen),
                              CWEventMask | CWBackPixel,
                              &attribs);
    XMapWindow(display, statusbar);

    // bardraw
    Drawable draw = XCreatePixmap(display, statusbar, bar_width, bar_height, depth);
    //GC gc = XCreateGC(display, statusbar, 0, NULL);
    GC gc = XDefaultGC(display, screen);
    XSetFillStyle(display, gc, FillSolid);

    Colormap colormap = {};
    colormap = XDefaultColormap(display, screen);
    XColor color = {};
    color.red = 255 * 255;
    color.green = 0 * 255;
    color.blue = 255 * 255;
    color.flags = DoRed | DoGreen | DoBlue;
    i32 res = XAllocColor(display, colormap, &color);
    if (res == 0) {
        ERROR("Unable to alloc color");
    }



    while (running) {
        if (XPending(display) > 0) {
            XEvent e;
            XNextEvent(display, &e);

            switch (e.type) {
                case Expose: {
                        XSetForeground(display, gc, color.pixel);
                        XSetBackground(display, gc, color.pixel);
                        XFillRectangle(display, draw, gc, 0, 0, bar_width, bar_height);
                        XCopyArea(display, draw, statusbar, gc, 0, 0, bar_width, bar_height, 0, 0);
                    } break;
                case ButtonPress: button_press(e); break;
                case KeyPress: key_press(e); break;
                case CreateNotify: break;
                case MapNotify: map_notify(e); break;
                case NoExpose: break;
                case MapRequest: map_request(e); break;
                default: ERRORF("Unhandled event %d\n", e.type); break;

            }

        }
        ////XFillRectangle(display, draw, gc, 0, 0, bar_width, bar_height);
        ////XCopyArea(display, draw, root, gc, 0, 0, bar_width, bar_height, 0, 0);
        ////XFlush(display);
        ////XSync(display, 0);
    }

    XFreePixmap(display, draw);
    XFreeGC(display, gc);
    XCloseDisplay(display);
}

