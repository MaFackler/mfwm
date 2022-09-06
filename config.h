#pragma once

union Arg {
    const char* String;
    i32 Int;
};

typedef void (*Action)(Arg arg);

struct KeyDef {
    u32 state;
    KeySym keysym;
    Action action;
    Arg arg;
};


void run(Arg arg) {
    if (fork() == 0) {
        char * l[] = {(char *) arg.String, NULL};
        setsid();
        execvp(arg.String, l);
    }
}

void quit(Arg arg) {
    state.running = false;
}

void select_next_window(Arg arg) {
    Tag *tag = state_get_current_tag(&state);
    u32 amount_windows = mf_vec_size(tag->windows);
    if (amount_windows > 0) {
        tag->selected_window = MF_Min(tag->selected_window + 1, amount_windows - 1);
        x11_window_focus(&state.x11, tag->windows[tag->selected_window]);
    }
}

void select_previous_window(Arg arg) {
    Tag *tag = state_get_current_tag(&state);
    u32 amount_windows = mf_vec_size(tag->windows);
    if (amount_windows > 0) {
        tag->selected_window = MF_Max(tag->selected_window - 1, 0);
        x11_window_focus(&state.x11, tag->windows[tag->selected_window]);
    }

}


// Constants
const i32 STATUSBAR_HEIGHT = 20;
const i32 GAP = 10;

#define MODKEY Mod1Mask  // ALT KEY
//#define MODKEY Mod4Mask  // SUPER KEY

const char* tags[] = {
    "1", "2", "3"
};

void select_next_tag(Arg arg) {
    assert(MF_ArrayLength(tags) > 0);
    state.selected_tag = mf_clamp(state.selected_tag + 1, 0, (i32) MF_ArrayLength(tags) - 1); 
}

void select_previous_tag(Arg arg) {
    assert(MF_ArrayLength(tags) > 0);
    state.selected_tag = mf_clamp(state.selected_tag - 1, 0, (i32) MF_ArrayLength(tags) - 1); 
}

void select_tag_nr(Arg arg) {
    state_select_tag(&state, arg.Int);
}

struct ColorSchemeDefinition {
    u32 fg;
    u32 bg;
};

enum {
    ColorSchemeTags,
    ColorSchemeWindows,
};

static const ColorSchemeDefinition color_schemes[][2] = {
    [ColorSchemeTags] = {{ 0x0, 0x45858800 }, {0x0, 0x83a59800}},
    [ColorSchemeWindows] = {{ 0x0, 0x689d6a00 }, {0x0, 0x8ec07c00}},
};

static KeyDef keybindings[] = {
    { MODKEY, XK_q, quit },
    { MODKEY, XK_j, select_next_window },
    { MODKEY, XK_k, select_previous_window },

    { MODKEY, XK_1, select_tag_nr, {.Int=0} },
    { MODKEY, XK_2, select_tag_nr, {.Int=1} },
    { MODKEY, XK_3, select_tag_nr, {.Int=2} },

    // Applications
    { MODKEY, XK_p, run , "dmenu_run"},
    { MODKEY, XK_w, run, "chromium" },
    { MODKEY, XK_Return, run, "xterm" },
};
