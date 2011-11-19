/*
 ** Uzbl event routines
 ** (c) 2009 by Robert Manea
*/

#include <glib.h>

#include "uzbl-core.h"
#include "events.h"
#include "util.h"
#include "type.h"

/* Event id to name mapping
 * Event names must be in the same
 * order as in 'enum event_type'
 *
 * TODO: Add more useful events
*/
const char *event_table[LAST_EVENT] = {
     "LOAD_START"       ,
     "LOAD_COMMIT"      ,
     "LOAD_FINISH"      ,
     "LOAD_ERROR"       ,
     "REQUEST_STARTING" ,
     "KEY_PRESS"        ,
     "KEY_RELEASE"      ,
     "MOD_PRESS"        ,
     "MOD_RELEASE"      ,
     "COMMAND_EXECUTED" ,
     "LINK_HOVER"       ,
     "TITLE_CHANGED"    ,
     "GEOMETRY_CHANGED" ,
     "WEBINSPECTOR"     ,
     "NEW_WINDOW"       ,
     "SELECTION_CHANGED",
     "VARIABLE_SET"     ,
     "FIFO_SET"         ,
     "SOCKET_SET"       ,
     "INSTANCE_START"   ,
     "INSTANCE_EXIT"    ,
     "LOAD_PROGRESS"    ,
     "LINK_UNHOVER"     ,
     "FORM_ACTIVE"      ,
     "ROOT_ACTIVE"      ,
     "FOCUS_LOST"       ,
     "FOCUS_GAINED"     ,
     "FILE_INCLUDED"    ,
     "PLUG_CREATED"     ,
     "COMMAND_ERROR"    ,
     "BUILTINS"         ,
     "PTR_MOVE"         ,
     "SCROLL_VERT"      ,
     "SCROLL_HORIZ"     ,
     "DOWNLOAD_STARTED" ,
     "DOWNLOAD_PROGRESS",
     "DOWNLOAD_COMPLETE",
     "ADD_COOKIE"       ,
     "DELETE_COOKIE"    ,
     "FOCUS_ELEMENT"    ,
     "BLUR_ELEMENT"
};


void
send_event_stdout(GString *msg);

void
send_event_socket(GString *msg);

struct _Event {
    int type;
    const gchar *name;
    GString *message;
    GArray *arguments;
};

Event*
event_new(int type, const gchar *custom_event) {
    if (type >= LAST_EVENT)
        return NULL;

    Event *event = g_new (Event, 1);
    event->type = type;
    event->name = custom_event ? custom_event : event_table[type];

    event->message = g_string_sized_new (512);
    g_string_printf (event->message, "EVENT [%s] %s",
        uzbl.state.instance_name, event->name);

    event->arguments = g_array_new (true, false, sizeof (char*));

    return event;
}

void
event_free(Event *event) {
    g_string_free (event->message, TRUE);
    for (unsigned int i = 0; i < event->arguments->len; i++) {
        g_free (g_array_index (event->arguments, char*, i));
    }
    g_array_free (event->arguments, true);
    g_free (event);
}


void
event_add_atom_argument(Event *event, const gchar *arg) {
    char *dup = g_strdup (arg);
    g_assert (valid_name (arg));

    g_string_append_c(event->message, ' ');
    g_array_append_val (event->arguments, dup);
    g_string_append (event->message, arg);
}

void
event_add_string_argument(Event *event, const gchar *arg) {
    char *dup = g_strdup (arg);
    g_array_append_val (event->arguments, dup);

    g_string_append_c(event->message, ' ');
    g_string_append_c (event->message, '\'');
    append_escaped (event->message, arg);
    g_string_append_c (event->message, '\'');
}

void
event_add_int_argument(Event *event, const int arg) {
    g_string_append_c(event->message, ' ');
    char *end = event->message->str + event->message->len;
    g_string_append_printf (event->message, "%d", arg);
    char *dup = g_strdup (end);
    g_array_append_val (event->arguments, dup);
}

void
event_add_float_argument(Event *event, const double arg) {
    g_string_append_c(event->message, ' ');
    // Make sure the formatted double fits in the buffer
    if (event->message->allocated_len - event->message->len < G_ASCII_DTOSTR_BUF_SIZE)
        g_string_set_size (event->message, event->message->len + G_ASCII_DTOSTR_BUF_SIZE);

    // format in C locale
    char *tmp = g_ascii_formatd (
        event->message->str + event->message->len,
        event->message->allocated_len - event->message->len,
        "%.2f", arg);
    event->message->len += strlen(tmp);
    char *dup = g_strdup (tmp);

    g_array_append_val (event->arguments, dup);
}

