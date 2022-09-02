#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>

#include <mf.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>
#include "mfwm_x11.h"
#include "mfwm_x11.cpp"

struct Rect {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
};

struct Statusbar {
    i32 width;
    i32 height;
};

struct State {
    bool running = true;
    // X11 Interface stuff
    X11Base x11;
    X11Window x11_statusbar;

    // Datastructures
    Statusbar statusbar;

    vec(u32) windows;
    vec(Rect) screens;
};


#define LOG(msg) fprintf(log_file, msg "\n"); fflush(log_file);
#define LOGF(msg, ...) fprintf(log_file, msg "\n", __VA_ARGS__); fflush(log_file);
#define ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
#define ERRORF(msg, ...) fprintf(stderr, msg, __VA_ARGS__); exit(EXIT_FAILURE);
static State state;
FILE *log_file = NULL;

#include "../config.h"

#define LOG_FUNCTION(a, b, with_window)

void log_event(const char *func, Window window) {
    char _buf[256] = {};
    if (window) {
        x11_get_window_name(&state.x11, window, &_buf[0], MF_ArrayLength(_buf));
    }
    LOGF("- EVENT - %s: %d-%s", func, window, &_buf[0])
}

bool has_window(vec(u32) windows, u32 window) {
    bool res = false;
    mf_vec_for(windows) {
        if (*it == window) {
            res = true;
            break;
        }
    }
    return res;
}

void screen_layout_windows(X11Base *x11, Rect *screen, vec(u32) windows) {
    mf_vec_for(windows) {
        XWindowChanges wc = {};
        wc.x = screen->x + GAP;
        wc.width = screen->w - 2 * GAP;
        wc.y = screen->y + STATUSBAR_HEIGHT + GAP;
        wc.height = screen->h - (STATUSBAR_HEIGHT + 2 * GAP);
        XConfigureWindow(x11->display,
                         *it,
                         CWX | CWY | CWWidth | CWHeight,
                         &wc);
    }
}

void map_notify(XEvent &e) {
    XMapEvent event = e.xmap;
    log_event(__func__, event.window);
}

void map_request(XEvent &e) {
    XMapRequestEvent event = e.xmaprequest;
    log_event(__func__, event.window);

    if (has_window(state.windows, event.window)) {
        screen_layout_windows(&state.x11, &state.screens[0], state.windows);
    }

    XSelectInput(state.x11.display,
                 event.window,
                 EnterWindowMask | FocusChangeMask | PointerMotionMask);
    XMapWindow(state.x11.display, event.window);
    XSync(state.x11.display, 0);
}

void unmap_request(XEvent &e) {
    XUnmapEvent event = e.xunmap;
    // NOTE: unmap request not able to print window name
    log_event(__func__, 0);

    i32 index = -1;
    for (i32 i = 0; i < mf_vec_size(state.windows); ++i) {
        if (event.window == state.windows[i]) {
            index = i;
            break;
        }
    }

    if (index >= 0) {
        auto size = mf_vec_size(state.windows);
        memmove(state.windows + index, state.windows + index + 1, size - index - 1);
    }
}

void configure_notify(XEvent &e) {
    XConfigureEvent event = e.xconfigure;
    log_event(__func__, event.window);
}

void configure_request(XEvent &e) {
    XWindowChanges wc = {};
    XConfigureRequestEvent event = e.xconfigurerequest;
    log_event(__func__, event.window);


    // NOTE: Forward request to XConfigureWindow and create
    // window with default properties
    wc.x = event.x;
    wc.y = event.y;
    wc.width = event.width;
    wc.height = event.height;
    wc.border_width = event.border_width;
    wc.sibling = event.above;
    wc.stack_mode = event.detail;

    XConfigureWindow(state.x11.display,
                     event.window,
                     event.value_mask,
                     &wc);
    XSync(state.x11.display, 0);

    mf_vec_push(state.windows, event.window);
}

void expose(XEvent &e) {
    XExposeEvent event = e.xexpose;
    log_event(__func__, event.window);
}

void button_press(XEvent &e) {
    XButtonPressedEvent event = e.xbutton;
    log_event(__func__, event.window);
}

void key_press(XEvent &e) {
    XKeyPressedEvent event = e.xkey;
    KeySym keysym = XKeycodeToKeysym(state.x11.display, event.keycode, 0);
    log_event(__func__, event.window);

    for (KeyDef &keydef: keybindings) {
        if (event.state == keydef.state && keysym == keydef.keysym) {
            keydef.action(keydef.arg);
        }
    }
}

int main() {
    log_file = fopen("./mfwm.log", "w");
    MF_Assert(log_file);
    LOGF("hey %d", 30);

    // X11
    x11_init(&state.x11);
    X11Color color_red = x11_add_color(&state.x11, 255, 0, 0);
    X11Color color_green = x11_add_color(&state.x11, 0, 255, 0);

    bool isActive = XineramaIsActive(state.x11.display);
    i32 num_screens = 0;
    XineramaScreenInfo *info = XineramaQueryScreens(state.x11.display, &num_screens);

    for (i32 i = 0; i < num_screens; ++i) {
        mf_vec_push(state.screens, ((Rect) {
            info[i].x_org,
            info[i].y_org,
            info[i].width,
            info[i].height,
        }));
    }
    MF_Assert(mf_vec_size(state.screens) == 1);
    printf("Got screen %d+%d %dx%d\n", state.screens[0].x, state.screens[0].y, state.screens[0].w, state.screens[0].h);

    // Setup datastructures
    state.statusbar = {state.screens[0].w, STATUSBAR_HEIGHT};
    state.x11_statusbar = x11_create_window(&state.x11, state.statusbar.width, state.statusbar.height);

    for (KeyDef &def: keybindings) {
        x11_window_grab_key(&state.x11, state.x11.root, def.keysym, def.state);
    }


    while (state.running) {
        if (XPending(state.x11.display) > 0) {
            XEvent e;
            XNextEvent(state.x11.display, &e);

            switch (e.type) {
                case ButtonPress: button_press(e); break;
                case KeyPress: key_press(e); break;
                case KeyRelease: break;
                case UnmapNotify: unmap_request(e); break;

                case NoExpose: break;

                // These events are in oreder of the event chain
                case CreateNotify: break;
                case ConfigureRequest: configure_request(e); break;
                case ConfigureNotify: configure_notify(e); break;
                case MapRequest: map_request(e); break;
                case MapNotify: map_notify(e); break;
                case Expose: {
                    x11_fill_rect(&state.x11, state.x11_statusbar, color_green);
                } break;
                // --------

                case MotionNotify: break;
                case EnterNotify: break;
                case DestroyNotify: break;
                case PropertyNotify: break;

                case FocusIn: break;
                case FocusOut: break;
                case ClientMessage: break;
                default: ERRORF("Unhandled event %d\n", e.type); break;

            }
        }
    }

    fclose(log_file);
    x11_destroy_window(&state.x11, state.x11_statusbar);
    x11_shutdown(&state.x11);
}

