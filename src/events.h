/*
 ** Uzbl event routines
 ** (c) 2009 by Robert Manea
*/

#ifndef __EVENTS__
#define __EVENTS__

#include <glib.h>
#include <stdarg.h>

/* Event system */
enum event_type {
    LOAD_START, LOAD_COMMIT, LOAD_FINISH, LOAD_ERROR,
    REQUEST_STARTING,
    KEY_PRESS, KEY_RELEASE, MOD_PRESS, MOD_RELEASE,
    COMMAND_EXECUTED,
    LINK_HOVER, TITLE_CHANGED, GEOMETRY_CHANGED,
    WEBINSPECTOR, NEW_WINDOW, SELECTION_CHANGED,
    VARIABLE_SET, FIFO_SET, SOCKET_SET,
    INSTANCE_START, INSTANCE_EXIT, LOAD_PROGRESS,
    LINK_UNHOVER, FORM_ACTIVE, ROOT_ACTIVE,
    FOCUS_LOST, FOCUS_GAINED, FILE_INCLUDED,
    PLUG_CREATED, COMMAND_ERROR, BUILTINS,
    PTR_MOVE, SCROLL_VERT, SCROLL_HORIZ,
    DOWNLOAD_STARTED, DOWNLOAD_PROGRESS, DOWNLOAD_COMPLETE,
    ADD_COOKIE, DELETE_COOKIE,
    FOCUS_ELEMENT, BLUR_ELEMENT,

    /* must be last entry */
    LAST_EVENT
};

typedef struct _Event Event;
struct _Event;

/**
 * Allocate a new event empty event of the given type
 */
Event *
event_new(int type, const gchar *custom_event);

/**
 * Add an argument to the event
 */
void
event_add_argument(Event *event, int type, ...);

void
event_add_atom_argument(Event *event, const gchar *arg);

void
event_add_string_argument(Event *event, const gchar *arg);

void
event_add_int_argument(Event *event, const int arg);

void
event_add_float_argument(Event *event, const double arg);

/**
 * Send the event over the supported interfaces
 */
void
event_send(const Event *event);

/**
 * Frees a event string
 */
void
event_free(Event *event);

/**
 * Shorthand for constructing an event and sending over the supported
 * interfaces.
 */
void
send_event(int type, const gchar *custom_event, ...) G_GNUC_NULL_TERMINATED;

void
vsend_event(int type, const gchar *custom_event, va_list vargs);

/**
 * Misc. functions related to events
 */

void
event_buffer_timeout(guint sec);

void
replay_buffered_events();

gchar *
get_modifier_mask(guint state);

void
key_to_event(guint keyval, guint state, guint is_modifier, gint mode);

void
button_to_event(guint buttonval, guint state, gint mode);

#endif
