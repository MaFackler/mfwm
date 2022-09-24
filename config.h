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

void run_sync(Arg arg) {
    command_run_process_sync(arg.String);
}

void run(Arg arg) {
    command_run_process_child((char *) arg.String);
}

void quit(Arg arg) {
    state.running = false;
}

void select_next_window(Arg arg) {
    window_manager_window_next(&state.wm);
}

void select_previous_window(Arg arg) {
    window_manager_window_previous(&state.wm);
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
    MF_Assert(MF_ArrayLength(tags) > 0);
    Monitor *mon = window_manager_get_selected_monitor(&state.wm);
    window_manager_monitor_select_tag(&state.wm, mon, mf_clamp(mon->selected_tag + 1, 0, (i32) MF_ArrayLength(tags) - 1));
}

void select_previous_tag(Arg arg) {
    MF_Assert(MF_ArrayLength(tags) > 0);
    Monitor *mon = window_manager_get_selected_monitor(&state.wm);
    window_manager_monitor_select_tag(&state.wm, mon, mf_clamp(mon->selected_tag - 1, 0, (i32) MF_ArrayLength(tags) - 1)); 
}

void select_tag_nr(Arg arg) {
    window_manager_tag_select(&state.wm, arg.Int);
}


enum {
    ColorSchemeBar,
    ColorSchemeTags,
    ColorSchemeWindows,
    ColorSchemeCount,
};

static int border_width_selected = 2;
static int border_width_unselected = 1;

static const ColorScheme colorschemes[] = {
    [ColorSchemeBar] = {{ 0x0, 0x0}, {0x0, 0x0}},
    [ColorSchemeTags] = {{ 0x0, 0x45858800 }, {0x0, 0x83a59800}},
    [ColorSchemeWindows] = {{ 0x0, 0x689d6a00 }, {0x0, 0x8ec07c00}},
};

static KeyDef keybindings[] = {
    { MODKEY | ShiftMask, XK_q, quit },
    { MODKEY, XK_j, select_next_window },
    { MODKEY, XK_k, select_previous_window },

    { MODKEY, XK_1, select_tag_nr, {.Int=0} },
    { MODKEY, XK_2, select_tag_nr, {.Int=1} },
    { MODKEY, XK_3, select_tag_nr, {.Int=2} },

    // Applications
    { MODKEY, XK_p, run , "dmenu_run"},
    { MODKEY, XK_w, run, "chromium" },
    { MODKEY, XK_Return, run, "alacritty" },
};

static const char *startup_commands[] = {
    "feh --bg-scale ~/Pictures/Wallpaper.jpg",
};