void
event_add_argument(Event *event, int type, ...) {
    va_list vargs;
    va_start (vargs, type);
    switch (type) {
    case TYPE_INT:
        event_add_int_argument (event, va_arg (vargs, int));
        break;

    case TYPE_FLOAT:
        event_add_float_argument (event, va_arg (vargs, double));
        break;

    case TYPE_STR:
        event_add_string_argument (event, va_arg (vargs, char*));
        break;

    case TYPE_NAME:
        event_add_atom_argument (event, va_arg (vargs, char*));
        break;
    }
    va_end (vargs);
}

void
event_send(const Event *event) {
    // A event string is not supposed to contain newlines as it will be
    // interpreted as two events
    if (!strchr(event->message->str, '\n')) {
        g_string_append_c(event->message, '\n');

        if (uzbl.state.events_stdout)
            send_event_stdout (event->message);
        send_event_socket (event->message);
    }
}

void
vsend_event(int type, const gchar *custom_event, va_list vargs) {
    Event *event = event_new (type, custom_event);
    if (event == NULL)
        return;

    int next;
    while ((next = va_arg (vargs, int)) != 0) {
        switch (next) {
        case TYPE_INT:
            event_add_int_argument (event, va_arg (vargs, int));
            break;

        case TYPE_FLOAT:
            // ‘float’ is promoted to ‘double’ when passed through ‘...’
            event_add_float_argument (event, va_arg (vargs, double));
            break;

        case TYPE_STR:
            event_add_string_argument (event, va_arg (vargs, char*));
            break;

        case TYPE_NAME:
            event_add_atom_argument (event, va_arg (vargs, char*));
            break;
        }
    }

    event_send (event);
    event_free (event);
}

void
send_event(int type, const gchar *custom_event, ...) {
    va_list vargs, vacopy;
    va_start (vargs, custom_event);
    va_copy (vacopy, vargs);
    vsend_event (type, custom_event, vacopy);
    va_end (vacopy);
    va_end (vargs);
}

void
event_buffer_timeout(guint sec) {
    struct itimerval t;
    memset(&t, 0, sizeof t);
    t.it_value.tv_sec = sec;
    t.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &t, NULL);
}

static void
send_event_sockets(GPtrArray *sockets, GString *msg) {
    GError *error = NULL;
    GIOStatus ret;
    gsize len;
    guint i=0;

    while(i < sockets->len) {
        GIOChannel *gio = g_ptr_array_index(sockets, i++);

        if(gio && gio->is_writeable && msg) {
            ret = g_io_channel_write_chars (gio,
                    msg->str, msg->len,
                    &len, &error);

            if (ret == G_IO_STATUS_ERROR) {
                g_warning ("Error sending event to socket: %s", error->message);
                g_clear_error (&error);
            } else {
                if (g_io_channel_flush(gio, &error) == G_IO_STATUS_ERROR) {
                    g_warning ("Error flushing: %s", error->message);
                    g_clear_error (&error);
                }
            }
        }
    }
}

void
replay_buffered_events() {
    guint i = 0;

    event_buffer_timeout(0);

    /* replay buffered events */
    while(i < uzbl.state.event_buffer->len) {
        GString *tmp = g_ptr_array_index(uzbl.state.event_buffer, i++);
        send_event_sockets(uzbl.comm.connect_chan, tmp);
        g_string_free(tmp, TRUE);
    }

    g_ptr_array_free(uzbl.state.event_buffer, TRUE);
    uzbl.state.event_buffer = NULL;
}

void
send_event_socket(GString *msg) {
    /* write to all --connect-socket sockets */
    if(uzbl.comm.connect_chan) {
        send_event_sockets(uzbl.comm.connect_chan, msg);
        if(uzbl.state.event_buffer)
            replay_buffered_events();
    }
    /* buffer events until a socket is set and connected
    * or a timeout is encountered
    */
    else {
        if(!uzbl.state.event_buffer)
            uzbl.state.event_buffer = g_ptr_array_new();
        g_ptr_array_add(uzbl.state.event_buffer, (gpointer)g_string_new(msg->str));
    }

    /* write to all client sockets */
    if(msg && uzbl.comm.client_chan) {
        send_event_sockets(uzbl.comm.client_chan, msg);
    }
}

void
send_event_stdout(GString *msg) {
    printf("%s", msg->str);
    fflush(stdout);
}

