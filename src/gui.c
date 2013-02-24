/*
 ** GUI code
 ** (c) 2009-2013 by Robert Manea et al.
*/

#include "gui.h"

#include "uzbl-core.h"
#include "events.h"
#include "menu.h"
#include "type.h"
#include "variables.h" /* FIXME: This is for get_geometry which   *
                        *        should probably be in this file. */

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glib.h>

#include <limits.h>

static void
uzbl_status_bar_init (void);
static void
uzbl_web_view_init (void);
static void
uzbl_vbox_init (void);
static void
uzbl_window_init (void);
static void
uzbl_plug_init (void);

void
uzbl_gui_init (gboolean plugmode)
{
    uzbl_status_bar_init ();
    uzbl_web_view_init ();
    uzbl_vbox_init ();

    if (plugmode) {
        uzbl_plug_init ();
    } else {
        uzbl_window_init ();
    }
}

void
set_window_property (const gchar *prop, const gchar *value)
{
    if (uzbl.gui.main_window && GTK_IS_WIDGET (uzbl.gui.main_window))
    {
        gdk_property_change (
            gtk_widget_get_window (GTK_WIDGET (uzbl.gui.main_window)),
            gdk_atom_intern_static_string (prop),
            gdk_atom_intern_static_string ("STRING"),
            CHAR_BIT * sizeof(value[0]),
            GDK_PROP_MODE_REPLACE,
            (const guchar *)value,
            strlen (value));
    }
}

static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data);
static gboolean
key_release_cb (GtkWidget *widget, GdkEventKey *event, gpointer data);

void
uzbl_status_bar_init (void)
{
    uzbl.gui.status_bar = uzbl_status_bar_new ();

    g_object_connect (G_OBJECT (uzbl.gui.status_bar),
        "signal::key-press-event",   G_CALLBACK (key_press_cb), NULL,
        "signal::key-release-event", G_CALLBACK (key_press_cb), NULL,
        NULL);

    /* TODO: What should be done with these?
    g_object_connect (G_OBJECT (UZBL_STATUS_BAR (uzbl.gui.status_bar)->label_left),
        "signal::key-press-event",   G_CALLBACK (key_press_cb),   NULL,
        "signal::key-release-event", G_CALLBACK (key_release_cb), NULL,
        NULL);

    g_object_connect (G_OBJECT (UZBL_STATUS_BAR (uzbl.gui.status_bar)->label_right),
        "signal::key-press-event",   G_CALLBACK (key_press_cb),   NULL,
        "signal::key-release-event", G_CALLBACK (key_release_cb), NULL,
        NULL);
      */
}

/* Mouse events */
static gboolean
button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean
button_release_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
static void
link_hover_cb (WebKitWebView *view, const gchar *title, const gchar *link, gpointer data);
/* Page metadata events */
static void
title_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data);
static void
progress_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data);
static void
load_status_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data);
static void
uri_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data);
static gboolean
load_error_cb (WebKitWebView *view, WebKitWebFrame *frame, gchar *uri, gpointer web_err, gpointer data);
static void
window_object_cleared_cb (WebKitWebView *view, WebKitWebFrame *frame,
        JSGlobalContextRef *context, JSObjectRef *object, gpointer data);
/* Navigation events */
static gboolean
navigation_decision_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action,
        WebKitWebPolicyDecision *policy_decision, gpointer data);
static gboolean
new_window_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action,
        WebKitWebPolicyDecision *policy_decision, gpointer data);
static gboolean
mime_policy_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, gchar *mime_type,
        WebKitWebPolicyDecision *policy_decision, gpointer data);
/* FIXME: Make private again.
static gboolean
download_cb (WebKitWebView *view, WebKitDownload *download, gpointer data);
*/
static void
request_starting_cb (WebKitWebView *view, WebKitWebFrame *frame, WebKitWebResource *resource,
        WebKitNetworkRequest *request, WebKitNetworkResponse *response, gpointer data);
/* UI events */
static WebKitWebView *
create_web_view_cb (WebKitWebView *view, WebKitWebFrame *frame, gpointer data);
static void
close_web_view_cb (WebKitWebView *view, gpointer data);
static gboolean
focus_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data);
#if WEBKIT_CHECK_VERSION (1, 9, 0)
static gboolean
context_menu_cb (WebKitWebView *view, GtkMenu *menu, WebKitHitTestResult *hit_test_result,
        gboolean triggered_with_keyboard, gpointer data);
