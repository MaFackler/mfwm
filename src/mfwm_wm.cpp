
void tag_add_window(Tag *tag, u32 window, mf_str window_name) {
    u32 index = mf_vec_push(tag->windows, window);
    mf_vec_push(tag->window_names, window_name);
    tag->selected_window = index;
}

bool tag_has_window(Tag *tag, u32 window) {
    mf_vec_for(tag->windows) {
        if (*it == window)
            return true;
    }
    return false;
}

bool tag_has_windows(Tag *tag) {
    return mf_vec_size(tag->windows) > 0;
}

u32 tag_get_selected_window(Tag *tag) {
    return tag->windows[tag->selected_window];
}

void tag_select_window(Tag *tag, u32 window) {
    for (i32 i = 0; i < mf_vec_size(tag->windows); ++i) {
        if (tag->windows[i] == window) {
            tag->selected_window = i;
            return;
        }
    }
    assert(!"INVALID");
}

Tag* window_manager_get_current_tag(WindowManager *wm) {
    return &wm->tags[wm->selected_tag];    
}
