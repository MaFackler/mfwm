#pragma once
#include <mf.h>
#define MF_STRING_IMPLEMENTATION
#include <mf_string.h>
#define MF_VECTOR_IMPLEMENTATION
#include <mf_vector.h>

// TODO: Rect maybe from mf_math
struct Rect {
    i32 x;
    i32 y;
    i32 w;
    i32 h;
};

struct Tag {
    const char *name;
    i32 selected_window = 0;
    vec(u32) windows = NULL;
    vec(mf_str) window_names = NULL;
};


void tag_add_window(Tag *tag, u32 window, mf_str window_name);
bool tag_has_window(Tag *tag, u32 window);
bool tag_has_windows(Tag *tag);
u32 tag_get_selected_window(Tag *tag);
void tag_select_window(Tag *tag, u32 window);

struct WindowManager {

    i32 selected_tag = 0;
    vec(Tag) tags = NULL;
    vec(Rect) screens = NULL;
};

Tag* window_manager_get_current_tag(WindowManager *wm);
