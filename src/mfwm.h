#pragma once

// config.h stuff
struct ColorSchemeDefinition {
    u32 fg;
    u32 bg;
};

struct ColorScheme {
    ColorSchemeDefinition normal;
    ColorSchemeDefinition selected;
};

struct Statusbar {
    i32 x;
    i32 y;
    i32 width;
    i32 height;
};

