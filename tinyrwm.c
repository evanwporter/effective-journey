// SPDX-FileCopyrightText: © 2026 Isaac Freund
// SPDX-License-Identifier: 0BSD

#include "tinyrwm.h"

#include <errno.h>
#include <linux/input-event-codes.h>
#include <river-window-management-v1-client-protocol.h>
#include <river-xkb-bindings-v1-client-protocol.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "config.h"
#include "util.h"

/* Global variables */
struct WindowManager wm;
struct river_window_manager_v1* window_manager_v1;
struct river_xkb_bindings_v1* xkb_bindings_v1;

static void output_handle_removed(void* data, struct river_output_v1* obj)
{
    struct Output* output = data;
    output->removed = true;
}

// Ignored events
static void output_handle_wl_output(void* data, struct river_output_v1* obj, uint32_t name)
{
}

static void output_handle_position(void* data, struct river_output_v1* obj, int32_t x, int32_t y)
{
    struct Output* output = data;
    output->mx = x;
    output->my = y;

    // Window area position
    // TODO: Bar
    output->wx = x;
    output->wy = y;
}

static void output_handle_dimensions(void* data,
                                     struct river_output_v1* obj,
                                     int32_t width,
                                     int32_t height)
{
    struct Output* output = data;
    output->mw = width;
    output->mh = height;

    // Window area dimensions
    // TODO: Bar
    output->ww = width;
    output->wh = height;
}

const struct river_output_v1_listener river_output_listener = {
    .removed = output_handle_removed,
    .wl_output = output_handle_wl_output,
    .position = output_handle_position,
    .dimensions = output_handle_dimensions,
};

static void output_maybe_destroy(struct Output* output)
{
    if (!output->removed)
    {
        return;
    }
    river_output_v1_destroy(output->obj);
    wl_list_remove(&output->link);
    free(output);
}

/* This returns the next tiled client on the currently selected tag(s).
 *
 * Given an input client c the function returns the next visible tiled client in the list, or NULL
 * if there are no more subsequent tiled clients.
 */
struct Window* nexttiled(struct Window* w)
{
    if (!w || !w->mon) return NULL;

    for (; w && (w->isfloating || w->closed); w = wl_container_of(w->tile_link.next, w, tile_link))
    {
        // Check if we've reached the end of the list
        if (w->tile_link.next == &w->mon->clients) return NULL;
    }
    return w;
}

/* Attach window to the beginning of the tile list (makes it the new master) */
void attach(struct Window* w)
{
    wl_list_insert(&w->mon->clients, &w->tile_link);
}

/* Detach window from the tile list */
void detach(struct Window* w)
{
    wl_list_remove(&w->tile_link);
}

/* Attach window to the beginning of the stack (focus) list */
void attachstack(struct Window* w)
{
    wl_list_insert(&w->mon->stack, &w->stack_link);
}

/* Detach window from the stack (focus) list */
void detachstack(struct Window* w)
{
    wl_list_remove(&w->stack_link);
}

static void window_handle_closed(void* data, struct river_window_v1* obj)
{
    struct Window* window = data;
    window->closed = true;
}

static void window_handle_dimensions(void* data,
                                     struct river_window_v1* obj,
                                     int32_t width,
                                     int32_t height)
{
    struct Window* window = data;
    window->w = width;
    window->h = height;
}

static void window_handle_pointer_move_requested(void* data,
                                                 struct river_window_v1* obj,
                                                 struct river_seat_v1* river_seat)
{
    struct Window* window = data;
    window->pointer_move_requested = river_seat_v1_get_user_data(river_seat);
}

static void window_handle_pointer_resize_requested(void* data,
                                                   struct river_window_v1* obj,
                                                   struct river_seat_v1* river_seat,
                                                   uint32_t edges)
{
    struct Window* window = data;
    window->pointer_resize_requested = river_seat_v1_get_user_data(river_seat);
    window->pointer_resize_requested_edges = edges;
}

// Ignored events
static void window_handle_dimensions_hint(void* data,
                                          struct river_window_v1* obj,
                                          int32_t min_width,
                                          int32_t min_height,
                                          int32_t max_width,
                                          int32_t max_height)
{
}
static void window_handle_app_id(void* data, struct river_window_v1* obj, const char* app_id)
{
}
static void window_handle_title(void* data, struct river_window_v1* obj, const char* title)
{
}
static void window_handle_parent(void* data,
                                 struct river_window_v1* obj,
                                 struct river_window_v1* parent)
{
}
static void window_handle_decoration_hint(void* data, struct river_window_v1* obj, uint32_t hint)
{
}
static void window_handle_show_window_menu_requested(void* data,
                                                     struct river_window_v1* obj,
                                                     int32_t x,
                                                     int32_t y)
{
}
static void window_handle_maximize_requested(void* data, struct river_window_v1* obj)
{
}
static void window_handle_unmaximize_requested(void* data, struct river_window_v1* obj)
{
}
static void window_handle_fullscreen_requested(void* data,
                                               struct river_window_v1* obj,
                                               struct river_output_v1* river_output)
{
}
static void window_handle_exit_fullscreen_requested(void* data, struct river_window_v1* obj)
{
}
static void window_handle_minimize_requested(void* data, struct river_window_v1* obj)
{
}
static void window_handle_unreliable_pid(void* data,
                                         struct river_window_v1* obj,
                                         int32_t unreliable_pid)
{
}
static void window_handle_presentation_hint(void* data, struct river_window_v1* obj, uint32_t hint)
{
}
static void window_handle_identifier(void* data,
                                     struct river_window_v1* obj,
                                     const char* identifier)
{
}

