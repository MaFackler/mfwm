
TEST("Window management") {
    Tag t;
    
    MFT_CHECK(!tag_has_windows(&t));
    MFT_CHECK(!tag_has_window(&t, 1337));

    tag_add_window(&t, 1337, mf_str_new("1337 Window"));
    MFT_CHECK(tag_has_window(&t, 1337));

    MFT_CHECK_INT(tag_get_selected_window(&t), 1337);

    tag_add_window(&t, 815, mf_str_new("815 Window"));
    MFT_CHECK(tag_has_window(&t, 1337));
    MFT_CHECK(tag_has_window(&t, 815));

    MFT_CHECK_INT(tag_get_selected_window(&t), 815);
    tag_select_window(&t, 1337);
    MFT_CHECK_INT(tag_get_selected_window(&t), 1337);

}
