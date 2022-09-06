#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

#include <mf.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>
#define MF_STRING_IMPLEMENTATION
#include <mf_string.h>
#define MF_MATH_IMPLEMENTATION
#include <mf_math.h>
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

struct Tag {
    const char *name;
    i32 selected_window = 0;
    vec(u32) windows = NULL;
    vec(mf_str) window_names = NULL;
};

bool tag_has_windows(Tag *tag) {
    return mf_vec_size(tag->windows) > 0;
}

u32 tag_get_selected_window(Tag *tag) {
    return tag->windows[tag->selected_window];
}

void screen_layout_windows(X11Base *x11, Rect *screen, vec(u32) windows);

struct State {
    bool running = true;
    // X11 Interface stuff
    X11Base x11;
    X11Window x11_statusbar;

    X11Color color_bar_bg;
    X11Color color_button_tag_bg;
    X11Color color_button_selected_tag_bg;
    X11Color color_button_tag_fg;

    X11Color color_button_window_bg;
    X11Color color_button_selected_window_bg;
    X11Color color_button_window_fg;

    // Datastructures
    Statusbar statusbar;

    i32 selected_tag = 0;

    vec(Tag) tags = NULL;

    vec(Rect) screens = NULL;

};

inline Tag* state_get_current_tag(State *state) {
    return &state->tags[state->selected_tag];    
}

void state_push_window(State *state, u32 window) {
    Tag *tag = &state->tags[state->selected_tag];
    u32 index = mf_vec_push(tag->windows, window);
    mf_str s = mf_str_new(256);
    x11_get_window_name(&state->x11, window, s.data, s.capacity);
    s.size = strlen(s.data);
    mf_vec_push(tag->window_names, s);

    tag->selected_window = index;
}

void state_delete_window(State *state, u32 window) {
}

void state_select_tag(State *state, u32 tag_index) {
    assert(tag_index < mf_vec_size(state->tags));
    Tag *tag = state_get_current_tag(state);
    mf_vec_for(tag->windows) {
        x11_window_hide(&state->x11, *it);
    }
    state->selected_tag = tag_index;

    tag = state_get_current_tag(state);
    screen_layout_windows(&state->x11, &state->screens[0], tag->windows);

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
        mf_str buf = mf_str_stack(256);
        x11_get_window_name(x11, window, buf.data, buf.capacity);
        buf.size = strlen(buf.data);
        LOGF("Window name is nr=%d id=%d-%s", i, window, buf.data);

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
    x11_window_focus(&state.x11, event.window);
    render();
}

void map_request(XEvent &e) {
    XMapRequestEvent event = e.xmaprequest;
    log_event(__func__, event.window);

    Tag *tag = state_get_current_tag(&state);
    if (mf_vec_index(tag->windows, (u32) event.window) >= 0) {
        screen_layout_windows(&state.x11, &state.screens[0], tag->windows);
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

    LOGF("umap_request window %d", event.window);
    Tag *tag = state_get_current_tag(&state);
    i32 index = mf_vec_index(tag->windows, (u32) event.window);
    if (index >= 0) {
        mf_vec_delete(tag->windows, index);
        mf_str *s = mf_vec_delete(tag->window_names, index);
        mf_str_free(*s);
        screen_layout_windows(&state.x11, &state.screens[0], tag->windows);
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

    Tag *tag = state_get_current_tag(&state);
    if (mf_vec_index(tag->windows, (u32) event.window) == -1) {
        state_push_window(&state, event.window);
    }
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
    x11_fill_rect(&state.x11, state.x11_statusbar,
                  0, 0,
                  state.x11_statusbar.width, state.x11_statusbar.height,
                  state.color_bar_bg);


    i32 x_margin = 5;
    i32 spacing = 1;
    i32 x = spacing;
    i32 h = state.x11_statusbar.height - 2 * spacing;
    for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
        mf_strview text = S(tags[i]);
        i32 w = x11_get_text_width(&state.x11, text.data, text.size);
        i32 w_total = w + x_margin * 2;

        X11Color c = state.color_button_tag_bg;
        if (i == state.selected_tag) {
            c = state.color_button_selected_tag_bg;
        }

        x11_fill_rect(&state.x11, state.x11_statusbar,
                      x, spacing, w_total, h, c);

        // TODO: Center Text
        x11_draw_text(&state.x11, state.x11_statusbar, x + x_margin,
                      state.x11.font_height,
                      text.data, text.size, state.color_button_tag_fg);

        x += w_total + spacing;
    }
    x += 100;

    Tag *tag = state_get_current_tag(&state);
    for (i32 i = 0; i < mf_vec_size(tag->window_names); ++i) {
        mf_str &text = tag->window_names[i];
        i32 w = x11_get_text_width(&state.x11, text.data, text.size);
        i32 w_total = w + x_margin * 2;
        X11Color c = state.color_button_window_bg;
        if (i == tag->selected_window) {
            c = state.color_button_selected_window_bg;
        }

        x11_fill_rect(&state.x11, state.x11_statusbar,
                      x, 0, w, w, c);

        // TODO: Center Text
        x11_draw_text(&state.x11, state.x11_statusbar,
                      x + x_margin, state.x11.font_height,
                      text.data, text.size, state.color_button_window_fg);
        x += w + spacing;
    }
    //XSync(state.x11.display, False);
}

int main() {
    log_file = fopen("./mfwm.log", "w");
    MF_Assert(log_file);

    // X11
    x11_init(&state.x11);

    // TODO: What to do with alpha
    // TODO: move this to config
    state.color_bar_bg = x11_add_color(&state.x11, 0x28282800);
    state.color_button_selected_tag_bg = x11_add_color(&state.x11, 0x83a59800);
    state.color_button_tag_bg = x11_add_color(&state.x11, 0x45858800);
    state.color_button_selected_window_bg = x11_add_color(&state.x11, 0x8ec07c00);
    state.color_button_window_bg = x11_add_color(&state.x11, 0x689d6a00);

    state.color_button_tag_fg = x11_add_color(&state.x11, 0x0000000000);
    state.color_button_tag_fg = x11_add_color(&state.x11, 0x0000000000);
    state.color_button_window_fg = x11_add_color(&state.x11, 0x0000000000);

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

    // Setup datastructures
    for (i32 i = 0; i < MF_ArrayLength(tags); ++i) {
        Tag *tag = mf_vec_add(state.tags);
        tag->name = tags[i];
        tag->window_names = NULL;
        tag->windows = NULL;
    }
    state.statusbar = {state.screens[0].w, STATUSBAR_HEIGHT};
    state.x11_statusbar = x11_window_create(&state.x11, state.statusbar.width, state.statusbar.height);

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

                // These events are in order of the event chain
                case CreateNotify: break;
                case ConfigureRequest: configure_request(e); break;
                case ConfigureNotify: configure_notify(e); break;
                case MapRequest: map_request(e); break;
                case MapNotify: map_notify(e); break;
                case Expose: render(); break;
                // --------

                case MotionNotify: break;
                case EnterNotify: break;
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
    x11_window_destroy(&state.x11, state.x11_statusbar);
    x11_shutdown(&state.x11);
}

