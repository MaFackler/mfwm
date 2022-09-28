#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

#include "mfwm_wm.h"
#include <map>
#include <array>
#include <string>
#include <vector>

#include <mf.h>
#define MF_MATH_IMPLEMENTATION
#include <mf_math.h>
#include "mfwm.h"
#include "mfwm_log.h"
#include "mfwm_x11.h"

#include "mfwm_x11.cpp"
#include "mfwm_wm.cpp"

using std::array;
using std::map;
using std::string_view;
using std::vector;

struct State {
    bool running = true;
    // X11 Interface stuff
    X11Base x11;
    vector<X11Window> x11_statusbars;
    XColor color_debug;
    map<u32, XColor> colors;

    // Datastructures
    vector<Statusbar> statusbars;
    WindowManager wm;
};

inline std::string state_get_window_name(State *state, u32 window) {
    string res(256, '\0');
    x11_get_window_name(&state->x11, window, res.data(), res.size());
    res.resize(strlen(res.data()));
    return res;
}

static State state = {};
#include "mfwm_commands.cpp"

#include "../config.h"

#define LOG_FUNCTION(a, b, with_window)

void render();

XColor& get_color(u32 color) {
    return state.colors[color];
}

void window_register(u32 window) {
    XSelectInput(state.x11.display,
                 window,
                 EnterWindowMask | FocusChangeMask | PointerMotionMask);
    XMapWindow(state.x11.display, window);
}

void window_focus(u32 window) {
    x11_window_focus(&state.x11, window);
    x11_window_set_border(&state.x11,
                          window,
                          border_width_selected,
                          //state.color_button_window_bg,
                          get_color(colorschemes[ColorSchemeWindows].normal.bg));
}

void window_unfocus(u32 window) {
    x11_window_set_border(&state.x11, window, border_width_unselected,
                          get_color(colorschemes[ColorSchemeWindows].normal.bg));
}

void window_hide(u32 window) {
    x11_window_hide(&state.x11, window);
}

void do_layout(Rect *rect, const vector<u32> &windows) {
    X11Base *x11 = &state.x11;
    i32 amount_windows = windows.size();
    for (i32 i = 0; i < amount_windows; ++i) {
        u32 window = windows[i]; 
        string buf(256, '\0');
        x11_get_window_name(x11, window, &buf[0], buf.size());
        buf.resize(strlen(&buf[0]));
        LOGF("Window name is nr=%d id=%d-%s", i, window, buf);

        // Calculate
        i32 window_x = rect->x + GAP;
        i32 window_y = rect->y + STATUSBAR_HEIGHT + GAP;

        i32 amount_gaps = 2;
        if (i > 0 && amount_windows > 2) {
            amount_gaps += (amount_windows - 2);
        }
        float window_height = rect->h - (STATUSBAR_HEIGHT + (amount_gaps * GAP));
        i32 window_width = rect->w - 2 * GAP;
        if (amount_windows > 1) {
            window_width = (window_width - GAP) * 0.5;
            if (i > 0) {
                window_x += window_width + GAP;
                window_height /= (amount_windows - 1);
                //window_y += (window_height + GAP) * (i - 1);
                window_y += (window_height + GAP) * (i - 1);
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
    for (i32 i = 0; i < state.wm.monitors.size(); ++i) {
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
    string window_name = state_get_window_name(&state, event.window);
    window_manager_window_add(&state.wm, event.window, window_name.c_str());
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
    for (i32 i = 0; i < state.wm.monitors.size(); ++i) {

        Monitor *mon = &state.wm.monitors[i];
        X11Window statusbar = state.x11_statusbars[i];
        XColor color = get_color(colorschemes[ColorSchemeBar].normal.bg);
        if (i == state.wm.selected_monitor) {
            color = get_color(colorschemes[ColorSchemeBar].selected.bg);
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
        for (i32 i = 0; i < mon->tags.size(); ++i) {
            // TODO: this is stupid. just reverence to tags???...
            string_view text(mon->tags[i].name);
            i32 w = x11_get_text_width(&state.x11, text.data(), text.size());
            i32 w_total = w + x_margin * 2;

            XColor c = get_color(colorschemes[ColorSchemeTags].normal.bg);
            if (i == mon->selected_tag) {
                c = get_color(colorschemes[ColorSchemeTags].selected.bg);
            }

            x11_fill_rect(&state.x11, statusbar,
                          x, spacing, w_total, h, c);

            // TODO: Center Text
            x11_draw_text(&state.x11, statusbar, x + x_margin,
                          state.x11.font_height,
                          text.data(), text.size(),
                          get_color(colorschemes[ColorSchemeTags].normal.fg));

            x += w_total + spacing;
        }
        x += 100;
        LOG("tags drawn");

        Tag *tag = window_manager_monitor_get_selected_tag(&state.wm, mon);
        for (i32 i = 0; i < tag->window_names.size(); ++i) {
            string &text = tag->window_names[i];
            LOG("going to get text width");
            i32 w = x11_get_text_width(&state.x11, text.data(), text.size());
            i32 w_total = w + x_margin * 2;
            XColor c = get_color(colorschemes[ColorSchemeWindows].normal.bg);
            if (i == tag->selected_window) {
                c = get_color(colorschemes[ColorSchemeWindows].selected.bg);
            }

            x11_fill_rect(&state.x11, statusbar,
                          x, 0, w, w, c);

            // TODO: Center Text
            x11_draw_text(&state.x11, statusbar,
                          x + x_margin, state.x11.font_height,
                          text.data(), text.size(),
                          get_color(colorschemes[ColorSchemeWindows].normal.fg));
            x += w + spacing;
        }
        LOG("window buttons drawn");
    }
    LOG("dorender end");
    //XSync(state.x11.display, False);
}

int main() {
    log_init();
    MF_Assert(log_file);
    LOG("start");

    // X11
    x11_init(&state.x11);

    // TODO: What to do with alpha
    for (u32 i = 0; i < ColorSchemeCount; ++i) {
        ColorScheme scheme = colorschemes[i];
#define __add_color(c) \
        if (!state.colors.count(c)) \
            state.colors[c] = x11_add_color(&state.x11, c);

        __add_color(scheme.normal.fg);
        __add_color(scheme.normal.bg);
        __add_color(scheme.selected.fg);
        __add_color(scheme.selected.bg);
#undef __add_color

    }

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
        state.statusbars.emplace_back(statusbar);
        X11Window window = x11_window_create(&state.x11,
                                             statusbar.x, statusbar.y,
                                             statusbar.width, statusbar.height);
        state.x11_statusbars.push_back(window);
    }


    for(auto &it: state.wm.monitors) {
        for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
            window_manager_monitor_add_tag(&state.wm, &it, tags[i]);
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

    for (X11Window &window: state.x11_statusbars) {
        x11_window_destroy(&state.x11, window);
    }
    x11_shutdown(&state.x11);
}

