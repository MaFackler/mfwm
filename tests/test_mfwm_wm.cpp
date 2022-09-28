void dummy(u32 window) {
}

void dummy_layout(Rect *rec, const vector<u32> &windows) {
}

WindowManager setup() {
    WindowManager wm = {};
    wm.api.window_register = dummy;
    wm.api.window_focus = dummy;
    wm.api.window_unfocus = dummy;
    wm.api.window_hide = dummy;
    wm.api.do_layout = dummy_layout;
    return wm;
}

TEST("Multiple monitors") {
    WindowManager wm = setup();
    window_manager_add_monitor(&wm, {0, 0, 1920, 1080});
    CHECK(wm.selected_monitor, 0);
    CHECK(wm.monitors.size(), 1);

    window_manager_add_monitor(&wm, {1920, 0, 1920, 1080});
    CHECK(wm.selected_monitor, 1);
    CHECK(wm.monitors.size(), 2);

    CHECK(wm.monitors[0].tags.size(), 0);
    CHECK(wm.monitors[1].tags.size(), 0);
    Monitor *mon = window_manager_select_monitor(&wm, 0);

    Tag *tag = window_manager_monitor_add_tag(&wm, mon, "1");
    window_manager_window_add(&wm, 1337, "Screen 1 Tag 1 Window 1");
    CHECK(tag->selected_window, 0);
    window_manager_window_add(&wm, 1339, "Screen 1 Tag 1 Window 2");
    CHECK(tag->selected_window, 1);

    wm.selected_monitor = 1;
    mon = window_manager_get_selected_monitor(&wm);
    tag = window_manager_monitor_add_tag(&wm, mon, "1");
    window_manager_window_add(&wm, 1, "Screen 2 Tag 1 Window 1");

    CHECK(wm.monitors.size(), 2);
    CHECK(wm.monitors[0].tags.size(), 1);
    CHECK(wm.monitors[0].tags[0].windows.size(), 2);
    CHECK(wm.monitors[0].tags[0].window_names[0].c_str(), "Screen 1 Tag 1 Window 1");
    CHECK(wm.monitors[0].tags[0].window_names[1].c_str(), "Screen 1 Tag 1 Window 2");

    CHECK(wm.monitors[1].tags.size(), 1);
    CHECK(wm.monitors[1].tags[0].windows.size(), 1);
    CHECK(wm.monitors[1].tags[0].window_names[0].c_str(), "Screen 2 Tag 1 Window 1");

}

TEST("next and previous") {
    WindowManager wm = setup();
    Monitor *mon = window_manager_add_monitor(&wm, {0, 0, 1600, 900});
    window_manager_monitor_add_tag(&wm, mon, "1");

    window_manager_window_add(&wm, 1, "First Widnow");
    CHECK(wm.monitors[0].tags[0].selected_window, 0);
    window_manager_window_add(&wm, 2, "Second Windows");
    CHECK(wm.monitors[0].tags[0].selected_window, 1);

    CHECK(wm.monitors[0].tags[0].selected_window, 1);
    window_manager_window_next(&wm);
    CHECK(wm.monitors[0].tags[0].selected_window, 1);
    window_manager_window_previous(&wm);
    CHECK(wm.monitors[0].tags[0].selected_window, 0);
    window_manager_window_previous(&wm);
    CHECK(wm.monitors[0].tags[0].selected_window, 0);
    window_manager_window_next(&wm);
    CHECK(wm.monitors[0].tags[0].selected_window, 1);
}