const struct river_window_v1_listener river_window_listener = {
    .closed = window_handle_closed,
    .dimensions_hint = window_handle_dimensions_hint,
    .dimensions = window_handle_dimensions,
    .app_id = window_handle_app_id,
    .title = window_handle_title,
    .parent = window_handle_parent,
    .decoration_hint = window_handle_decoration_hint,
    .pointer_move_requested = window_handle_pointer_move_requested,
    .pointer_resize_requested = window_handle_pointer_resize_requested,
    .show_window_menu_requested = window_handle_show_window_menu_requested,
    .maximize_requested = window_handle_maximize_requested,
    .unmaximize_requested = window_handle_unmaximize_requested,
    .fullscreen_requested = window_handle_fullscreen_requested,
    .exit_fullscreen_requested = window_handle_exit_fullscreen_requested,
    .minimize_requested = window_handle_minimize_requested,
    .unreliable_pid = window_handle_unreliable_pid,
    .presentation_hint = window_handle_presentation_hint,
    .identifier = window_handle_identifier,
};

static void window_maybe_destroy(struct Window* window)
{
    if (!window->closed)
    {
        return;
    }

    struct Seat* seat;
    wl_list_for_each(seat, &wm.seats, link)
    {
        if (seat->focused == window)
        {
            seat->focused = NULL;
        }
        if (seat->op_window == window)
        {
            river_seat_v1_op_end(seat->obj);
            seat->op = SEAT_OP_NONE;
            seat->op_window = NULL;
        }
    }

    river_window_v1_destroy(window->obj);
    wl_list_remove(&window->link);
    free(window);
}

static void window_set_position(struct Window* window, int32_t x, int32_t y)
{
    river_node_v1_set_position(window->node, x, y);
    window->x = x;
    window->y = y;
}

/* Set borders on a window
 *
 * Must be called during a render sequence.
 * focused=1 uses focused border color, focused=0 uses unfocused color.
 */
static void window_set_borders(struct Window* w, int bw, int focused)
{
    const unsigned int* colors = focused ? border_color_focused : border_color_unfocused;

    // Set borders on all four edges
    // https://isaacfreund.com/docs/wayland/river-window-management-v1/#river_window_v1.set_borders
    river_window_v1_set_borders(w->obj,
                                RIVER_WINDOW_V1_EDGES_TOP | RIVER_WINDOW_V1_EDGES_RIGHT |
                                    RIVER_WINDOW_V1_EDGES_BOTTOM | RIVER_WINDOW_V1_EDGES_LEFT,
                                bw,
                                colors[0],  // R
                                colors[1],  // G
                                colors[2],  // B
                                colors[3]   // A
    );
}

/* Resize a window to the given position and dimensions with border width */
void resize(
    struct Window* w, const int x, const int y, const int width, const int height, const int bw)
{
    if (!w) return;

    // Store border width
    w->bw = bw;

    // Update window's position
    window_set_position(w, x, y);

    // Tell River compositor to resize the window
    // The actual dimensions will be set by window_handle_dimensions callback
    river_window_v1_propose_dimensions(w->obj, width, height);

    // Note: Borders are set in the render pass by drawborders()
    // This ensures all windows get correct focus state
}

/* Arrange windows on an output using its current layout
 *
 * This sets / updates the layout symbol for the monitor and calls the layout arrange function
 * (tile, monocle, etc.) to resize and reposition client windows.
 */
void arrange(struct Output* m)
{
    // TODO: if its NULL then rearrange all windows
    if (!m) return;

    // Set the layout symbol from the current layout
    if (m->lt && m->lt->symbol) strncpy(m->ltsymbol, m->lt->symbol, sizeof m->ltsymbol);

    // Call the layout's arrange function if it has one
    // (NULL means floating layout - no automatic arrangement)
    if (m->lt && m->lt->arrange) m->lt->arrange(m);
}

/* Tile layout - master/stack arrangement
 *
 * Master area on the left, stack area on the right:
 *
 * With nmaster=1:
 * ┌───────────┬────┐
 * │           │ W2 │
 * │    W1     ├────┤
 * │  (master) │ W3 │
 * │           ├────┤
 * │           │ W4 │
 * └───────────┴────┘
 *
 * Master width is controlled by mfact (0.5 = 50% of screen)
 */