gchar *
get_modifier_mask(guint state) {
    GString *modifiers = g_string_new("");

    if(state & GDK_MODIFIER_MASK) {
        if(state & GDK_SHIFT_MASK)
            g_string_append(modifiers, "Shift|");
        if(state & GDK_LOCK_MASK)
            g_string_append(modifiers, "ScrollLock|");
        if(state & GDK_CONTROL_MASK)
            g_string_append(modifiers, "Ctrl|");
        if(state & GDK_MOD1_MASK)
            g_string_append(modifiers,"Mod1|");
        /* Mod2 is usually Num_Luck. Ignore it as it messes up keybindings.
        if(state & GDK_MOD2_MASK)
            g_string_append(modifiers,"Mod2|");
        */
        if(state & GDK_MOD3_MASK)
            g_string_append(modifiers,"Mod3|");
        if(state & GDK_MOD4_MASK)
            g_string_append(modifiers,"Mod4|");
        if(state & GDK_MOD5_MASK)
            g_string_append(modifiers,"Mod5|");
        if(state & GDK_BUTTON1_MASK)
            g_string_append(modifiers,"Button1|");
        if(state & GDK_BUTTON2_MASK)
            g_string_append(modifiers,"Button2|");
        if(state & GDK_BUTTON3_MASK)
            g_string_append(modifiers,"Button3|");
        if(state & GDK_BUTTON4_MASK)
            g_string_append(modifiers,"Button4|");
        if(state & GDK_BUTTON5_MASK)
            g_string_append(modifiers,"Button5|");

        if(modifiers->str[modifiers->len-1] == '|')
            g_string_truncate(modifiers, modifiers->len-1);
    }

    return g_string_free(modifiers, FALSE);
}

/* backwards compatibility. */
#if ! GTK_CHECK_VERSION (2, 22, 0)
#define GDK_KEY_Shift_L GDK_Shift_L
#define GDK_KEY_Shift_R GDK_Shift_R
#define GDK_KEY_Control_L GDK_Control_L
#define GDK_KEY_Control_R GDK_Control_R
#define GDK_KEY_Alt_L GDK_Alt_L
#define GDK_KEY_Alt_R GDK_Alt_R
#define GDK_KEY_Super_L GDK_Super_L
#define GDK_KEY_Super_R GDK_Super_R
#define GDK_KEY_ISO_Level3_Shift GDK_ISO_Level3_Shift
#endif

guint key_to_modifier(guint keyval) {
    /* FIXME
     * Should really use XGetModifierMapping and/or Xkb to get actual mod keys
     */
    switch(keyval) {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
        return GDK_SHIFT_MASK;
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
        return GDK_CONTROL_MASK;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
        return GDK_MOD1_MASK;
    case GDK_KEY_Super_L:
    case GDK_KEY_Super_R:
        return GDK_MOD4_MASK;
    case GDK_KEY_ISO_Level3_Shift:
        return GDK_MOD5_MASK;
    default:
        return 0;
    }
}

guint
button_to_modifier (guint buttonval) {
    if(buttonval <= 5)
        return 1 << (7 + buttonval);
    return 0;
}

/* Transform gdk key events to our own events */
void
key_to_event(guint keyval, guint state, guint is_modifier, gint mode) {
    gchar ucs[7];
    gint ulen;
    gchar *keyname;
    guint32 ukval = gdk_keyval_to_unicode(keyval);
    gchar *modifiers = NULL;
    guint mod = key_to_modifier (keyval);

    /* Get modifier state including this key press/release */
    modifiers = get_modifier_mask(mode == GDK_KEY_PRESS ? state | mod : state & ~mod);

    if(is_modifier && mod) {
        send_event(mode == GDK_KEY_PRESS ? MOD_PRESS : MOD_RELEASE, NULL,
            TYPE_STR, modifiers,
            TYPE_NAME, get_modifier_mask (mod),
            NULL);
    }
    /* check for printable unicode char */
    /* TODO: Pass the keyvals through a GtkIMContext so that
     *       we also get combining chars right
    */
    else if(g_unichar_isgraph(ukval)) {
        ulen = g_unichar_to_utf8(ukval, ucs);
        ucs[ulen] = 0;

        send_event(mode == GDK_KEY_PRESS ? KEY_PRESS : KEY_RELEASE, NULL,
            TYPE_STR, modifiers, TYPE_STR, ucs, NULL);
    }
    /* send keysym for non-printable chars */
    else if((keyname = gdk_keyval_name(keyval))){
        send_event(mode == GDK_KEY_PRESS ? KEY_PRESS : KEY_RELEASE, NULL,
            TYPE_STR, modifiers, TYPE_NAME, keyname, NULL);
    }

    g_free(modifiers);
}

/* Transform gdk button events to our own events */
void
button_to_event(guint buttonval, guint state, gint mode) {
    gchar *details;
    const char *reps;
    gchar *modifiers = NULL;
    guint mod = button_to_modifier (buttonval);

    /* Get modifier state including this button press/release */
    modifiers = get_modifier_mask(mode != GDK_BUTTON_RELEASE ? state | mod : state & ~mod);

    switch (mode) {
        case GDK_2BUTTON_PRESS:
            reps = "2";
            break;
        case GDK_3BUTTON_PRESS:
            reps = "3";
            break;
        default:
            reps = "";
            break;
    }

    details = g_strdup_printf("%sButton%d", reps, buttonval);

    send_event(mode == GDK_BUTTON_PRESS ? KEY_PRESS : KEY_RELEASE, NULL,
        TYPE_STR, modifiers, TYPE_FORMATTEDSTR, details, NULL);

    g_free(details);
    g_free(modifiers);
}

/* vi: set et ts=4: */