#else
static gboolean
populate_popup_cb (WebKitWebView *view, GtkMenu *menu, gpointer data);
#endif
/* Scrollbar events */
static gboolean
scroll_vert_cb (GtkAdjustment *adjust, gpointer data);
static gboolean
scroll_horiz_cb (GtkAdjustment *adjust, gpointer data);

void
uzbl_web_view_init (void)
{
    uzbl.gui.web_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());
    uzbl.gui.scrolled_win = gtk_scrolled_window_new (NULL, NULL);

    gtk_container_add (
        GTK_CONTAINER (uzbl.gui.scrolled_win),
        GTK_WIDGET (uzbl.gui.web_view)
    );

    g_object_connect (G_OBJECT (uzbl.gui.web_view),
        /* Keyboard events */
        "signal::key-press-event",                      G_CALLBACK (key_press_cb),             NULL,
        "signal::key-release-event",                    G_CALLBACK (key_release_cb),           NULL,
        /* Mouse events */
        "signal::button-press-event",                   G_CALLBACK (button_press_cb),          NULL,
        "signal::button-release-event",                 G_CALLBACK (button_release_cb),        NULL,
        "signal::hovering-over-link",                   G_CALLBACK (link_hover_cb),            NULL,
        /* Page metadata events */
        "signal::notify::title",                        G_CALLBACK (title_change_cb),          NULL,
        "signal::notify::progress",                     G_CALLBACK (progress_change_cb),       NULL,
        "signal::notify::load-status",                  G_CALLBACK (load_status_change_cb),    NULL,
        "signal::notify::uri",                          G_CALLBACK (uri_change_cb),            NULL,
        "signal::load-error",                           G_CALLBACK (load_error_cb),            NULL,
        "signal::window-object-cleared",                G_CALLBACK (window_object_cleared_cb), NULL,
        /* Navigation events */
        "signal::navigation-policy-decision-requested", G_CALLBACK (navigation_decision_cb),   NULL,
        "signal::new-window-policy-decision-requested", G_CALLBACK (new_window_cb),            NULL,
        "signal::mime-type-policy-decision-requested",  G_CALLBACK (mime_policy_cb),           NULL,
        "signal::download-requested",                   G_CALLBACK (download_cb),              NULL,
        "signal::resource-request-starting",            G_CALLBACK (request_starting_cb),      NULL,
        /* UI events */
        "signal::create-web-view",                      G_CALLBACK (create_web_view_cb),       NULL,
        "signal::close-web-view",                       G_CALLBACK (close_web_view_cb),        NULL,
        "signal::focus-in-event",                       G_CALLBACK (focus_cb),                 NULL,
        "signal::focus-out-event",                      G_CALLBACK (focus_cb),                 NULL,
#if WEBKIT_CHECK_VERSION (1, 9, 0)
        "signal::context-menu",                         G_CALLBACK (context_menu_cb),          NULL,
#else
        "signal::populate-popup",                       G_CALLBACK (populate_popup_cb),        NULL,
#endif
        NULL);

    uzbl.gui.bar_h = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (uzbl.gui.scrolled_win));
    uzbl.gui.bar_v = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (uzbl.gui.scrolled_win));

    g_object_connect (G_OBJECT (uzbl.gui.bar_v),
        "signal::value-changed", G_CALLBACK (scroll_vert_cb), NULL,
        "signal::changed",       G_CALLBACK (scroll_vert_cb), NULL,
        NULL);
    g_object_connect (G_OBJECT (uzbl.gui.bar_h),
        "signal::value-changed", G_CALLBACK (scroll_horiz_cb), NULL,
        "signal::changed",       G_CALLBACK (scroll_horiz_cb), NULL,
        NULL);

}

void
uzbl_vbox_init (void)
{
#if GTK_CHECK_VERSION (3, 0, 0)
    uzbl.gui.vbox = gtk_box_new (FALSE, 0);
    gtk_orientable_set_orientation (GTK_ORIENTABLE (uzbl.gui.vbox), GTK_ORIENTATION_VERTICAL);
#else
    uzbl.gui.vbox = gtk_vbox_new (FALSE, 0);
#endif

    gtk_box_pack_start (GTK_BOX (uzbl.gui.vbox), uzbl.gui.scrolled_win, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (uzbl.gui.vbox), uzbl.gui.status_bar, FALSE, TRUE, 0);
}

static void
destroy_cb (GtkWidget *widget, gpointer data);
static gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data);

