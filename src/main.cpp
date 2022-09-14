#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

#include "mfwm_wm.h"

#include <mf.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>
#define MF_STRING_IMPLEMENTATION
#include <mf_string.h>
#define MF_MATH_IMPLEMENTATION
#include <mf_math.h>
#include "mfwm_x11.h"
#include "mfwm_x11.cpp"

#include "mfwm_wm.cpp"



struct Statusbar {
    i32 x;
    i32 y;
    i32 width;
    i32 height;
};



void screen_layout_windows(X11Base *x11, Rect *screen, vec(u32) windows);

struct State {
    bool running = true;
    // X11 Interface stuff
    X11Base x11;


    vec(X11Window) x11_statusbars;

    XColor color_debug;
    XColor color_bar_bg;
    XColor color_button_tag_bg;
    XColor color_button_selected_tag_bg;
    XColor color_button_tag_fg;

    XColor color_button_window_bg;
    XColor color_button_selected_window_bg;
    XColor color_button_window_fg;

    // Datastructures
    vec(Statusbar) statusbars;

    WindowManager wm;
};


void state_push_window(State *state, u32 window) {
    Tag *tag = window_manager_get_current_tag(&state->wm);
    mf_str s = mf_str_new(256);
    x11_get_window_name(&state->x11, window, s, mf_str_capacity(s));
    // TODO: this is bad
    mf__str_get_header(s)->size = strlen(s);
    tag_add_window(tag, window, s);
}

void state_delete_window(State *state, u32 window) {
}

void state_select_tag(State *state, u32 tag_index) {
    MF_Assert(tag_index < mf_vec_size(state->wm.tags));
    Tag *tag = window_manager_get_current_tag(&state->wm);
    mf_vec_for(tag->windows) {
        x11_window_hide(&state->x11, *it);
    }
    state->wm.selected_tag = tag_index;

    tag = window_manager_get_current_tag(&state->wm);
    screen_layout_windows(&state->x11, &state->wm.screens[0], tag->windows);

    if (tag_has_windows(tag)) {
        x11_window_focus(&state->x11, tag_get_selected_window(tag));
    }
}


#define LOG(msg) fprintf(log_file, msg "\n"); fflush(log_file);
#define LOGF(msg, ...) fprintf(log_file, msg "\n", __VA_ARGS__); fflush(log_file);
#define ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
#define ERRORF(msg, ...) fprintf(stderr, msg, __VA_ARGS__); exit(EXIT_FAILURE);
static State state = {};
FILE *log_file = NULL;

#include "../config.h"

#define LOG_FUNCTION(a, b, with_window)

void render();

void log_event(const char *func, Window window) {
    char _buf[256] = {};
    if (window) {
        x11_get_window_name(&state.x11, window, &_buf[0], MF_ArrayLength(_buf));
    }
    LOGF("- EVENT - %s: %d-%s", func, window, &_buf[0])
}

void screen_layout_windows(X11Base *x11, Rect *screen, vec(u32) windows) {

    i32 amount_windows = mf_vec_size(windows);
    for (i32 i = 0; i < amount_windows; ++i) {
        u32 window = windows[i]; 
        // TODO: stack string/buffer
        mf_str buf = mf_str_new(256);
        x11_get_window_name(x11, window, buf, mf_str_capacity(buf));
        mf__str_get_header(buf)->size = strlen(buf);
        LOGF("Window name is nr=%d id=%d-%s", i, window, buf);
        mf_str_free(buf);

        // Calculate
        i32 window_x = screen->x + GAP;
        i32 window_y = screen->y + STATUSBAR_HEIGHT + GAP;

        i32 window_height = screen->h - (STATUSBAR_HEIGHT + 2 * GAP);
        i32 window_width = screen->w - 2 * GAP;
        if (amount_windows > 1) {
            window_width = (window_width - GAP) * 0.5;
            if (i > 0) {
                window_x += window_width + GAP;
                window_height /= (amount_windows - 1);
                window_y += window_height * (i - 1);
            }
        }

        // Configure X Window
        XWindowChanges wc = {};
        wc.x = window_x;
        wc.width = window_width;
        wc.y = window_y;
        wc.height = window_height;
        XConfigureWindow(x11->display,
                         window,
                         CWX | CWY | CWWidth | CWHeight,
                         &wc);
    }
}

