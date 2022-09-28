void window_manager_window_add(WindowManager *wm, u32 window, const char *window_name) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);

    if (window_manager_tag_has_windows(wm, tag)) {
        u32 old_window = window_manager_tag_get_selected_window(wm, tag);
        wm->api.window_unfocus(old_window);
    }
    tag->windows.push_back(window);
    u64 index = tag->windows.size() - 1;
    tag->window_names.push_back(window_name);
    tag->selected_window = index;
    wm->api.window_register(window);
    wm->api.window_focus(window);
    wm->api.do_layout(&mon->rect, tag->windows);
}

void window_manager_window_delete(WindowManager *wm, u32 window) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);
    auto ele = std::find(tag->windows.begin(), tag->windows.end(), window);
    u64 index = ele - tag->windows.begin();

    if (ele != tag->windows.end()) {
        tag->windows.erase(ele);
        tag->window_names.erase(tag->window_names.begin() + index);

        wm->api.do_layout(&mon->rect, tag->windows);
        tag->selected_window = std::min(index, tag->windows.size() - 1);
        if (window_manager_tag_has_windows(wm, tag)) {
            wm->api.window_focus(window_manager_tag_get_selected_window(wm, tag));
        }
    }
}

void window_manager_window_focus(WindowManager *wm, u32 window) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);


    // TODO: it seems like this could be called but the monitor is not 'internally'
    // switched. So its neccesary to check if the window is available
    if (window_manager_tag_has_window(wm, tag, window)) {
        u32 old_window = window_manager_tag_get_selected_window(wm, tag);
        if (window != old_window) {
            window_manager_tag_select_window(wm, tag, window);

            wm->api.window_unfocus(old_window);
            wm->api.window_focus(window);
        }
    }
}

void window_manager_window_next(WindowManager *wm) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);
    i32 amount_windows = tag->windows.size();
    if (amount_windows > 0) {
        wm->api.window_unfocus(window_manager_tag_get_selected_window(wm, tag));
        tag->selected_window = MF_Min(tag->selected_window + 1, amount_windows - 1);
        wm->api.window_focus(window_manager_tag_get_selected_window(wm, tag));
    }
}

void window_manager_window_previous(WindowManager *wm) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);
    u32 amount_windows = tag->windows.size();
    if (amount_windows > 0) {
        wm->api.window_unfocus(window_manager_tag_get_selected_window(wm, tag));
        tag->selected_window = std::max(tag->selected_window - 1, 0);
        wm->api.window_focus(window_manager_tag_get_selected_window(wm, tag));
    }
}

void window_manager_tag_select(WindowManager *wm, u32 tag_index) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);
    for(u32 it : tag->windows) {
        wm->api.window_hide(it);
    }
    tag = window_manager_monitor_select_tag(wm, mon, tag_index);

    wm->api.do_layout(&mon->rect, tag->windows);

    if (window_manager_tag_has_windows(wm, tag)) {
        u32 win = window_manager_tag_get_selected_window(wm, tag);
        wm->api.window_focus(win);
    }
}

bool window_manager_tag_has_window(WindowManager *wm, Tag *tag, u32 window) {
    auto it = std::find(tag->windows.begin(), tag->windows.end(), window);
    return it != tag->windows.end();
}

bool window_manager_tag_has_windows(WindowManager *wm, Tag *tag) {
    return tag->windows.size() > 0;
}

u32 window_manager_tag_get_selected_window(WindowManager *wm, Tag *tag) {
    return tag->windows[tag->selected_window];
}

void window_manager_tag_select_window(WindowManager *wm, Tag *tag, u32 window) {
    for (i32 i = 0; i < tag->windows.size(); ++i) {
        if (tag->windows[i] == window) {
            u32 old_window = window_manager_tag_get_selected_window(wm, tag);

            tag->selected_window = i;
            u32 new_window = tag->windows[tag->selected_window];

            return;
        }
    }
    MF_Assert(!"INVALID");
}


Monitor* window_manager_add_monitor(WindowManager *wm, Rect rect) {
    wm->selected_monitor = wm->monitors.size();
    Monitor &monitor = wm->monitors.emplace_back();
    monitor.rect = rect;
    monitor.selected_tag = 0;
    return &monitor;
}


Monitor* window_manager_get_selected_monitor(WindowManager *wm) {
    return &wm->monitors[wm->selected_monitor];
}

Monitor* window_manager_select_monitor(WindowManager *wm, i32 index) {
    wm->selected_monitor = index;
    return window_manager_get_selected_monitor(wm);
}

Tag* window_manager_get_selected_tag(WindowManager *wm) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    return window_manager_monitor_get_selected_tag(wm, mon);
}

Tag* window_manager_monitor_get_selected_tag(WindowManager *wm, Monitor *monitor) {
    return &monitor->tags[monitor->selected_tag];    
}

u32 window_manager_get_selected_window(WindowManager *wm) {
    Monitor *mon = window_manager_get_selected_monitor(wm);
    Tag *tag = window_manager_monitor_get_selected_tag(wm, mon);
    return window_manager_tag_get_selected_window(wm, tag);
}

Tag* window_manager_monitor_add_tag(WindowManager *wm, Monitor *monitor, const char *name) {
    Tag &tag = monitor->tags.emplace_back(); 
    tag.name = name;
    tag.selected_window = 0;
    return &tag;
}

Tag* window_manager_monitor_select_tag(WindowManager *wm, Monitor *monitor, i32 index) {
    monitor->selected_tag = index;
    return window_manager_monitor_get_selected_tag(wm, monitor);
}