static void tile(struct Output* m)
{
    /* Variables:
     *    i - iterator, represents number of clients processed
     *    n - total number of clients
     *    h - calculated client height
     *    mw - calculated width of the master area
     *    my - calculated master area y position relative to the window area
     *    ty - calculated stack area y position relative to window area (tile y, the naming is
     *         likely a remnant from a time before the nmaster patch was applied upstream -
     *         before that the master area had only one client and the remaining clients would
     *         be tiled in the tile area)
     *    bw - border width
     */
    unsigned int i, n, h, mw, my, ty, bw;
    struct Window* w;

    /* This loop just counts the number of tiled clients storing the count in the variable n. */
    if (wl_list_empty(&m->clients)) return;
    w = wl_container_of(m->clients.next, w, tile_link);
    for (n = 0, w = nexttiled(w); w;
         w = nexttiled(wl_container_of(w->tile_link.next, w, tile_link)), n++);

    /* If we have no tiled clients then there is nothing to do, stop processing now. */
    if (n == 0) return;

    /* The general idea here is that we have a master area where the master client(s) are tiled
     * and a stack area where the remaining clients are tiled.
     *
     * The number of clients in the master area is controlled using nmaster.
     *
     * In principle the code below is not that complicated, but something that does make it a
     * bit convoluted are the two exceptional cases where:
     *    - nmaster is 0, in which case only the stack area is drawn and
     *    - nmaster is greater than n, in which case only the master area is drawn
     */

    if (n == 1)
        bw = 0;
    else
        bw = borderpx;

    /* If we have enough clients for both the master and the stack area then we split the
     * window area in two by applying the master stack factor (mfact). */
    if (n > nmaster)
        /* But in the exceptional case that nmaster is 0 then we also set the master area
         * width to 0. This because all the clients will be drawn in the stack area, and
         * the stack area subtracts mw from the available width. */
        mw = nmaster ? m->ww * mfact : 0;
    else
        /* If we have less clients than nmaster then all clients will be drawn in the
         * master area and thus the master area takes up the entire window area. */
        mw = m->ww;

    /* This loops through all clients initialising i, the master y (my), and the stack y (ty)
     * to 0 while incrementing i for each client processed. */
    w = wl_container_of(m->clients.next, w, tile_link);
    for (i = my = ty = 0, w = nexttiled(w); w;
         w = nexttiled(wl_container_of(w->tile_link.next, w, tile_link)), i++)

        /* If this client goes into the master area (this includes the case where all
         * clients go into the master area). */
        if (i < nmaster)
        {
            /* Here we calculate the height of the client based on the remaining space
             * and the number of clients left to place.
             *
             *    (m->wh - my)        - the remaining space
             *    MIN(n, nmaster)     - this covers for the exceptional case where
             *                          nmaster is greater than the number of clients,
             *                          imagine if nmaster is 8 and we have 6 clients
             *    (MIN(...) - i)      - the number of remaining clients
             *
             * Putting this together we have that the height h is the remaining space
             * divided by the remaining clients.
             */
            h = (m->wh - my) / (MIN(n, nmaster) - i);

            /* This resizes and positions the client accordingly.
             *
             *    m->wx          - the window area x position
             *    m->wy + my     - the window area y position + master client y position
             *    mw - (2*bw)    - the width of the client, defined earlier to be either
             *                     the entire width of the monitor window area or the
             *                     width of the master area after mfact has been
             *                     applied, we subtract the border width from the size
             *    h - (2*bw)     - the calculated height of the client, we subtract the
             *                     border width from the size
             *    bw             - border width for this window
             */
            resize(w, m->wx, m->wy + my, mw - (2 * bw), h - (2 * bw), bw);

            /* We increment the master y position with the height of the client after
             * the resize so that we know where the next client can be positioned.
             *
             * The if statement is a guard to prevent the my variable growing larger
             * than the window area height, in which case the height calculation above
             * would result in a negative value - and a negative value for an unsigned
             * int results in a really really big number causing a crash. */
            if (my + HEIGHT(w) < m->wh) my += HEIGHT(w);
            /* Otherwise the client goes into the stack area (this includes the case where
             * nmaster is 0 and all clients go into the stack area). */
        }
        else
        {
            /* Here we calculate the height of the client based on the remaining space
             * and the number of clients left to place.
             *
             *    (m->wh - ty)        - the remaining space
             *    (n - i)             - the number of remaining clients
             */
            h = (m->wh - ty) / (n - i);

            /* This resizes and positions the client accordingly.
             *
             *    m->wx + mw        - the window area x position + master width gives the
             *                        stack area x position (mw can be 0)
             *    m->wy + ty        - the window area y position + stack client y position
             *    m->ww - mw        - the width of the client in the stack area is the
             *      - (2*bw)          remaining space after master width has been deducted,
             *                        we subtract the border width from the size
             *    h - (2*bw)        - the calculated height of the client, we subtract the
             *                        border width from the size
             *    bw                - border width for this window
             */
            resize(w, m->wx + mw, m->wy + ty, m->ww - mw - (2 * bw), h - (2 * bw), bw);

            /* We increment the stack y position with the height of the client after
             * the resize so that we know where the next client can be positioned. */
            if (ty + HEIGHT(w) < m->wh) ty += HEIGHT(w);
        }

    /* Now following that how come the implementation is so complicated in that it continuously
     * calculates the remaining space for each client? Why does it not just simply divide the
     * available space by the number of clients and leave it at that?
     *
     * The reason for why it is implemented in this way has specifically to do with size hints
     * in that a client like the simple terminal (st) for example would not be able to utilise
     * all the space given. By default size hints are respected in tiled resizals, and by
     * calculating the size one client at a time and only incrementing by the size that was
     * used after size hints has been applied the space usage is more or less optimised.
     * Another thing to consider is that no matter how you divide the available space there
     * will always be the case where some divisions will give remainder pixels that are not
     * allocated. The way windows are tiled here the last client to be tiled in each respective
     * area will receive the remaining space. This is why the bottom client in the stack area
     * often appears larger than the rest.
     */
}