void map_notify(XEvent &e) {
    XMapEvent event = e.xmap;
    log_event(__func__, event.window);
    render();
}

void enter_notify(XEvent &e) {
    XCrossingEvent event = e.xcrossing;

    // TODO: window manager should also take care of root
    if (event.window != state.x11.root) {
        Tag *tag = window_manager_get_current_tag(&state.wm);
        if (tag_has_windows(tag)) {
            u32 old_window = tag_get_selected_window(tag);
            x11_window_set_border(&state.x11, old_window, 1, state.color_button_window_bg);
        }

        tag_select_window(tag, event.window);
        x11_window_set_border(&state.x11, event.window, 3, state.color_button_selected_window_bg);
    }

    x11_window_focus(&state.x11, event.window);
}

void map_request(XEvent &e) {
    XMapRequestEvent event = e.xmaprequest;
    log_event(__func__, event.window);

    Tag *tag = window_manager_get_current_tag(&state.wm);

    // Unfocus old window and push new window
    if (mf_vec_index(tag->windows, (u32) event.window) == -1) {
        if (tag_has_windows(tag)) {
            u32 old_window = tag_get_selected_window(tag);
            x11_window_set_border(&state.x11, old_window, 1, state.color_button_window_bg);
        }

        state_push_window(&state, event.window);
    }

    // Releayet windows and register it to x11
    if (mf_vec_index(tag->windows, (u32) event.window) >= 0) {
        screen_layout_windows(&state.x11, &state.wm.screens[0], tag->windows);
    }
    XSelectInput(state.x11.display,
                 event.window,
                 EnterWindowMask | FocusChangeMask | PointerMotionMask);
    XMapWindow(state.x11.display, event.window);

    // Focus new window
    x11_window_focus(&state.x11, event.window);
    x11_window_set_border(&state.x11, event.window, 3, state.color_button_selected_window_bg);

    XSync(state.x11.display, 0);
}

void unmap_request(XEvent &e) {
    XUnmapEvent event = e.xunmap;
    // NOTE: unmap request not able to print window name
    log_event(__func__, 0);

    LOGF("umap_request window %d", event.window);
    Tag *tag = window_manager_get_current_tag(&state.wm);
    i32 index = mf_vec_index(tag->windows, (u32) event.window);
    if (index >= 0) {
        mf_vec_delete(tag->windows, index);
        mf_str *s = mf_vec_delete(tag->window_names, index);
        mf_str_free(*s);
        screen_layout_windows(&state.x11, &state.wm.screens[0], tag->windows);
        tag->selected_window = MF_Min(index, mf_vec_size(tag->windows) - 1);
        if (tag_has_windows(tag)) {
            x11_window_focus(&state.x11, tag_get_selected_window(tag));
        }
    }

    render();
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
            render();
        }
    }
}

void render() {

    for (i32 i = 0; i < mf_vec_size(state.wm.screens); ++i) {

        Rect *screen = &state.wm.screens[i];
        X11Window statusbar = state.x11_statusbars[i];
        x11_fill_rect(&state.x11, statusbar,
                      0, 0,
                      statusbar.width, statusbar.height,
                      state.color_bar_bg);


        i32 x_margin = 5;
        i32 spacing = 1;
        i32 x = spacing;
        i32 h = statusbar.height - 2 * spacing;
        for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
            mf_strview text = S(tags[i]);
            i32 w = x11_get_text_width(&state.x11, text.data, text.size);
            i32 w_total = w + x_margin * 2;

            XColor c = state.color_button_tag_bg;
            if (i == state.wm.selected_tag) {
                c = state.color_button_selected_tag_bg;
            }

            x11_fill_rect(&state.x11, statusbar,
                          x, spacing, w_total, h, c);

            // TODO: Center Text
            x11_draw_text(&state.x11, statusbar, x + x_margin,
                          state.x11.font_height,
                          text.data, text.size, state.color_button_tag_fg);

            x += w_total + spacing;
        }
        x += 100;

        Tag *tag = window_manager_get_current_tag(&state.wm);
        for (i32 i = 0; i < mf_vec_size(tag->window_names); ++i) {
            mf_str text = tag->window_names[i];
            i32 w = x11_get_text_width(&state.x11, text, mf_str_size(text));
            i32 w_total = w + x_margin * 2;
            XColor c = state.color_button_window_bg;
            if (i == tag->selected_window) {
                c = state.color_button_selected_window_bg;
            }

            x11_fill_rect(&state.x11, statusbar,
                          x, 0, w, w, c);

            // TODO: Center Text
            x11_draw_text(&state.x11, statusbar,
                          x + x_margin, state.x11.font_height,
                          text, mf_str_size(text), state.color_button_window_fg);
            x += w + spacing;
        }
    }
    //XSync(state.x11.display, False);
}