void
uzbl_window_init (void)
{
    uzbl.gui.main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    /* TODO: Plumb through from command line. */
    gtk_window_set_default_size (GTK_WINDOW (uzbl.gui.main_window), 800, 600);
    gtk_window_set_title (GTK_WINDOW (uzbl.gui.main_window), "Uzbl");
    gtk_widget_set_name (GTK_WIDGET (uzbl.gui.main_window), "Uzbl");

#if GTK_CHECK_VERSION (3, 0, 0)
    /* TODO: Make into an option? */
    gtk_window_set_has_resize_grip (GTK_WINDOW (uzbl.gui.main_window), FALSE);
#endif

    /* Fill in the main window */
    gtk_container_add (GTK_CONTAINER (uzbl.gui.main_window), uzbl.gui.vbox);

    g_object_connect (G_OBJECT (uzbl.gui.main_window),
        "signal::destroy",         G_CALLBACK (destroy_cb),         NULL,
        "signal::configure-event", G_CALLBACK (configure_event_cb), NULL,
        NULL);
}

void
uzbl_plug_init (void)
{
    uzbl.gui.plug = GTK_PLUG (gtk_plug_new (uzbl.state.socket_id));

    gtk_widget_set_name (GTK_WIDGET (uzbl.gui.plug), "Uzbl");

    gtk_container_add (GTK_CONTAINER (uzbl.gui.plug), uzbl.gui.vbox);

    g_object_connect (G_OBJECT (uzbl.gui.plug),
        /* FIXME: Should we really quit GTK if the plug is destroyed? */
        "signal::destroy",           G_CALLBACK (destroy_cb),   NULL,
        "signal::key-press-event",   G_CALLBACK (key_press_cb), NULL,
        "signal::key-release-event", G_CALLBACK (key_press_cb), NULL,
        NULL);
}

/* Status bar callbacks */

gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (data);

    if (event->type == GDK_KEY_PRESS) {
        key_to_event (event->keyval, event->state, event->is_modifier, GDK_KEY_PRESS);
    }

    return (uzbl.behave.forward_keys ? FALSE : TRUE);
}

gboolean
key_release_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (data);

    if (event->type == GDK_KEY_RELEASE) {
        key_to_event (event->keyval, event->state, event->is_modifier, GDK_KEY_RELEASE);
    }

    return (uzbl.behave.forward_keys ? FALSE : TRUE);
}

/* Web view callbacks */

/* Mouse events */

#define NO_CLICK_CONTEXT -1
static gint
get_click_context (void);

gboolean
button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (data);

    gint context;
    gboolean propagate = FALSE;
    gboolean sendev    = FALSE;
    gboolean is_editable = FALSE;
    gboolean is_document = FALSE;

    /* Save last button click for use in menu */
    if (uzbl.state.last_button) {
        gdk_event_free ((GdkEvent *)uzbl.state.last_button);
    }
    uzbl.state.last_button = (GdkEventButton *)gdk_event_copy ((GdkEvent *)event);

    /* Grab context from last click */
    context = get_click_context ();

    is_editable = (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE);
    is_document = (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_DOCUMENT);

    if (event->type == GDK_BUTTON_PRESS) {
        /* left click */
        if (event->button == 1) {
            if (is_editable) {
                send_event (FORM_ACTIVE, NULL,
                    TYPE_NAME, "button1",
                    NULL);
            } else if (is_document) {
                send_event (ROOT_ACTIVE, NULL,
                    TYPE_NAME, "button1",
                    NULL);
            } else {
                sendev    = TRUE;
                propagate = TRUE;
            }
        } else if ((event->button == 2) && !is_editable) {
            sendev    = TRUE;
            propagate = TRUE;
        } else if (event->button == 3) {
            /* Ignore middle click */
        } else if (event->button > 3) {
            sendev    = TRUE;
            propagate = TRUE;
        }
    }

    if ((event->type == GDK_2BUTTON_PRESS) || (event->type == GDK_3BUTTON_PRESS)) {
        if ((event->button == 1) && !is_editable && is_document) {
            sendev    = TRUE;
            propagate = uzbl.state.handle_multi_button;
        } else if ((event->button == 2) && !is_editable) {
            sendev    = TRUE;
            propagate = uzbl.state.handle_multi_button;
        } else if (event->button >= 3) {
            sendev    = TRUE;
            propagate = uzbl.state.handle_multi_button;
        }
    }

    if (sendev) {
        button_to_event (event->button, event->state, event->type);
    }

    return propagate;
}

gboolean
button_release_cb (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (data);

    gint context;
    gboolean propagate = FALSE;
    gboolean sendev    = FALSE;
    gboolean is_editable = FALSE;

    context = get_click_context ();

    is_editable = (context & WEBKIT_HIT_TEST_RESULT_CONTEXT_EDITABLE);

    if (event->type == GDK_BUTTON_RELEASE) {
        if ((event->button == 2) && !is_editable) {
            sendev    = TRUE;
            propagate = TRUE;
        } else if (event->button == 3) {
            /* Ignore middle click */
        } else if (event->button > 3) {
            sendev    = TRUE;
            propagate = TRUE;
        }

        if (sendev) {
            button_to_event (event->button, event->state, GDK_BUTTON_RELEASE);
        }
    }

    return propagate;
}