void monocle(struct Output* m)
{
    unsigned int n = 0; /* number of clients */
    struct Window* w;

    /* This for loop is just to get a count of all visible tiled clients.
     * This number could be used to update a layout symbol in a bar to say e.g. [3].
     */
    if (wl_list_empty(&m->clients)) return;
    w = wl_container_of(m->clients.next, w, tile_link);
    for (w = nexttiled(w); w; w = nexttiled(wl_container_of(w->tile_link.next, w, tile_link)), n++);

    /* The layout symbol of the monitor is only overwritten if there are clients visible
     * on the selected tag(s). Look up snprintf if you are unsure what this does, but the gist
     * of it is that it replaces the %d format inside the string "[%d]" with the value of n
     * (e.g. 3) and writes the output to the monitor layout symbol (m->ltsymbol) and it writes
     * at most 16 bytes (sizeof m->ltsymbol) to that variable.
     */
    if (n > 0) /* override layout symbol */
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);

    /* This just loops through all tiled clients and resizes them to take up the entire window
     * area. Note that this does not have anything to do with which window is shown on top, that
     * is determined by the window that has focus which will be above other tiled windows in the
     * stack.
     */
    w = wl_container_of(m->clients.next, w, tile_link);
    for (w = nexttiled(w); w; w = nexttiled(wl_container_of(w->tile_link.next, w, tile_link)))
        resize(w, m->wx, m->wy, m->ww - 2 * w->bw, m->wh - 2 * w->bw, 0);
}

static void seat_pointer_move(struct Seat* seat, struct Window* window);
static void seat_pointer_resize(struct Seat* seat, struct Window* window, uint32_t edges);

static void window_manage(struct Window* window)
{
    if (window->new)
    {
        window->new = false;
        window_set_position(window, 0, 0);
        river_window_v1_propose_dimensions(window->obj, 0, 0);
    }
    if (window->pointer_move_requested != NULL)
    {
        seat_pointer_move(window->pointer_move_requested, window);
        window->pointer_move_requested = NULL;
    }
    if (window->pointer_resize_requested != NULL)
    {
        seat_pointer_resize(window->pointer_resize_requested,
                            window,
                            window->pointer_resize_requested_edges);
        window->pointer_resize_requested = NULL;
    }
}

static void xkb_binding_handle_pressed(void* data, struct river_xkb_binding_v1* obj)
{
    struct XkbBinding* binding = data;
    binding->seat->pending_action = binding->action;
}

static void xkb_binding_handle_released(void* data, struct river_xkb_binding_v1* obj)
{
}

const struct river_xkb_binding_v1_listener river_xkb_binding_listener = {
    .pressed = xkb_binding_handle_pressed,
    .released = xkb_binding_handle_released,
};

static void xkb_binding_destroy(struct XkbBinding* binding)
{
    river_xkb_binding_v1_destroy(binding->obj);
    wl_list_remove(&binding->link);
    free(binding);
}

static void xkb_binding_create(struct Seat* seat,
                               uint32_t mods,
                               xkb_keysym_t keysym,
                               enum Action action)
{
    struct XkbBinding* binding = ecalloc(1, sizeof(struct XkbBinding));
    binding->obj = river_xkb_bindings_v1_get_xkb_binding(xkb_bindings_v1, seat->obj, keysym, mods);
    binding->seat = seat;
    binding->action = action;

    river_xkb_binding_v1_add_listener(binding->obj, &river_xkb_binding_listener, binding);
    river_xkb_binding_v1_enable(binding->obj);

    wl_list_insert(seat->xkb_bindings.prev, &binding->link);
}

static void pointer_binding_handle_pressed(void* data, struct river_pointer_binding_v1* obj)
{
    struct PointerBinding* binding = data;
    binding->seat->pending_action = binding->action;
}

static void pointer_binding_handle_released(void* data, struct river_pointer_binding_v1* obj)
{
}

const struct river_pointer_binding_v1_listener river_pointer_binding_listener = {
    .pressed = pointer_binding_handle_pressed,
    .released = pointer_binding_handle_released,
};

static void pointer_binding_destroy(struct PointerBinding* binding)
{
    river_pointer_binding_v1_destroy(binding->obj);
    wl_list_remove(&binding->link);
    free(binding);
}

static void pointer_binding_create(struct Seat* seat,
                                   uint32_t mods,
                                   uint32_t button,
                                   enum Action action)
{
    struct PointerBinding* binding = ecalloc(1, sizeof(struct PointerBinding));
    binding->obj = river_seat_v1_get_pointer_binding(seat->obj, button, mods);
    binding->seat = seat;
    binding->action = action;

    river_pointer_binding_v1_add_listener(binding->obj, &river_pointer_binding_listener, binding);
    river_pointer_binding_v1_enable(binding->obj);

    wl_list_insert(seat->pointer_bindings.prev, &binding->link);
}

static void seat_handle_removed(void* data, struct river_seat_v1* obj)
{
    struct Seat* seat = data;
    seat->removed = true;
}

static void seat_handle_pointer_enter(void* data,
                                      struct river_seat_v1* obj,
                                      struct river_window_v1* river_window)
{
    struct Seat* seat = data;
    seat->hovered = river_window_v1_get_user_data(river_window);
}

static void seat_handle_pointer_leave(void* data, struct river_seat_v1* obj)
{
    struct Seat* seat = data;
    seat->hovered = NULL;
}

static void seat_handle_window_interaction(void* data,
                                           struct river_seat_v1* obj,
                                           struct river_window_v1* river_window)
{
    struct Seat* seat = data;
    seat->interacted = river_window_v1_get_user_data(river_window);
}

static void seat_handle_op_delta(void* data, struct river_seat_v1* obj, int32_t dx, int32_t dy)
{
    struct Seat* seat = data;
    seat->op_dx = dx;
    seat->op_dy = dy;
}

