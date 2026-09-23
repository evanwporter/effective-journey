#ifndef CONFIG_H
#define CONFIG_H

/* number of clients in master area */
static unsigned int nmaster = 1;

/* Master area size [0.05..0.95] */
static float mfact = 0.5;

/* Border width in pixels */
static const int borderpx = 3;

/* Border colors (RGBA, 0x00000000 to 0xffffffff)
 * Colors from dwm gruvbox theme:
 * - Unfocused: #3c3836 (dark gray)
 * - Focused:   #e78a3e (orange)
 */
static const unsigned int border_color_unfocused[] = {
    0x3c000000,  // R (0x3c / 0xff * 0xffffffff)
    0x38000000,  // G
    0x36000000,  // B
    0xffffffff,  // A (opaque)
};

static const unsigned int border_color_focused[] = {
    0xe7000000,  // R (0xe7 / 0xff * 0xffffffff)
    0x8a000000,  // G
    0x3e000000,  // B
    0xffffffff,  // A (opaque)
};

/* Layout definitions */
static const Layout layouts[] = {
    /* symbol     arrange function */
    {"[]=", tile},   /* tiling layout */
    {"[M]", monocle}, /* monocle layout */
    {"><>", NULL},   /* floating layout (no arrange function) */
};

#endif  // CONFIG_H