gint
get_click_context ()
{
    WebKitHitTestResult *ht;
    guint context;

    if (!uzbl.state.last_button) {
        return NO_CLICK_CONTEXT;
    }

    ht = webkit_web_view_get_hit_test_result (uzbl.gui.web_view, uzbl.state.last_button);
    g_object_get (ht, "context", &context, NULL);
    g_object_unref (ht);

    return (gint)context;
}

void
link_hover_cb (WebKitWebView *view, const gchar *title, const gchar *link, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (data);

    if (uzbl.state.last_selected_url) {
        g_free (uzbl.state.last_selected_url);
    }

    if (uzbl.state.selected_url) {
        uzbl.state.last_selected_url = g_strdup (uzbl.state.selected_url);
        g_free (uzbl.state.selected_url);
        uzbl.state.selected_url = NULL;
    } else {
        uzbl.state.last_selected_url = NULL;
    }

    if (uzbl.state.last_selected_url && g_strcmp0 (link, uzbl.state.last_selected_url)) {
        send_event (LINK_UNHOVER, NULL,
            TYPE_STR, uzbl.state.last_selected_url,
            NULL);
    }

    if (link) {
        uzbl.state.selected_url = g_strdup (link);
        send_event (LINK_HOVER, NULL,
            TYPE_STR, uzbl.state.selected_url,
            TYPE_STR, title,
            NULL);
    }

    update_title ();
}

/* Page metadata events */

void
title_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    const gchar *title = webkit_web_view_get_title (view);

    if (uzbl.gui.main_title) {
        g_free (uzbl.gui.main_title);
    }
    uzbl.gui.main_title = title ? g_strdup (title) : g_strdup ("(no title)");

    update_title ();

    send_event (TITLE_CHANGED, NULL,
        TYPE_STR, uzbl.gui.main_title,
        NULL);
    /* TODO: Collect all environment settings into one place. */
    g_setenv ("UZBL_TITLE", uzbl.gui.main_title, TRUE);
}

void
progress_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    int progress = 100 * webkit_web_view_get_progress (view);

    send_event (LOAD_PROGRESS, NULL,
        TYPE_INT, progress,
        NULL);
}

void
load_status_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    WebKitLoadStatus status;

    status = webkit_web_view_get_load_status (view);

    switch (status) {
        case WEBKIT_LOAD_PROVISIONAL:
            send_event (LOAD_START, NULL,
                TYPE_STR, uzbl.state.uri ? uzbl.state.uri : "",
                NULL);
            break;
        case WEBKIT_LOAD_COMMITTED:
        {
            WebKitWebFrame *frame = webkit_web_view_get_main_frame (view);
            send_event (LOAD_COMMIT, NULL,
                TYPE_STR, webkit_web_frame_get_uri (frame),
                NULL);
            break;
        }
        case WEBKIT_LOAD_FINISHED:
            send_event (LOAD_FINISH, NULL,
                TYPE_STR, uzbl.state.uri,
                NULL);
            break;
        case WEBKIT_LOAD_FIRST_VISUALLY_NON_EMPTY_LAYOUT:
            /* TODO: Implement */
            break;
        case WEBKIT_LOAD_FAILED:
            /* Handled by load_error_cb */
            break;
        default:
            uzbl_debug ("Unrecognized load status: %d\n", status);
            break;
    }
}

void
uri_change_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    g_free (uzbl.state.uri);
    g_object_get (view, "uri", &uzbl.state.uri, NULL);

    /* TODO: Collect all environment settings into one place. */
    g_setenv ("UZBL_URI", uzbl.state.uri, TRUE);
    set_window_property ("UZBL_URI", uzbl.state.uri);
}

gboolean
load_error_cb (WebKitWebView *view, WebKitWebFrame *frame, gchar *uri, gpointer web_err, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (frame);
    UZBL_UNUSED (data);

    GError *err = web_err;

    send_event (LOAD_ERROR, NULL,
        TYPE_STR, uri,
        TYPE_INT, err->code,
        TYPE_STR, err->message,
        NULL);

    return FALSE;
}

#if WEBKIT_CHECK_VERSION (1, 3, 13)
static void
dom_focus_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data);
static void
dom_blur_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data);
#endif

