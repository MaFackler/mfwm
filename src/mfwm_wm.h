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

struct WindowManagerApi {
    void (*window_register)(u32 window);
    void (*window_focus)(u32 window);
    void (*window_unfocus)(u32 window);
    void (*window_hide)(u32 window);
    void (*do_layout)(Rect *rect, vec(u32) windows);
};

struct Tag {
    const char *name;
    i32 selected_window = 0;
    vec(u32) windows = NULL;
    vec(mf_str) window_names = NULL;
};


struct Monitor {
    Rect rect;
    i32 selected_tag = 0;
    vec(Tag) tags = NULL;
};


struct WindowManager {
    WindowManagerApi api; 
    i32 selected_monitor = 0;
    vec(Monitor) monitors = NULL;
};

void window_manager_window_add(WindowManager *wm, u32 window, mf_str window_name);
void window_manager_window_delete(WindowManager *wm, u32 window);
void window_manager_window_focus(WindowManager *wm, u32 window);
void window_manager_window_next(WindowManager *wm);
void window_manager_window_previous(WindowManager *wm);
void window_manager_tag_select(WindowManager *wm, u32 tag_index);

// Getter
Monitor* window_manager_get_selected_monitor(WindowManager *wm);
Tag* window_manager_monitor_get_selected_tag(WindowManager *wm, Monitor *monitor);


Monitor* window_manager_add_monitor(WindowManager *wm, Rect rect);
Monitor* window_manager_select_monitor(WindowManager *wm, i32 index);

Tag* window_manager_get_selected_tag(WindowManager *wm);
Tag* window_manager_monitor_add_tag(WindowManager *wm, Monitor *monitor, const char *name);
Tag* window_manager_monitor_select_tag(WindowManager *wm, Monitor *monitor, i32 index);

bool window_manager_tag_has_window(WindowManager *wm, Tag *tag, u32 window);
bool window_manager_tag_has_windows(WindowManager *wm, Tag *tag);
u32 window_manager_tag_get_selected_window(WindowManager *wm, Tag *tag);
void window_manager_tag_select_window(WindowManager *wm, Tag *tag, u32 window);
