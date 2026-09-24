/* tinyrwm configuration */

#include <xkbcommon/xkbcommon-keysyms.h>

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
    0x3c000000, // R (0x3c / 0xff * 0xffffffff)
    0x38000000, // G
    0x36000000, // B
    0xffffffff, // A (opaque)
};

static const unsigned int border_color_focused[] = {
    0xe7000000, // R (0xe7 / 0xff * 0xffffffff)
    0x8a000000, // G
    0x3e000000, // B
    0xffffffff, // A (opaque)
};

/* Modifier key macros */
#define CONTROL (1 << 2) // RIVER_SEAT_V1_MODIFIERS_CTRL
#define SUPER (1 << 6) // RIVER_SEAT_V1_MODIFIERS_MOD4
#define SHIFT (1 << 0) // RIVER_SEAT_V1_MODIFIERS_SHIFT
#define ALT (1 << 3) // RIVER_SEAT_V1_MODIFIERS_MOD1

/* Key bindings
 * Each entry defines: { modifiers, keysym, function, argument }
 * See xkbcommon-keysyms.h for key symbols
 */
static const Key keybinds[] = {
    /* modifier         key              function          argument */
    { SUPER | SHIFT, XKB_KEY_Return, spawn_terminal, { 0 } },
    { SUPER | SHIFT, XKB_KEY_q, exit_wm, { 0 } },
    { SUPER | SHIFT, XKB_KEY_c, close_window, { 0 } },
    { SUPER, XKB_KEY_j, focus_stack, { .i = +1 } },
    { SUPER, XKB_KEY_k, focus_stack, { .i = -1 } },
    { SUPER, XKB_KEY_h, set_master_fact, { .f = -0.05 } },
    { SUPER, XKB_KEY_l, set_master_fact, { .f = +0.05 } },
};

/* Layout definitions */
static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=", tile }, /* tiling layout */
    { "[M]", monocle }, /* monocle layout */
    { "><>", NULL }, /* floating layout (no arrange function) */
};
