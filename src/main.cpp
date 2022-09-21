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

#define LOG(msg) fprintf(log_file, msg "\n"); fflush(log_file);
#define LOGF(msg, ...) fprintf(log_file, msg "\n", __VA_ARGS__); fflush(log_file);
#define ERROR(msg) fprintf(stderr, msg); exit(EXIT_FAILURE);
#define ERRORF(msg, ...) fprintf(stderr, msg, __VA_ARGS__); exit(EXIT_FAILURE);
FILE *log_file = NULL;

#include "mfwm_wm.cpp"



struct Statusbar {
    i32 x;
    i32 y;
    i32 width;
    i32 height;
};



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

mf_str state_get_window_name(State *state, u32 window) {
    mf_str s = mf_str_new(256);
    x11_get_window_name(&state->x11, window, s, mf_str_capacity(s));
    mf__str_get_header(s)->size = strlen(s);
    return s;
}



void state_delete_window(State *state, u32 window) {
}


static State state = {};
#include "mfwm_commands.cpp"

#include "../config.h"

#define LOG_FUNCTION(a, b, with_window)

void render();

void window_register(u32 window) {
    XSelectInput(state.x11.display,
                 window,
                 EnterWindowMask | FocusChangeMask | PointerMotionMask);
    XMapWindow(state.x11.display, window);
}

void window_focus(u32 window) {
    x11_window_focus(&state.x11, window);
    x11_window_set_border(&state.x11, window, 3, state.color_button_window_bg);
}

void window_unfocus(u32 window) {
    x11_window_set_border(&state.x11, window, 0, state.color_button_selected_window_bg);
}

void window_hide(u32 window) {
    x11_window_hide(&state.x11, window);
}

