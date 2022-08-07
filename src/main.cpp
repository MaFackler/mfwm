#include <stdio.h>
#include <X11/Xlib.h>

#include <mf.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>
#include "mfwm_x11.h"
#include "mfwm_x11.cpp"


struct Statusbar {
    u32 width;
    u32 height;

    void init(u32 width, u32 height);
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

void Statusbar::init(u32 width, u32 height) {
    this->width = width;
    this->height = height;

}



void map_notify(XEvent &e) {
}

void map_request(XEvent &e) {
   XSelectInput(state.x11.display,
                e.xmap.window,
                EnterWindowMask | FocusChangeMask | PointerMotionMask);
   XMapWindow(state.x11.display, e.xmap.window);
   XSync(state.x11.display, 0);
}

void expose(XEvent &e) {
}

void button_press(XEvent &e) {
}

void key_press(XEvent &e) {
    state.running = false;
}

int main() {
    log_file = fopen("./mfwm.log", "w");
    state.x11.init();
    X11Color color_red = state.x11.add_color(255, 0, 0);
    X11Color color_green = state.x11.add_color(0, 255, 0);
    state.statusbar.init(800, 20);
    state.x11_statusbar = state.x11.create_window(800, 20);

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
                case CreateNotify: break;
                case MapNotify: map_notify(e); break;
                case NoExpose: break;
                case MapRequest: map_request(e); break;
                default: ERRORF("Unhandled event %d\n", e.type); break;

            }
        }
    }

    state.x11.destroy_window(state.x11_statusbar);
    XFreeGC(state.x11.display, state.x11.gc);
    XCloseDisplay(state.x11.display);
}

