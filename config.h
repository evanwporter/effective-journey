#ifndef CONFIG_H
#define CONFIG_H

#include <xkbcommon/xkbcommon-keysyms.h>
#include <river-window-management-v1-client-protocol.h>

/* Forward declarations */
struct Seat;
struct Output;
typedef struct Layout Layout;
typedef union Arg Arg;

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

/* Modifier key macros */
#define CONTROL RIVER_SEAT_V1_MODIFIERS_CTRL
#define SUPER RIVER_SEAT_V1_MODIFIERS_MOD4
#define SHIFT RIVER_SEAT_V1_MODIFIERS_SHIFT
#define ALT RIVER_SEAT_V1_MODIFIERS_MOD1

/* Function declarations (defined in tinyrwm.c) */
void spawn_terminal(struct Seat* seat, const Arg* arg);
void close_window(struct Seat* seat, const Arg* arg);
void focus_stack(struct Seat* seat, const Arg* arg);
void set_master_fact(struct Seat* seat, const Arg* arg);
void exit_wm(struct Seat* seat, const Arg* arg);

/* Commands */
static const char* termcmd[] = {"foot", NULL};

/* Key bindings
 * Each entry defines: { modifiers, keysym, function, argument }
 * See xkbcommon-keysyms.h for key symbols
 */
static const Key keybinds[] = {
    /* modifier         key              function          argument */
    {SUPER | SHIFT, XKB_KEY_Return, spawn_terminal,   {0}},
    {SUPER | SHIFT, XKB_KEY_q,      exit_wm,          {0}},
    {SUPER | SHIFT, XKB_KEY_c,      close_window,     {0}},
    {SUPER,         XKB_KEY_j,      focus_stack,      {.i = +1}},
    {SUPER,         XKB_KEY_k,      focus_stack,      {.i = -1}},
    {SUPER,         XKB_KEY_h,      set_master_fact,  {.f = -0.05}},
    {SUPER,         XKB_KEY_l,      set_master_fact,  {.f = +0.05}},
};

#endif  // CONFIG_H
