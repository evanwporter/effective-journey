// SPDX-FileCopyrightText: © 2026 Evan Porter
// SPDX-License-Identifier: 0BSD

#ifndef TINYRWM_H
#define TINYRWM_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

/* Forward declarations for River protocol types */
struct river_output_v1;
struct river_window_v1;
struct river_node_v1;
struct river_seat_v1;
struct river_xkb_binding_v1;
struct river_pointer_binding_v1;
struct river_window_manager_v1;
struct river_xkb_bindings_v1;

/* Output structure */
struct Output
{
    /* These variables represents the position and dimensions of the monitor.
     *    mx - monitor position on the x-axis
     *    my - monitor position on the y-axis
     *    mw - the monitor's width
     *    mh - the monitor's height
     */
    int mx, my, mw, mh;

    /* These variables represents the position and dimensions of the window area, as in the part
     * of the monitor where windows are tiled. This is the space of the monitor excluding the
     * bar window. These are set in the updatebarpos function.
     *    wx - window area position on the x-axis
     *    wy - window area position on the y-axis
     *    ww - the window area's width
     *    wh - the window area's height
     */
    int wx, wy, ww, wh;

    /* This represents the workspaces the monitor owns.
     *
     * As an example consider the hexadecimal value of 0x51 (decimal 81) which has a binary
     * value of:
     *    001010001  - bitmask
     *    987654321  - workspaces
     *
     * This would mean that the monitor is showing workspaces 1, 5 and 7.
     */
    unsigned int workspaces;

    struct river_output_v1* obj;
    bool removed;
    struct wl_list link;  // WindowManager.outputs
};

/* Window structure */
struct Window
{
    struct river_window_v1* obj;
    struct river_node_v1* node;

    bool new;
    bool closed;
    bool isfloating;

    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    struct Seat* pointer_move_requested;
    struct Seat* pointer_resize_requested;
    uint32_t pointer_resize_requested_edges;

    struct wl_list link;  // WindowManager.windows
};

/* Action enumeration for key/pointer bindings */
enum Action
{
    ACTION_NONE,
    ACTION_SPAWN_FOOT,
    ACTION_CLOSE,
    ACTION_FOCUS_NEXT,
    ACTION_MOVE,
    ACTION_RESIZE,
    ACTION_EXIT,
};

/* XKB (keyboard) binding structure */
struct XkbBinding
{
    struct river_xkb_binding_v1* obj;
    struct Seat* seat;
    enum Action action;
    struct wl_list link;
};

/* Pointer (mouse) binding structure */
struct PointerBinding
{
    struct river_pointer_binding_v1* obj;
    struct Seat* seat;
    enum Action action;
    struct wl_list link;
};

/* Seat operation enumeration */
enum SeatOp
{
    SEAT_OP_NONE,
    SEAT_OP_MOVE,
    SEAT_OP_RESIZE,
};

/* Seat structure - represents an input device (keyboard/pointer) */
struct Seat
{
    struct river_seat_v1* obj;
    bool new;
    bool removed;

    struct Window* focused;
    struct Window* hovered;
    struct Window* interacted;

    struct wl_list xkb_bindings;      // XkbBinding
    struct wl_list pointer_bindings;  // PointerBinding
    enum Action pending_action;

    enum SeatOp op;
    // For SEAT_OP_MOVE and SEAT_OP_RESIZE
    struct Window* op_window;
    int32_t op_start_x, op_start_y;
    int32_t op_dx, op_dy;
    bool op_release;
    // For SEAT_OP_RESIZE only
    int32_t op_start_width, op_start_height;
    uint32_t op_edges;

    struct wl_list link;  // WindowManager.seats
};

/* Window manager global state */
struct WindowManager
{
    struct wl_list outputs;  // Output
    struct wl_list windows;  // Window
    struct wl_list seats;    // Seat
};

/* Global variables */
extern struct WindowManager wm;
extern struct river_window_manager_v1* window_manager_v1;
extern struct river_xkb_bindings_v1* xkb_bindings_v1;

/* Tiling configuration */
extern unsigned int nmaster;
extern float mfact;

/* Function declarations */
struct Window* nexttiled(struct Window* w);

#endif  // TINYRWM_H