static void seat_handle_op_release(void* data, struct river_seat_v1* obj)
{
    struct Seat* seat = data;
    seat->op_release = true;
}

// Ignored events
static void seat_handle_wl_seat(void* data, struct river_seat_v1* obj, uint32_t id)
{
}
static void seat_handle_shell_surface_interaction(
    void* data, struct river_seat_v1* obj, struct river_shell_surface_v1* river_shell_surface)
{
}
static void seat_handle_pointer_position(void* data,
                                         struct river_seat_v1* obj,
                                         int32_t x,
                                         int32_t y)
{
}

const struct river_seat_v1_listener river_seat_listener = {
    .removed = seat_handle_removed,
    .wl_seat = seat_handle_wl_seat,
    .pointer_enter = seat_handle_pointer_enter,
    .pointer_leave = seat_handle_pointer_leave,
    .window_interaction = seat_handle_window_interaction,
    .shell_surface_interaction = seat_handle_shell_surface_interaction,
    .op_delta = seat_handle_op_delta,
    .op_release = seat_handle_op_release,
    .pointer_position = seat_handle_pointer_position,
};

static void seat_maybe_destroy(struct Seat* seat)
{
    if (!seat->removed)
    {
        return;
    }

    struct XkbBinding *xkb_binding, *xkb_binding_tmp;
    wl_list_for_each_safe(xkb_binding, xkb_binding_tmp, &seat->xkb_bindings, link)
    {
        xkb_binding_destroy(xkb_binding);
    }

    struct PointerBinding *pointer_binding, *pointer_binding_tmp;
    wl_list_for_each_safe(pointer_binding, pointer_binding_tmp, &seat->pointer_bindings, link)
    {
        pointer_binding_destroy(pointer_binding);
    }

    river_seat_v1_destroy(seat->obj);
    wl_list_remove(&seat->link);
    free(seat);
}

/* Give focus to a given window. This will window will be placed on top of the stack.
 *
 * If the given window is NULL then it will be given to the first visible window in the stacking
 order. What this means is that the window that last had focus will receive input focus.
 */
static void seat_focus(struct Seat* seat, struct Window* window)
{
    // If no monitor assigned yet, pick the first one
    if (seat->mon == NULL && !wl_list_empty(&wm.outputs))
    {
        seat->mon = wl_container_of(wm.outputs.next, seat->mon, link);
    }

    // If no window specified, try to find the top window from the monitor's stack
    if (window == NULL && seat->mon && !wl_list_empty(&seat->mon->stack))
    {
        // Find first valid window from monitor's stack (most recently focused)
        struct Window* w;
        wl_list_for_each(w, &seat->mon->stack, stack_link)
        {
            if (!w->closed && !w->isfloating)
            {
                window = w;
                break;
            }
        }
    }

    // Already focused - nothing to do
    if (seat->focused == window)
    {
        return;
    }

    if (window != NULL)
    {
        // Update seat's monitor if window is on a different monitor
        if (window->mon && window->mon != seat->mon)
        {
            seat->mon = window->mon;
        }

        // Move to top of stack (focus order)
        // This makes it the most recently focused window
        detachstack(window);
        attachstack(window);

        // Tell River compositor to give it keyboard focus
        river_seat_v1_focus_window(seat->obj, window->obj);

        // Place it visually on top (Z-order)
        river_node_v1_place_top(window->node);

        // Also update global window list order
        wl_list_remove(&window->link);
        wl_list_insert(wm.windows.prev, &window->link);
    }
    else
    {
        // No window to focus - clear focus
        river_seat_v1_clear_focus(seat->obj);
    }

    seat->focused = window;
}

/* User function to move focus up or down the stack
 *
 * This cycles through visible tiled windows in the stack order.
 * inc > 0 moves forward (next window), inc < 0 moves backward (previous window).
 * wl_list is a circular doubly-linked list, so wrapping is automatic.
 */
static void focusstack(struct Seat* seat, int inc)
{
    struct Window* w = NULL;
    struct wl_list* link;

    /* Bail if there is no currently focused window */
    if (!seat->focused) return;

    /* Bail if there's no monitor */
    if (!seat->mon) return;

    /* If the input value is positive then we move forward to find the next visible tiled window. */
    if (inc > 0)
    {
        /* Start from the focused window and iterate forward.
         * List wraps automatically */
        for (link = seat->focused->stack_link.next; link != &seat->focused->stack_link;
             link = link->next)
        {
            /* Skip the list head */
            if (link == &seat->mon->stack) continue;

            w = wl_container_of(link, w, stack_link);

            /* Exit early if the window is not closed and
             * not floating windows */
            if (!w->closed && !w->isfloating) break;
            w = NULL;
        }
    }

    /* Otherwise we move backward to find the prior visible tiled window. */
    else
    {
        /* Start from the focused window and iterate backward
         * List wraps automatically */
        for (link = seat->focused->stack_link.prev; link != &seat->focused->stack_link;
             link = link->prev)
        {
            /* Skip the list head */
            if (link == &seat->mon->stack) continue;

            w = wl_container_of(link, w, stack_link);

            /* Exit early if the window is not closed and
             * not floating windows */
            if (!w->closed && !w->isfloating) break;
            w = NULL;
        }
    }

    /* If we found a window, give it focus */
    if (w && w != seat->focused)
    {
        seat_focus(seat, w);
    }
}

