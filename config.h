#pragma once

union Arg {
    const char* string;
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
        char * l[] = {(char *) arg.string, NULL};
        setsid();
        execvp(arg.string, l);
    }
}

void quit(Arg arg) {
    state.running = false;
}

void select_next_window(Arg arg) {
    u32 amount_windows = mf_vec_size(state.windows);
    if (amount_windows > 0) {
        state.selected_window = MF_Min(state.selected_window + 1,
                                       amount_windows - 1);
        x11_window_focus(&state.x11, state.windows[state.selected_window]);
    }
}

void select_previous_window(Arg arg) {
    u32 amount_windows = mf_vec_size(state.windows);
    if (amount_windows > 0) {
        state.selected_window = MF_Max(state.selected_window - 1,
                                       0);
        x11_window_focus(&state.x11, state.windows[state.selected_window]);
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

static KeyDef keybindings[] = {
    { MODKEY, XK_q, quit },
    { MODKEY, XK_j, select_next_window },
    { MODKEY, XK_k, select_previous_window },

    // Applications
    { MODKEY, XK_p, run , "dmenu_run"},
    { MODKEY, XK_w, run, "chromium" },
    { MODKEY, XK_Return, run, "xterm" },
};