void do_layout(Rect *rect, vec(u32) windows) {
    X11Base *x11 = &state.x11;
    i32 amount_windows = mf_vec_size(windows);
    for (i32 i = 0; i < amount_windows; ++i) {
        u32 window = windows[i]; 
        // TODO: why does mf_str not work bug???!
        //mf_str buf = mf_str_new(256);
        char buf[256] = {};
        x11_get_window_name(x11, window, buf, 256);
        mf__str_get_header(buf)->size = strlen(buf);
        LOGF("Window name is nr=%d id=%d-%s", i, window, buf);
        //mf_str_free(buf);

        // Calculate
        i32 window_x = rect->x + GAP;
        i32 window_y = rect->y + STATUSBAR_HEIGHT + GAP;

        i32 window_height = rect->h - (STATUSBAR_HEIGHT + 2 * GAP);
        i32 window_width = rect->w - 2 * GAP;
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

void log_event(const char *func, Window window) {
    char _buf[256] = {};
    if (window) {
        x11_get_window_name(&state.x11, window, &_buf[0], MF_ArrayLength(_buf));
    }
    LOGF("- EVENT - %s: %d-%s", func, window, &_buf[0])
}


void map_notify(XEvent &e) {
    XMapEvent event = e.xmap;
    log_event(__func__, event.window);
    render();
}

void focus_in(XEvent &e) {
    XFocusChangeEvent event = e.xfocus;
    log_event(__func__, event.window);
    //MF_Assert(!"INVALID");
}

void motion_notify(XEvent &e) {
    XMotionEvent event = e.xmotion;
    i32 x = event.x_root;
    i32 y = event.y_root;
    log_event(__func__, event.window);

    if (event.window != state.x11.root) {
        return;
    }
    for (i32 i = 0; i < mf_vec_size(state.wm.monitors); ++i) {
        Monitor *it = &state.wm.monitors[i];
        if (x >= it->rect.x && x < it->rect.x + it->rect.w) {
            if (i != state.wm.selected_monitor) {
                window_manager_select_monitor(&state.wm, i);
                render();
            }
            break;
        }
    }

    LOG("motion notify end");
}

void enter_notify(XEvent &e) {
    XCrossingEvent event = e.xcrossing;
    log_event(__func__, event.window);

    // TODO: window manager should also take care of root
    if (event.window != state.x11.root) {
        window_manager_window_focus(&state.wm, event.window);
    }

    x11_window_focus(&state.x11, event.window);
}

void map_request(XEvent &e) {
    XMapRequestEvent event = e.xmaprequest;
    log_event(__func__, event.window);
    mf_str window_name = state_get_window_name(&state, event.window);
    window_manager_window_add(&state.wm, event.window, window_name);
    XSync(state.x11.display, 0);
}

void unmap_request(XEvent &e) {
    XUnmapEvent event = e.xunmap;
    // NOTE: unmap request not able to print window name
    log_event(__func__, 0);
    LOGF("umap_request window %d", event.window);
    window_manager_window_delete(&state.wm, event.window);
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

    LOG("do render");
    for (i32 i = 0; i < mf_vec_size(state.wm.monitors); ++i) {

        Monitor *mon = &state.wm.monitors[i];
        X11Window statusbar = state.x11_statusbars[i];
        auto color = state.color_bar_bg;
        if (i == state.wm.selected_monitor) {
            color = state.color_debug;
        }
        x11_fill_rect(&state.x11, statusbar,
                      0, 0,
                      statusbar.width, statusbar.height,
                      color);
        LOG("statusbar drawn");


        i32 x_margin = 5;
        i32 spacing = 1;
        i32 x = spacing;
        i32 h = statusbar.height - 2 * spacing;
        for (i32 i = 0; i < mf_vec_size(mon->tags); ++i) {
            mf_strview text = S(mon->tags[i].name);
            i32 w = x11_get_text_width(&state.x11, text.data, text.size);
            i32 w_total = w + x_margin * 2;

            XColor c = state.color_button_tag_bg;
            if (i == mon->selected_tag) {
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
        LOG("tags drawn");

        Tag *tag = window_manager_monitor_get_selected_tag(&state.wm, mon);
        for (i32 i = 0; i < mf_vec_size(tag->window_names); ++i) {
            const char *text = tag->window_names[i];
            LOG("going to get text width");
            i32 w = x11_get_text_width(&state.x11, text, strlen(text));
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
                          text, strlen(text), state.color_button_window_fg);
            x += w + spacing;
        }
        LOG("window buttons drawn");
    }
    LOG("dorender end");
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
    for (i32 i = 0; i < num_screens; ++i) {


        Rect rect = {
            info[i].x_org,
            info[i].y_org,
            info[i].width,
            info[i].height,
        };
        LOGF("Got scren number %d (%d+%d %dx%d)", info[i].screen_number, rect.x, rect.y, rect.w, rect.h);
        window_manager_add_monitor(&state.wm, rect);
        Statusbar statusbar = {rect.x, rect.y, rect.w, STATUSBAR_HEIGHT};
        mf_vec_push(state.statusbars, statusbar);
        X11Window window = x11_window_create(&state.x11,
                                             statusbar.x, statusbar.y,
                                             statusbar.width, statusbar.height);
        mf_vec_push(state.x11_statusbars, window);
    }


    mf_vec_for(state.wm.monitors) {
        for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
            window_manager_monitor_add_tag(&state.wm, it, tags[i]);
        }
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

    state.wm.api.window_register = window_register;
    state.wm.api.window_focus = window_focus;
    state.wm.api.window_unfocus = window_unfocus;
    state.wm.api.window_hide = window_hide;
    state.wm.api.do_layout = do_layout;

    while (state.running) {
        if (XPending(state.x11.display) > 0) {
            XEvent e;
            XNextEvent(state.x11.display, &e);

            if (e.type != 6) {
                //LOGF("GOT EVENT %d", e.type);
            }
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
                //
                case FocusIn: focus_in(e); break;
                case MotionNotify: motion_notify(e); break;

                case EnterNotify: enter_notify(e); break;
                case DestroyNotify: break;
                case PropertyNotify: break;

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