/* User function to adjust the master area factor (mfact)
 *
 * The master area factor controls what percentage of the screen width the master area takes up
 * in the tile layout. For example, mfact=0.5 means the master area takes 50% of the width.
 *
 * This function can be called with either:
 *   - A relative adjustment: f < 1.0 adds to current mfact (e.g., +0.05 increases by 5%)
 *   - An absolute value: f >= 1.0 sets mfact directly (e.g., 1.55 sets to 0.55)
 *
 * The value is clamped to the range [0.05, 0.95] to ensure both master and stack areas
 * remain usable.
 */
static void setmfact(struct Output* m, float f)
{
    float next_mfact; /* The next factor value */

    /* If there's no monitor or the current layout is floating layout (as indicated by
     * having a NULL arrange function as defined in the layouts array), then we do nothing. */
    if (!m || !m->lt || !m->lt->arrange) return;

    /* If the given float argument is less than 1.0 then make a relative adjustment of the mfact
     * value, otherwise set the mfact value absolutely. */
    next_mfact = f < 1.0 ? f + mfact : f - 1.0;

    /* Check that the next factor value is within the bounds of the minimum of 0.05 and the
     * maximum of 0.95. If it is not then we bail out here */
    if (next_mfact < 0.05 || next_mfact > 0.95) return;

    /* Set the master / stack factor to the new value */
    mfact = next_mfact;

    /* This makes a call to arrange so that the tiled windows are resized and repositioned
     * following the change to the master / stack factor. In principle this could have been a
     * call directly to the tile function as all that is needed is for the clients to be tiled
     * again.
     *
     * The call to arrange is a catch all that can prevent obscure issues and the performance
     * overhead is negligible.
     */
    arrange(m);
}

/* User function to spawn a command
 *
 * This forks a new process and executes the given command. The child process is set up
 * with its own session and default signal handlers so programs start cleanly.
 *
 * @called_from seat_action when a spawn key binding is triggered
 */
void spawn(const char* const* argv)
{
    struct sigaction sa;

    /* Bail if no command provided */
    if (!argv || !argv[0])
        return;

    /* This call to fork creates a new (duplicate) process of the current process.
     *
     * For the parent process, fork() returns the child's PID and we return immediately.
     * For the child process, fork() returns 0 and we enter the if statement.
     */
    if (fork() == 0) {
        /* Close the Wayland display connection before proceeding. The child inherits
         * the parent's file descriptors and we don't want the child holding onto the
         * Wayland connection. */
        if (wm.display)
            close(wl_display_get_fd(wm.display));

        /* The call to setsid creates a new session and sets the process group ID. This is
         * needed because a child created via fork inherits its parent's session ID and we
         * need our own because this session ID will be preserved across the execvp call. */
        setsid();

        /* This restores SIGCHLD sighandler to default before spawning a program.
         *
         * From sigaction(2):
         * A child created via fork(2) inherits a copy of its parent's signal dispositions.
         * During an execve(2), the dispositions of handled signals are reset to the default;
         * the dispositions of ignored signals are left unchanged.
         *
         * The reason why this is needed is that some programs would not start due to inheriting
         * the signal handler of tinyrwm which ignores SIGCHLD. */
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = SIG_DFL;
        sigaction(SIGCHLD, &sa, NULL);

        /* The execvp causes the program that is currently being run (tinyrwm in this case) to
         * be replaced with a new program and with a newly initialised stack, heap and data
         * segments. If this is successful then this is the last thing this process does in
         * the tinyrwm code. */
        execvp(argv[0], (char* const*)argv);

        /* If the execvp fails for whatever reason, then we are still here executing tinyrwm
         * code. So we print an error and call exit to ensure that this process stops running. */
        fprintf(stderr, "tinyrwm: execvp '%s' failed: %s\n", argv[0], strerror(errno));
        exit(1);
    }
}

static void seat_pointer_move(struct Seat* seat, struct Window* window)
{
    seat_focus(seat, window);
    river_seat_v1_op_start_pointer(seat->obj);
    seat->op = SEAT_OP_MOVE;
    seat->op_window = window;
    seat->op_start_x = window->x;
    seat->op_start_y = window->y;
    seat->op_dx = 0;
    seat->op_dy = 0;
}

static void seat_pointer_resize(struct Seat* seat, struct Window* window, uint32_t edges)
{
    seat_focus(seat, window);
    river_window_v1_inform_resize_start(window->obj);
    river_seat_v1_op_start_pointer(seat->obj);
    seat->op = SEAT_OP_RESIZE;
    seat->op_window = window;
    seat->op_edges = edges;
    seat->op_start_x = window->x;
    seat->op_start_y = window->y;
    seat->op_start_width = window->w;
    seat->op_start_height = window->h;
    seat->op_dx = 0;
    seat->op_dy = 0;
}

static void seat_action(struct Seat* seat, enum Action action)
{
    switch (action)
    {
        case ACTION_NONE:
            break;
        case ACTION_SPAWN_FOOT:
            if (fork() == 0)
            {
                execlp("foot", "foot", (char*)0);
            }
            break;
        case ACTION_CLOSE:
            if (seat->focused != NULL)
            {
                river_window_v1_close(seat->focused->obj);
            }
            break;
        case ACTION_FOCUS_NEXT:
            if (!wl_list_empty(&wm.windows))
            {
                // Focus the bottom window
                struct Window* window = wl_container_of(wm.windows.next, window, link);
                seat_focus(seat, window);
            }
            break;
        case ACTION_MOVE:
            if (seat->op == SEAT_OP_NONE && seat->hovered != NULL)
            {
                seat_pointer_move(seat, seat->hovered);
            }
            break;
        case ACTION_RESIZE:
            if (seat->op == SEAT_OP_NONE && seat->hovered != NULL)
            {
                seat_pointer_resize(seat,
                                    seat->hovered,
                                    RIVER_WINDOW_V1_EDGES_BOTTOM | RIVER_WINDOW_V1_EDGES_RIGHT);
            }
            break;
        case ACTION_EXIT:
            river_window_manager_v1_exit_session(window_manager_v1);
            break;
    }
}