void
window_object_cleared_cb (WebKitWebView *view, WebKitWebFrame *frame,
        JSGlobalContextRef *context, JSObjectRef *object, gpointer data)
{
    UZBL_UNUSED (frame);
    UZBL_UNUSED (context);
    UZBL_UNUSED (object);
    UZBL_UNUSED (data);

#if WEBKIT_CHECK_VERSION (1, 3, 13)
    /* Take this opportunity to set some callbacks on the DOM */
    WebKitDOMDocument *document = webkit_web_view_get_dom_document (view);
    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (document),
        "focus", G_CALLBACK (dom_focus_cb), TRUE, NULL);
    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (document),
        "blur",  G_CALLBACK (dom_blur_cb), TRUE, NULL);
#else
    UZBL_UNUSED (view);
#endif
}


#if WEBKIT_CHECK_VERSION (1, 3, 13)
void
dom_focus_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data)
{
    UZBL_UNUSED (target);
    UZBL_UNUSED (data);

    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    send_event (FOCUS_ELEMENT, NULL,
        TYPE_STR, name,
        NULL);
}

void
dom_blur_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data)
{
    UZBL_UNUSED (target);
    UZBL_UNUSED (data);

    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    send_event (BLUR_ELEMENT, NULL,
        TYPE_STR, name,
        NULL);
}
#endif

/* Navigation events */

/* TODO: Put into utils.h */
static void
remove_trailing_newline (const char *line);

gboolean
navigation_decision_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action,
        WebKitWebPolicyDecision *policy_decision, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (frame);
    UZBL_UNUSED (navigation_action);
    UZBL_UNUSED (data);

    const gchar *uri = webkit_network_request_get_uri (request);
    gboolean decision_made = FALSE;

    uzbl_debug ("Navigation requested -> %s\n", uri);

    if (uzbl.behave.scheme_handler) {
        GString *result = g_string_new ("");
        GArray *args = g_array_new (TRUE, FALSE, sizeof (gchar *));
        const UzblCommandInfo *scheme_command = parse_command_parts (uzbl.behave.scheme_handler, args);

        if (scheme_command) {
            g_array_append_val (args, uri);
            run_parsed_command (scheme_command, args, result);
        }
        g_array_free (args, TRUE);

        if (result->len > 0) {
            remove_trailing_newline (result->str);

            if (!g_strcmp0 (result->str, "USED")) {
                webkit_web_policy_decision_ignore (policy_decision);
                decision_made = TRUE;
            }
        }

        g_string_free (result, TRUE);
    }

    if (!decision_made) {
        webkit_web_policy_decision_use (policy_decision);
    }

    return TRUE;
}

gboolean
new_window_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, WebKitWebNavigationAction *navigation_action,
        WebKitWebPolicyDecision *policy_decision, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (frame);
    UZBL_UNUSED (data);

    const gchar *uri = webkit_network_request_get_uri (request);
    const gchar *target_frame = webkit_web_navigation_action_get_target_frame (navigation_action);

    uzbl_debug ("New window requested -> %s\n", uri);

    /* The create_web_view_cb is also called for _blank targets. */
    if (!g_strcmp0 (target_frame, "_blank")) {
        send_event (NEW_WINDOW, NULL,
            TYPE_STR, uri,
            NULL);
    } else {
        uzbl_debug ("Ignoring new window request; target == _blank\n");
    }

    webkit_web_policy_decision_ignore (policy_decision);

    return TRUE;
}

gboolean
mime_policy_cb (WebKitWebView *view, WebKitWebFrame *frame,
        WebKitNetworkRequest *request, gchar *mime_type,
        WebKitWebPolicyDecision *policy_decision, gpointer data)
{
    UZBL_UNUSED (frame);
    UZBL_UNUSED (request);
    UZBL_UNUSED (data);

    /* TODO: Ignore based on external filter program? */

    /* If we can display it, let's display it... */
    if (webkit_web_view_can_show_mime_type (view, mime_type)) {
        webkit_web_policy_decision_use (policy_decision);
    } else {
        /* ...everything we can't display is downloaded */
        webkit_web_policy_decision_download (policy_decision);
    }

    return TRUE;
}

static void
download_progress_cb (WebKitDownload *download, GParamSpec *param_spec, gpointer data);
static void
download_status_cb (WebKitDownload *download, GParamSpec *param_spec, gpointer data);

