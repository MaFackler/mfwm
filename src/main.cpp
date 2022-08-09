#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <mf.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>
#include "mfwm_x11.h"
#include "mfwm_x11.cpp"


struct Statusbar {
    u32 width;
    u32 height;
};

struct State {
    bool running = true;
    // X11 Interface stuff
    X11Base x11;
    X11Window x11_statusbar;

    // Datastructures
    Statusbar statusbar;
};

#define LOG(msg) fprintf(log_file, msg "\n");
#define LOGF(msg, ...) fprintf(log_file, msg "\n", __VA_ARGS__);
#define ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
#define ERRORF(msg, ...) fprintf(stderr, msg, __VA_ARGS__); exit(EXIT_FAILURE);
static State state;
FILE *log_file = NULL;

void run() {
    if (fork() == 0) {
        char * l[] = {(char *) "dmenu_run", NULL};
        setsid();
        execvp("dmenu_run", l);
    }

}


void map_notify(XEvent &e) {
    LOG("map_notify");
}

void map_request(XEvent &e) {
    XSelectInput(state.x11.display,
                e.xmap.window,
                EnterWindowMask | FocusChangeMask | PointerMotionMask);
    XMapWindow(state.x11.display, e.xmap.window);
    XSync(state.x11.display, 0);
    LOG("map_request");
}

void configure_notify(XEvent &e) {
}

void configure_request(XEvent &e) {
    XWindowChanges changes = {};
    auto event = e.xconfigurerequest;

    changes.x = event.x;
    changes.y = event.y;
    changes.width = event.width;
    changes.height = event.height;
    XConfigureWindow(state.x11.display,
                     event.window,
                     event.value_mask,
                     &changes);
    XSync(state.x11.display, 0);
}

void expose(XEvent &e) {
    LOG("expose");
}

void button_press(XEvent &e) {
}

void key_press(XEvent &e) {
    LOG("key_press");
    KeySym keysym = XKeycodeToKeysym(state.x11.display, e.xkey.keycode, 0);
    LOGF("keypreess %d", e.xkey.keycode);

    if (e.xkey.state == Mod1Mask && keysym == XK_q) {
        state.running = false;
    }

    if (e.xkey.state == Mod1Mask && keysym == XK_p) {
        LOG("start process");
        run();
    }
#if 0
    i32 keysym = 0;
    KeySym *k = XGetKeyboardMapping(state.x11.display,
                                    e.xkey.keycode,
                                    1,
                                    &keysym);

    if (k[keysym] == XK_q) {
        state.running = false;
    }
    XFree(k);
#endif

    //run();
}

int main() {
    log_file = fopen("./mfwm.log", "w");
    MF_Assert(log_file);
    LOGF("hey %d", 30);

    // Setup datastructures
    state.statusbar = {800, 20};

    // X11
    state.x11.init();
    X11Color color_red = state.x11.add_color(255, 0, 0);
    X11Color color_green = state.x11.add_color(0, 255, 0);
    //state.x11_statusbar = state.x11.create_window(800, 20);

    while (state.running) {
        if (XPending(state.x11.display) > 0) {
            XEvent e;
            XNextEvent(state.x11.display, &e);

            switch (e.type) {
                case Expose: {
                    state.x11.fill_rect(state.x11_statusbar, color_green);
                } break;
                case ButtonPress: button_press(e); break;
                case KeyPress: key_press(e); break;
                case MapNotify: map_notify(e); break;
                case MapRequest: map_request(e); break;
                case ConfigureNotify: configure_notify(e); break;
                case ConfigureRequest: configure_request(e); break;

                case NoExpose: break;

                case UnmapNotify: break;
                case MotionNotify: break;
                case EnterNotify: break;
                case CreateNotify: break;
                case DestroyNotify: break;
                case PropertyNotify: break;

                case FocusIn: break;
                case FocusOut: break;
                default: ERRORF("Unhandled event %d\n", e.type); break;

            }
        }
    }

    fclose(log_file);
    state.x11.destroy_window(state.x11_statusbar);
    state.x11.shutdown();
}