static void seat_manage(struct Seat* seat)
{
    if (seat->new)
    {
        seat->new = false;
        const uint32_t super = RIVER_SEAT_V1_MODIFIERS_MOD4;
        xkb_binding_create(seat, super, XKB_KEY_space, ACTION_SPAWN_FOOT);
        xkb_binding_create(seat, super, XKB_KEY_q, ACTION_CLOSE);
        xkb_binding_create(seat, super, XKB_KEY_n, ACTION_FOCUS_NEXT);
        xkb_binding_create(seat, super, XKB_KEY_Escape, ACTION_EXIT);
        pointer_binding_create(seat, super, BTN_LEFT, ACTION_MOVE);
        pointer_binding_create(seat, super, BTN_RIGHT, ACTION_RESIZE);
    }

    // If no window was interacted with in the current manage sequence,
    // intentionally pass NULL to ensure the window on top has focus.
    // This is necessary to handle new windows for example.
    seat_focus(seat, seat->interacted);
    seat->interacted = NULL;

    seat_action(seat, seat->pending_action);
    seat->pending_action = ACTION_NONE;

    switch (seat->op)
    {
        case SEAT_OP_NONE:
            break;
        case SEAT_OP_MOVE:
            if (seat->op_release)
            {
                river_seat_v1_op_end(seat->obj);
                seat->op = SEAT_OP_NONE;
                seat->op_window = NULL;
                break;
            }
            break;
        case SEAT_OP_RESIZE:
            if (seat->op_release)
            {
                river_window_v1_inform_resize_end(seat->op_window->obj);
                river_seat_v1_op_end(seat->obj);
                seat->op = SEAT_OP_NONE;
                seat->op_window = NULL;
                break;
            }
            int32_t width = seat->op_start_width;
            int32_t height = seat->op_start_height;
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_LEFT) != 0)
            {
                width -= seat->op_dx;
            }
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_RIGHT) != 0)
            {
                width += seat->op_dx;
            }
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_TOP) != 0)
            {
                height -= seat->op_dy;
            }
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_BOTTOM) != 0)
            {
                height += seat->op_dy;
            }
            river_window_v1_propose_dimensions(seat->op_window->obj,
                                               width > 1 ? width : 1,
                                               height > 1 ? height : 1);
            break;
    }
    seat->op_release = false;
}

static void seat_render(struct Seat* seat)
{
    switch (seat->op)
    {
        case SEAT_OP_NONE:
            break;
        case SEAT_OP_MOVE:
            window_set_position(seat->op_window,
                                seat->op_start_x + seat->op_dx,
                                seat->op_start_y + seat->op_dy);
            break;
        case SEAT_OP_RESIZE:;
            int32_t x = seat->op_start_x;
            int32_t y = seat->op_start_y;
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_LEFT) != 0)
            {
                x += seat->op_start_width - seat->op_window->w;
            }
            if ((seat->op_edges & RIVER_WINDOW_V1_EDGES_TOP) != 0)
            {
                y += seat->op_start_height - seat->op_window->h;
            }
            window_set_position(seat->op_window, x, y);
            break;
    }
}

static void wm_handle_unavailable(void* data, struct river_window_manager_v1* obj)
{
    fprintf(stderr, "error: another window manager is already running\n");
    exit(1);
}

static void wm_handle_finished(void* data, struct river_window_manager_v1* obj)
{
    exit(0);
}

static void wm_handle_manage_start(void* data, struct river_window_manager_v1* obj)
{
    // Destroy closed windows and removed outputs/seats
    struct Output *output, *output_tmp;
    wl_list_for_each_safe(output, output_tmp, &wm.outputs, link)
    {
        output_maybe_destroy(output);
    }
    struct Window *window, *window_tmp;
    wl_list_for_each_safe(window, window_tmp, &wm.windows, link)
    {
        window_maybe_destroy(window);
    }
    struct Seat *seat, *seat_tmp;
    wl_list_for_each_safe(seat, seat_tmp, &wm.seats, link)
    {
        seat_maybe_destroy(seat);
    }

    // Carry out window management policy
    wl_list_for_each(window, &wm.windows, link)
    {
        window_manage(window);
    }
    wl_list_for_each(seat, &wm.seats, link)
    {
        seat_manage(seat);
    }

    river_window_manager_v1_manage_finish(window_manager_v1);
}

/* Draw borders for all windows, setting colors based on focus state
 *
 * This must be called during the render sequence to ensure the River
 * compositor applies the border styling.
 */
static void drawborders(void)
{
    struct Window* w;
    struct Seat* seat;

    // For each window, check if any seat has it focused
    wl_list_for_each(w, &wm.windows, link)
    {
        if (w->closed) continue;

        int focused = 0;

        // Check if this window is focused by any seat
        wl_list_for_each(seat, &wm.seats, link)
        {
            if (seat->focused == w)
            {
                focused = 1;
                break;
            }
        }

        // Set borders with current width and appropriate color
        window_set_borders(w, w->bw, focused);
    }
}