gboolean
download_cb (WebKitWebView *view, WebKitDownload *download, gpointer data)
{
    UZBL_UNUSED (view);

    /* get the URI being downloaded */
    const gchar *uri = webkit_download_get_uri (download);

    /* TODO: Split this logic into a separate function. */
    /* get the destination path, if specified.
     * this is only intended to be set when this function is trigger by an
     * explicit download using uzbl's 'download' action. */
    const gchar *destination = (const gchar *)data;

    uzbl_debug ("Download requested -> %s\n", uri);

    if (!uzbl.behave.download_handler) {
        /* reject downloads when there's no download handler */
        uzbl_debug ("No download handler set; ignoring download\n");
        webkit_download_cancel (download);
        return FALSE;
    }

    /* get a reasonable suggestion for a filename */
    const gchar *suggested_filename;
    g_object_get (download, "suggested-filename", &suggested_filename, NULL);

    /* get the mimetype of the download */
    const gchar *content_type = NULL;
    WebKitNetworkResponse *response = webkit_download_get_network_response (download);
    /* downloads can be initiated from the context menu, in that case there is
       no network response yet and trying to get one would crash. */
    if (WEBKIT_IS_NETWORK_RESPONSE (response)) {
        SoupMessage        *message = webkit_network_response_get_message (response);
        SoupMessageHeaders *headers = NULL;
        g_object_get (message, "response-headers", &headers, NULL);
        /* some versions of libsoup don't have "response-headers" here */
        if (headers) {
            content_type = soup_message_headers_get_one (headers, "Content-Type");
        }
    }

    if (!content_type) {
        content_type = "application/octet-stream";
    }

    /* get the filesize of the download, as given by the server.
       (this may be inaccurate, there's nothing we can do about that.) */
    unsigned int total_size = webkit_download_get_total_size (download);

    GArray *args = g_array_new (TRUE, FALSE, sizeof (gchar *));
    const UzblCommandInfo *download_command = parse_command_parts (uzbl.behave.download_handler, args);
    if (!download_command) {
        webkit_download_cancel (download);
        g_array_free (args, TRUE);
        return FALSE;
    }

    g_array_append_val (args, uri);
    g_array_append_val (args, suggested_filename);
    g_array_append_val (args, content_type);
    gchar *total_size_s = g_strdup_printf ("%d", total_size);
    g_array_append_val (args, total_size_s);

    if (destination) {
        g_array_append_val (args, destination);
    }

    GString *result = g_string_new ("");
    run_parsed_command (download_command, args, result);

    g_free (total_size_s);
    g_array_free (args, TRUE);

    /* no response, cancel the download */
    if (result->len == 0) {
        webkit_download_cancel (download);
        return FALSE;
    }

    /* we got a response, it's the path we should download the file to */
    gchar *destination_path = result->str;
    g_string_free (result, FALSE);

    /* presumably people don't need newlines in their filenames. */
    remove_trailing_newline (destination_path);

    /* set up progress callbacks */
    g_object_connect (G_OBJECT (download),
        "signal::notify::status",   G_CALLBACK (download_status_cb),   NULL,
        NULL);
    g_object_connect (G_OBJECT (download),
        "signal::notify::progress", G_CALLBACK (download_progress_cb), NULL,
        NULL);

    /* convert relative path to absolute path */
    if (destination_path[0] != '/') {
        /* TODO: Detect file:// URI from the handler */
        gchar *rel_path = destination_path;
        gchar *cwd = g_get_current_dir ();
        destination_path = g_strconcat (cwd, "/", destination_path, NULL);
        g_free (cwd);
        g_free (rel_path);
    }

    send_event (DOWNLOAD_STARTED, NULL,
        TYPE_STR, destination_path,
        NULL);

    /* convert absolute path to file:// URI */
    gchar *destination_uri = g_strconcat ("file://", destination_path, NULL);
    g_free (destination_path);

    webkit_download_set_destination_uri (download, destination_uri);
    g_free (destination_uri);

    return TRUE;
}

void
download_progress_cb (WebKitDownload *download, GParamSpec *param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    gdouble progress;
    g_object_get (download, "progress", &progress, NULL);

    const gchar *dest_uri = webkit_download_get_destination_uri (download);
    const gchar *dest_path = dest_uri + strlen ("file://");

    send_event (DOWNLOAD_PROGRESS, NULL,
        TYPE_STR, dest_path,
        TYPE_FLOAT, progress,
        NULL);
}

