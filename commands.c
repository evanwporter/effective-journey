#include "tinyrwm.h"

#include <river-window-management-v1-client-protocol.h>
#include <stddef.h>

#include "config.h"

/* Keybinding action functions
 * These are the functions called by keybindings defined in config.h
 */

void spawn_terminal(struct Seat* seat, const Arg* arg) {
    const char* termcmd[] = { "foot", NULL };
    spawn(termcmd);
}

void close_window(struct Seat* seat, const Arg* arg) {
    if (seat->focused != NULL) {
        river_window_v1_close(seat->focused->obj);
    }
}

void focus_stack(struct Seat* seat, const Arg* arg) {
    focusstack(seat, arg->i);
}

void set_master_fact(struct Seat* seat, const Arg* arg) {
    if (seat->mon != NULL) {
        setmfact(seat->mon, arg->f);
    }
}

void exit_wm(struct Seat* seat, const Arg* arg) {
    river_window_manager_v1_exit_session(window_manager_v1);
}