int main() {
    log_file = fopen("./mfwm.log", "w");
    MF_Assert(log_file);
    LOG("start");

    // X11
    x11_init(&state.x11);

    // TODO: What to do with alpha
    state.color_debug = x11_add_color(&state.x11, 0xFF000000);
    state.color_bar_bg = x11_add_color(&state.x11, 0x28282800);

    state.color_button_tag_bg = x11_add_color(&state.x11, color_schemes[ColorSchemeTags][0].bg);
    state.color_button_tag_fg = x11_add_color(&state.x11, color_schemes[ColorSchemeTags][0].fg);
    state.color_button_selected_tag_bg = x11_add_color(&state.x11, color_schemes[ColorSchemeTags][1].bg);

    state.color_button_window_bg = x11_add_color(&state.x11, color_schemes[ColorSchemeWindows][0].bg);
    state.color_button_window_fg = x11_add_color(&state.x11, color_schemes[ColorSchemeWindows][0].fg);
    state.color_button_selected_window_bg = x11_add_color(&state.x11, color_schemes[ColorSchemeWindows][1].bg);

    bool isActive = XineramaIsActive(state.x11.display);
    i32 num_screens = 0;
    XineramaScreenInfo *info = XineramaQueryScreens(state.x11.display, &num_screens);

    // Setup datastructures
    for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
        Tag *tag = mf_vec_add(state.wm.tags);
        tag->name = tags[i];
        tag->window_names = NULL;
        tag->windows = NULL;
    }

    for (i32 i = 0; i < num_screens; ++i) {

        LOGF("Got scren number %d", info[i].screen_number);

        Rect screen = {
            info[i].x_org,
            info[i].y_org,
            info[i].width,
            info[i].height,
        };
        mf_vec_push(state.wm.screens, screen);
        Statusbar statusbar = {screen.x, screen.y, screen.w, STATUSBAR_HEIGHT};
        mf_vec_push(state.statusbars, statusbar);
        X11Window window = x11_window_create(&state.x11,
                                             statusbar.x, statusbar.y,
                                             statusbar.width, statusbar.height);
        mf_vec_push(state.x11_statusbars, window);
    }



    for (KeyDef &def: keybindings) {
        x11_window_grab_key(&state.x11, state.x11.root, def.keysym, def.state);
    }

    LOG("before autostart\n");
    for (i32 i = 0; i < MF_ArrayLength(startup_commands); ++i) {
        const char *cmd = startup_commands[i];
        run_sync(Arg{cmd});
    }
    LOG("after autostart\n");

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

                // These events are in order of the event chain
                case CreateNotify: break;
                case ConfigureRequest: configure_request(e); break;
                case ConfigureNotify: configure_notify(e); break;
                case MapRequest: map_request(e); break;
                case MapNotify: map_notify(e); break;
                case Expose: render(); break;
                // --------

                case MotionNotify: break;
                case EnterNotify: enter_notify(e); break;
                case DestroyNotify: break;
                case PropertyNotify: break;

                case FocusIn: break;
                case FocusOut: break;
                case ClientMessage: break;
                case MappingNotify: break;
                default: ERRORF("Unhandled event %d\n", e.type); break;

            }
        }
    }

    fclose(log_file);

    mf_vec_for(state.x11_statusbars) {
        x11_window_destroy(&state.x11, *it);
    }
    x11_shutdown(&state.x11);
}