void
download_status_cb (WebKitDownload *download, GParamSpec *param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    WebKitDownloadStatus status;
    g_object_get (download, "status", &status, NULL);

    switch (status) {
        case WEBKIT_DOWNLOAD_STATUS_CREATED:
        case WEBKIT_DOWNLOAD_STATUS_STARTED:
            break; /* these are irrelevant */
        case WEBKIT_DOWNLOAD_STATUS_ERROR:
        case WEBKIT_DOWNLOAD_STATUS_CANCELLED:
            /* TODO: Implement events for these. */
            break;
        case WEBKIT_DOWNLOAD_STATUS_FINISHED:
        {
            const gchar *dest_uri = webkit_download_get_destination_uri (download);
            const gchar *dest_path = dest_uri + strlen ("file://");
            send_event (DOWNLOAD_COMPLETE, NULL,
                TYPE_STR, dest_path,
                NULL);
            break;
        }
    }
}

void
request_starting_cb (WebKitWebView *view, WebKitWebFrame *frame, WebKitWebResource *resource,
        WebKitNetworkRequest *request, WebKitNetworkResponse *response, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (frame);
    UZBL_UNUSED (resource);
    UZBL_UNUSED (response);
    UZBL_UNUSED (data);

    const gchar *uri = webkit_network_request_get_uri (request);
    SoupMessage *message = webkit_network_request_get_message (request);

    uzbl_debug ("Request starting -> %s\n", uri);

    if (message) {
        SoupURI *soup_uri = soup_uri_new (uri);
        soup_message_set_first_party (message, soup_uri);
    }

    send_event (REQUEST_STARTING, NULL,
        TYPE_STR, uri,
        NULL);

    if (uzbl.behave.request_handler) {
        GString *result = g_string_new ("");
        GArray *args = g_array_new (TRUE, FALSE, sizeof (gchar *));
        const UzblCommandInfo *request_command = parse_command_parts (uzbl.behave.request_handler, args);

        if (request_command) {
            g_array_append_val (args, uri);
            run_parsed_command (request_command, args, result);
        }
        g_array_free (args, TRUE);

        if (result->len > 0) {
            remove_trailing_newline (result->str);

            webkit_network_request_set_uri (request, result->str);
        }

        g_string_free (result, TRUE);
    }
}

/* UI events */

static void
create_web_view_js_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data);

WebKitWebView *
create_web_view_cb (WebKitWebView *view, WebKitWebFrame *frame, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (frame);
    UZBL_UNUSED (data);

    WebKitWebView *new_view = WEBKIT_WEB_VIEW (webkit_web_view_new ());

    uzbl_debug ("New web view -> javascript link...\n");

    g_object_connect (G_OBJECT (new_view),
        "signal::notify::uri", G_CALLBACK (create_web_view_js_cb), NULL,
        NULL);

    return new_view;
}

#define strprefix(str, prefix) \
    strncmp ((str), (prefix), strlen ((prefix)))

void
create_web_view_js_cb (WebKitWebView *view, GParamSpec param_spec, gpointer data)
{
    UZBL_UNUSED (param_spec);
    UZBL_UNUSED (data);

    webkit_web_view_stop_loading (view);
    const gchar *uri = webkit_web_view_get_uri (view);

    static const char *js_protocol = "javascript:";

    if (strprefix (uri, js_protocol) == 0) {
        eval_js (uzbl.gui.web_view, (gchar *)uri + strlen (js_protocol), NULL, js_protocol);
        gtk_widget_destroy (GTK_WIDGET (view));
    } else {
        send_event (NEW_WINDOW, NULL,
            TYPE_STR, uri,
            NULL);
    }
}

void
close_web_view_cb (WebKitWebView *view, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (data);

    send_event (CLOSE_WINDOW, NULL,
        NULL);
}

gboolean
focus_cb (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (event);
    UZBL_UNUSED (data);

    send_event (event->in ? FOCUS_GAINED : FOCUS_LOST, NULL,
        NULL);

    return FALSE;
}

static gboolean
populate_context_menu (GtkMenu *menu, WebKitHitTestResult *hit_test_result, gint context);

#if WEBKIT_CHECK_VERSION (1, 9, 0)
gboolean
context_menu_cb (WebKitWebView *view, GtkMenu *menu, WebKitHitTestResult *hit_test_result,
        gboolean triggered_with_keyboard, gpointer data)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (triggered_with_keyboard);
    UZBL_UNUSED (data);

    gint context;

    if (!uzbl.gui.menu_items) {
        return FALSE;
    }

    context = get_click_context ();

    /* check context */
    if (context == NO_CLICK_CONTEXT) {
        return FALSE;
    }

    /* display the default menu with our modifications. */
    return populate_context_menu (menu, hit_test_result, context);
}
#else
gboolean
populate_popup_cb (WebKitWebView *view, GtkMenu *menu, gpointer data)
{
    UZBL_UNUSED (data);

    gint context;

    if (!uzbl.gui.menu_items) {
        return FALSE;
    }

    context = get_click_context ();

    /* check context */
    if (context == NO_CLICK_CONTEXT) {
        return FALSE;
    }

    WebKitHitTestResult *hit_test_result;
    GdkEventButton event;
    gint x;
    gint y;
#if GTK_CHECK_VERSION (3, 0, 0)
    gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (view)),
        gdk_device_manager_get_client_pointer (
            gdk_display_get_device_manager (
                gtk_widget_get_display (GTK_WIDGET (v)))),
        &x, &y, NULL);
