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

// Constants
const i32 STATUSBAR_HEIGHT = 20;
const i32 GAP = 10;

#define MODKEY Mod1Mask  // ALT KEY
//#define MODKEY Mod4Mask  // SUPER KEY


static KeyDef keybindings[] = {
    { MODKEY, XK_p, run , "dmenu_run"},
    { MODKEY, XK_q, quit },
    { MODKEY, XK_w, run, "chromium" },
};