static void wm_handle_render_start(void* data, struct river_window_manager_v1* obj)
{
    struct Seat* seat;
    wl_list_for_each(seat, &wm.seats, link)
    {
        seat_render(seat);
    }

    // Update borders for all windows based on current focus state
    drawborders();

    river_window_manager_v1_render_finish(window_manager_v1);
}

static void wm_handle_window(void* data,
                             struct river_window_manager_v1* obj,
                             struct river_window_v1* river_window)
{
    struct Window* window = ecalloc(1, sizeof(struct Window));
    window->obj = river_window;
    window->node = river_window_v1_get_node(window->obj);
    window->new = true;

    // Initialize border width to default
    window->bw = borderpx;

    // Initialize the window's list links
    wl_list_init(&window->tile_link);
    wl_list_init(&window->stack_link);

    // Assign to first available monitor
    if (!wl_list_empty(&wm.outputs))
    {
        window->mon = wl_container_of(wm.outputs.next, window->mon, link);
    }
    else
    {
        window->mon = NULL;
    }

    river_window_v1_add_listener(window->obj, &river_window_listener, window);

    // Add to global window list
    wl_list_insert(wm.windows.prev, &window->link);

    // If we have a monitor, attach to its lists
    if (window->mon)
    {
        attach(window);
        attachstack(window);
    }
}

static void wm_handle_output(void* data,
                             struct river_window_manager_v1* obj,
                             struct river_output_v1* river_output)
{
    struct Output* output = ecalloc(1, sizeof(struct Output));
    output->obj = river_output;

    // Initialize the client list (tile order) and stack list (focus order)
    wl_list_init(&output->clients);
    wl_list_init(&output->stack);

    // Set default layout (first in layouts array - tiling)
    output->lt = &layouts[0];

    // Initialize layout symbol from the default layout
    if (output->lt && output->lt->symbol)
        strncpy(output->ltsymbol, output->lt->symbol, sizeof output->ltsymbol);

    river_output_v1_add_listener(output->obj, &river_output_listener, output);

    wl_list_insert(wm.outputs.prev, &output->link);
}

static void wm_handle_seat(void* data,
                           struct river_window_manager_v1* obj,
                           struct river_seat_v1* river_seat)
{
    struct Seat* seat = ecalloc(1, sizeof(struct Seat));
    seat->obj = river_seat;
    seat->new = true;
    seat->mon = NULL;  // Will be set to first output when needed
    wl_list_init(&seat->xkb_bindings);
    wl_list_init(&seat->pointer_bindings);

    river_seat_v1_add_listener(seat->obj, &river_seat_listener, seat);

    wl_list_insert(wm.seats.prev, &seat->link);
}

// Ignored events
static void wm_handle_session_locked(void* data, struct river_window_manager_v1* obj)
{
}
static void wm_handle_session_unlocked(void* data, struct river_window_manager_v1* obj)
{
}

static const struct river_window_manager_v1_listener wm_listener = {
    .unavailable = wm_handle_unavailable,
    .finished = wm_handle_finished,
    .manage_start = wm_handle_manage_start,
    .render_start = wm_handle_render_start,
    .session_locked = wm_handle_session_locked,
    .session_unlocked = wm_handle_session_unlocked,
    .window = wm_handle_window,
    .output = wm_handle_output,
    .seat = wm_handle_seat,
};

static void wm_init(void)
{
    wl_list_init(&wm.outputs);
    wl_list_init(&wm.windows);
    wl_list_init(&wm.seats);
}

static void handle_global(void* data,
                          struct wl_registry* registry,
                          uint32_t name,
                          const char* interface,
                          uint32_t version)
{
    if (strcmp(interface, river_window_manager_v1_interface.name) == 0)
    {
        if (version >= 4)
        {
            window_manager_v1 =
                wl_registry_bind(registry, name, &river_window_manager_v1_interface, 4);
        }
    }
    else if (strcmp(interface, river_xkb_bindings_v1_interface.name) == 0)
    {
        xkb_bindings_v1 = wl_registry_bind(registry, name, &river_xkb_bindings_v1_interface, 1);
    }
}

static void handle_global_remove(void* data, struct wl_registry* registry, uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

int main(void)
{
    struct wl_display* display = wl_display_connect(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "failed to connect to Wayland server\n");
        return 1;
    }

    // Store display in global state for spawn() to access
    wm.display = display;

    // Avoid passing WAYLAND_DEBUG on to our children.
    // It only matters if it's set when the display is created.
    unsetenv("WAYLAND_DEBUG");

    // Ensure children are automatically reaped.
    signal(SIGCHLD, SIG_IGN);

    struct wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    if (wl_display_roundtrip(display) < 0)
    {
        fprintf(stderr, "roundtrip failed\n");
        return 1;
    }

    if (window_manager_v1 == NULL || xkb_bindings_v1 == NULL)
    {
        fprintf(stderr,
                "river_window_manager_v1 or river_xkb_bindings_v1 "
                "not supported by the Wayland server\n");
        return 1;
    }

    wm_init();

    river_window_manager_v1_add_listener(window_manager_v1, &wm_listener, NULL);

    while (true)
    {
        if (wl_display_dispatch(display) < 0)
        {
            fprintf(stderr, "dispatch failed\n");
            return 1;
        }
    }

    return 0;
}