#else
    gdk_window_get_pointer (gtk_widget_get_window (GTK_WIDGET (view)), &x, &y, NULL);
#endif
    event.x = x;
    event.y = y;
    hit_test_result = webkit_web_view_get_hit_test_result (view, &event);

    gboolean ret = populate_context_menu (menu, hit_test_result, context);

    g_object_unref (hit_test_result);

    return ret;
}
#endif

static void
run_menu_command (GtkMenuItem *menu_item, gpointer data);

gboolean
populate_context_menu (GtkMenu *menu, WebKitHitTestResult *hit_test_result, gint context)
{
    guint i;

    /* find the user-defined menu items that are approprate for whatever was
     * clicked and append them to the default context menu. */
    for (i = 0; i < uzbl.gui.menu_items->len; ++i) {
        UzblMenuItem *menu_item = g_ptr_array_index (uzbl.gui.menu_items, i);
        GtkWidget *item;

        gboolean contexts_match = (context & menu_item->context);

        if (!contexts_match) {
            continue;
        }

        gboolean is_image = (menu_item->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE);

        if (is_image) {
            g_object_get (hit_test_result, "image-uri", &menu_item->argument, NULL);
        }

        if (menu_item->issep) {
            item = gtk_separator_menu_item_new ();
        } else {
            item = gtk_menu_item_new_with_label (menu_item->name);
            g_object_connect (G_OBJECT (item),
                "signal::activate", G_CALLBACK (run_menu_command), menu_item,
                NULL);
        }

        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
        gtk_widget_show (item);
    }

    return FALSE;
}

void
run_menu_command (GtkMenuItem *menu_item, gpointer data)
{
    UZBL_UNUSED (menu_item);

    UzblMenuItem *item = (UzblMenuItem *)data;

    gboolean is_image = (item->context & WEBKIT_HIT_TEST_RESULT_CONTEXT_IMAGE);

    if (is_image) {
        gchar *cmd = g_strdup_printf ("%s %s", item->cmd, item->argument);

        parse_cmd_line (cmd, NULL);

        g_free (cmd);
        g_free (item->argument);
    } else {
        parse_cmd_line (item->cmd, NULL);
    }
}

/* Scrollbar events */

static void
send_scroll_event (int type, GtkAdjustment *adjust);

gboolean
scroll_vert_cb (GtkAdjustment *adjust, gpointer data)
{
    UZBL_UNUSED (data);

    send_scroll_event (SCROLL_VERT, adjust);

    return FALSE;
}

gboolean
scroll_horiz_cb (GtkAdjustment *adjust, gpointer data)
{
    UZBL_UNUSED (data);

    send_scroll_event (SCROLL_HORIZ, adjust);

    return FALSE;
}

void
send_scroll_event (int type, GtkAdjustment *adjust)
{
    gdouble value = gtk_adjustment_get_value (adjust);
    gdouble min = gtk_adjustment_get_lower (adjust);
    gdouble max = gtk_adjustment_get_upper (adjust);
    gdouble page = gtk_adjustment_get_page_size (adjust);

    send_event (type, NULL,
        TYPE_FLOAT, value,
        TYPE_FLOAT, min,
        TYPE_FLOAT, max,
        TYPE_FLOAT, page,
        NULL);
}

/* Window callbacks */

void
destroy_cb (GtkWidget *widget, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (data);

    gtk_main_quit ();
}

gboolean
configure_event_cb (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
    UZBL_UNUSED (widget);
    UZBL_UNUSED (event);
    UZBL_UNUSED (data);

    gchar *last_geo    = uzbl.gui.geometry;
    /* TODO: We should set the geometry instead. */
    gchar *current_geo = get_geometry ();

    if (!last_geo || g_strcmp0 (last_geo, current_geo)) {
        send_event (GEOMETRY_CHANGED, NULL,
            TYPE_STR, current_geo,
            NULL);
    }

    g_free (current_geo);

    return FALSE;
}

void
remove_trailing_newline (const char *line)
{
    char *p = strchr (line, '\n' );
    if (p != NULL) {
        *p = '\0';
    }
}
