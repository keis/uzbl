/*
 * Command code
*/

#include "commands.h"

#include "events.h"
#include "gui.h"
#include "menu.h"
#include "soup.h"
#include "type.h"
#include "util.h"
#include "variables.h"

typedef void (*UzblCommand)(WebKitWebView *view, GArray *argv, GString *result);

struct _UzblCommandInfo {
    const gchar *name;
    UzblCommand  function;
    gboolean     split;
};

/* Navigation commands */
static void
cmd_back (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_forward (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_reload (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_reload_force (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_stop (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_uri (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_auth (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_download (WebKitWebView *view, GArray *argv, GString *result);

/* Cookie commands */
static void
cmd_cookie_add (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_cookie_delete (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_cookie_clear (WebKitWebView *view, GArray *argv, GString *result);

/* Display commands */
static void
cmd_scroll (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_zoom_in (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_zoom_out (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_zoom_toggle_type (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_status_toggle (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_hardcopy (WebKitWebView *view, GArray *argv, GString *result);

/* Search commands */
static void
cmd_search_forward (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_search_reverse (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_search_clear (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_search_reset (WebKitWebView *view, GArray *argv, GString *result);

/* Inspector commands */
static void
cmd_inspector_show (WebKitWebView *view, GArray *argv, GString *result);

/* Execution commands */
/* FIXME: Make private again.
void
cmd_js (WebKitWebView *view, GArray *argv, GString *result);
void
cmd_js_file (WebKitWebView *view, GArray *argv, GString *result);
*/
static void
cmd_spawn (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_spawn_sync (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_spawn_sync_exec (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_spawn_sh (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_spawn_sh_sync (WebKitWebView *view, GArray *argv, GString *result);

/* Uzbl commands */
static void
cmd_chain (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_include (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_exit (WebKitWebView *view, GArray *argv, GString *result);

/* Variable commands */
static void
cmd_set (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_toggle (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_dump_config (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_dump_config_as_events (WebKitWebView *view, GArray *argv, GString *result);
static void
cmd_print (WebKitWebView *view, GArray *argv, GString *result);

/* Event commands */
static void
event (WebKitWebView *view, GArray *argv, GString *result);
static void
event (WebKitWebView *view, GArray *argv, GString *result);

static UzblCommandInfo
builtin_command_table[] =
{   /* name                             function                      split */
    /* Navigation commands */
    { "back",                           cmd_back,                     TRUE  },
    { "forward",                        cmd_forward,                  TRUE  },
    { "reload",                         cmd_reload,                   TRUE  },
    { "reload_ign_cache",               cmd_reload_force,             TRUE  }, /* TODO: Rename to "reload_force". */
    { "stop",                           cmd_stop,                     TRUE  },
    { "uri",                            cmd_uri,                      FALSE },
    { "auth",                           cmd_auth,                     TRUE  },
    { "download",                       cmd_download,                 TRUE  },

    /* Cookie commands */
    { "add_cookie",                     cmd_cookie_add,               TRUE  }, /* TODO: Rework to be "cookie add". */
    { "delete_cookie",                  cmd_cookie_delete,            TRUE  }, /* TODO: Rework to be "cookie delete". */
    { "clear_cookies",                  cmd_cookie_clear,             TRUE  }, /* TODO: Rework to be "cookie clear". */

    /* Display commands */
    { "scroll",                         cmd_scroll,                   TRUE  },
    { "zoom_in",                        cmd_zoom_in,                  TRUE  }, /* TODO: Rework to be "zoom in". */
    { "zoom_out",                       cmd_zoom_out,                 TRUE  }, /* TODO: Rework to be "zoom out". */
    { "toggle_zoom_type",               cmd_zoom_toggle_type,         TRUE  }, /* XXX: Deprecated (toggle). */
    { "toggle_status",                  cmd_status_toggle,            TRUE  }, /* XXX: Deprecated (toggle). */
    { "hardcopy",                       cmd_hardcopy,                 FALSE },

    /* Menu commands */
    { "menu_add",                       cmd_menu_add,                 FALSE }, /* TODO: Rework to be "menu add". */
    { "menu_link_add",                  cmd_menu_add_link,            FALSE }, /* TODO: Rework to be "menu add link". */
    { "menu_image_add",                 cmd_menu_add_image,           FALSE }, /* TODO: Rework to be "menu add image". */
    { "menu_editable_add",              cmd_menu_add_edit,            FALSE }, /* TODO: Rework to be "menu add edit". */
    { "menu_separator",                 cmd_menu_add_separator,       FALSE }, /* TODO: Rework to be "menu add separator". */
    { "menu_link_separator",            cmd_menu_add_separator_link,  FALSE }, /* TODO: Rework to be "menu add separator link". */
    { "menu_image_separator",           cmd_menu_add_separator_image, FALSE }, /* TODO: Rework to be "menu add separator image". */
    { "menu_editable_separator",        cmd_menu_add_separator_edit,  FALSE }, /* TODO: Rework to be "menu add separator edit". */
    { "menu_remove",                    cmd_menu_remove,              FALSE }, /* TODO: Rework to be "menu remove". */
    { "menu_link_remove",               cmd_menu_remove_link,         FALSE }, /* TODO: Rework to be "menu remove link". */
    { "menu_image_remove",              cmd_menu_remove_image,        FALSE }, /* TODO: Rework to be "menu remove image". */
    { "menu_editable_remove",           cmd_menu_remove_edit,         FALSE }, /* TODO: Rework to be "menu remove edit". */

    /* Search commands */
    { "search",                         cmd_search_forward,           FALSE }, /* TODO: Rework to be "search forward". */
    { "search_reverse",                 cmd_search_reverse,           FALSE }, /* TODO: Rework to be "search reverse". */
    { "search_clear",                   cmd_search_clear,             FALSE }, /* TODO: Rework to be "search clear". */
    { "dehilight",                      cmd_search_reset,             TRUE  }, /* TODO: Rework to be "search reset". */

    /* Inspector commands */
    { "show_inspector",                 cmd_inspector_show,           TRUE  }, /* TODO: Rework to be "inspector show". */

    /* Execution commands */
    { "js",                             cmd_js,                       FALSE },
    { "script",                         cmd_js_file,                  TRUE  }, /* TODO: Rename to "js_file". */
    { "spawn",                          cmd_spawn,                    TRUE  },
    { "sync_spawn",                     cmd_spawn_sync,               TRUE  }, /* TODO: Rename to "spawn_sync". */
    /* XXX: Should this command be removed? */
    { "sync_spawn_exec",                cmd_spawn_sync_exec,          TRUE  }, /* TODO: Rename to "spawn_sync_exec". */
    { "sh",                             cmd_spawn_sh,                 TRUE  }, /* TODO: Rename to "spawn_sh". */
    { "sync_sh",                        cmd_spawn_sh_sync,            TRUE  }, /* TODO: Rename to "spawn_sh_sync". */

    /* Uzbl commands */
    { "chain",                          cmd_chain,                    TRUE  },
    { "include",                        cmd_include,                  FALSE },
    { "exit",                           cmd_exit,                     TRUE  },

    /* Variable commands */
    { "set",                            cmd_set,                      FALSE },
    { "toggle",                         cmd_toggle,                   TRUE  },
    { "dump_config",                    cmd_dump_config,              TRUE  },
    { "dump_config_as_events",          cmd_dump_config_as_events,    TRUE  },
    { "print",                          cmd_print,                    FALSE },

    /* Event commands */
    { "event",                          event,                        FALSE },
    { "request",                        event,                        FALSE }
};

void
uzbl_command_init ()
{
    unsigned i;
    unsigned len = LENGTH (builtin_command_table);

    uzbl.behave.commands = g_hash_table_new (g_str_hash, g_str_equal);

    for (i = 0; i < len; ++i) {
        g_hash_table_insert (uzbl.behave.commands, (gpointer)builtin_command_table[i].name, &builtin_command_table[i]);
    }
}

void
uzbl_command_send_builtin_event ()
{
    unsigned i;
    unsigned len = LENGTH (builtin_command_table);
    GString *command_list = g_string_new ("");

    for (i = 0; i < len; ++i) {
        g_string_append (command_list, builtin_command_table[i].name);
        g_string_append_c (command_list, ' ');
    }

    send_event (BUILTINS, NULL,
        TYPE_STR, command_list->str,
        NULL);

    g_string_free (command_list, TRUE);
}

gchar**
split_quoted(const gchar* src, const gboolean unquote) {
    /* split on unquoted space or tab, return array of strings;
       remove a layer of quotes and backslashes if unquote */
    if (!src) return NULL;

    gboolean dq = FALSE;
    gboolean sq = FALSE;
    GArray *a = g_array_new (TRUE, FALSE, sizeof(gchar*));
    GString *s = g_string_new ("");
    const gchar *p;
    gchar **ret;
    gchar *dup;
    for (p = src; *p != '\0'; p++) {
        if ((*p == '\\') && unquote && p[1]) g_string_append_c(s, *++p);
        else if (*p == '\\' && p[1]) { g_string_append_c(s, *p++);
                               g_string_append_c(s, *p); }
        else if ((*p == '"') && unquote && !sq) dq = !dq;
        else if (*p == '"' && !sq) { g_string_append_c(s, *p);
                                     dq = !dq; }
        else if ((*p == '\'') && unquote && !dq) sq = !sq;
        else if (*p == '\'' && !dq) { g_string_append_c(s, *p);
                                      sq = ! sq; }
        else if ((*p == ' ' || *p == '\t') && !dq && !sq) {
            dup = g_strdup(s->str);
            g_array_append_val(a, dup);
            g_string_truncate(s, 0);
        } else g_string_append_c(s, *p);
    }
    dup = g_strdup(s->str);
    g_array_append_val(a, dup);
    ret = (gchar**)a->data;
    g_array_free (a, FALSE);
    g_string_free (s, TRUE);
    return ret;
}

void
parse_command_arguments(const gchar *p, GArray *a, gboolean split) {
    if (!split && p) { /* pass the parameters through in one chunk */
        sharg_append(a, g_strdup(p));
        return;
    }

    gchar **par = split_quoted(p, TRUE);
    if (par) {
        guint i;
        for (i = 0; i < g_strv_length(par); i++)
            sharg_append(a, g_strdup(par[i]));
        g_strfreev (par);
    }
}

const UzblCommandInfo *
parse_command_parts(const gchar *line, GArray *a) {
    UzblCommandInfo *c = NULL;

    gchar *exp_line = expand(line, 0);
    if(exp_line[0] == '\0') {
        g_free(exp_line);
        return NULL;
    }

    /* separate the line into the command and its parameters */
    gchar **tokens = g_strsplit(exp_line, " ", 2);

    /* look up the command */
    c = g_hash_table_lookup(uzbl.behave.commands, tokens[0]);

    if(!c) {
        send_event(COMMAND_ERROR, NULL,
            TYPE_STR, exp_line,
            NULL);
        g_free(exp_line);
        g_strfreev(tokens);
        return NULL;
    }

    gchar *p = g_strdup(tokens[1]);
    g_free(exp_line);
    g_strfreev(tokens);

    /* parse the arguments */
    parse_command_arguments(p, a, c->split);
    g_free(p);

    return c;
}

void
run_parsed_command(const UzblCommandInfo *c, GArray *a, GString *result) {
    /* send the COMMAND_EXECUTED event, except for set and event/request commands */
    if(strcmp("set", c->name)   &&
       strcmp("event", c->name) &&
       strcmp("request", c->name)) {
        Event *event = format_event (COMMAND_EXECUTED, NULL,
            TYPE_NAME, c->name,
            TYPE_STR_ARRAY, a,
            NULL);

        /* might be destructive on array a */
        c->function(uzbl.gui.web_view, a, result);

        send_formatted_event (event);
        event_free (event);
    }
    else
        c->function(uzbl.gui.web_view, a, result);

    if(result) {
        g_free(uzbl.state.last_result);
        uzbl.state.last_result = g_strdup(result->str);
    }
}

void
parse_cmd_line(const char *ctl_line, GString *result) {
    gchar *work_string = g_strdup(ctl_line);

    /* strip trailing newline, and any other whitespace in front */
    g_strstrip(work_string);

    if( strcmp(work_string, "") ) {
        if((work_string[0] != '#')) { /* ignore comments */
            GArray *a = g_array_new (TRUE, FALSE, sizeof(gchar*));
            const UzblCommandInfo *c = parse_command_parts(work_string, a);
            if(c)
                run_parsed_command(c, a, result);
            g_array_free (a, TRUE);
        }
    }

    g_free(work_string);
}

#define ARG_CHECK(argv, count)   \
    do                           \
    {                            \
        if (argv->len < count) { \
            return;              \
        }                        \
    } while (false)

/* Navigation commands */

void
cmd_back (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    const gchar *count = argv_idx (argv, 0);

    int n = count ? atoi (count) : 1;

    webkit_web_view_go_back_or_forward (view, -n);
}

void
cmd_forward (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    const gchar *count = argv_idx (argv, 0);

    int n = count ? atoi (count) : 1;

    webkit_web_view_go_back_or_forward (view, n);
}

void
cmd_reload (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_reload (view);
}

void
cmd_reload_force (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_reload_bypass_cache (view);
}

void
cmd_stop (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_stop_loading (view);
}

void
cmd_uri (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    gchar *uri = argv_idx (argv, 0);

    set_var_value ("uri", uri);
}

void
cmd_auth (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);


    gchar *info;
    gchar *username;
    gchar *password;

    ARG_CHECK (argv, 3);

    info = argv_idx (argv, 0);
    username = argv_idx (argv, 1);
    password = argv_idx (argv, 2);

    authenticate (info, username, password);
}

void
cmd_download (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    const gchar *uri         = argv_idx (argv, 0);
    const gchar *destination = NULL;

    if (argv->len > 1) {
        destination = argv_idx (argv, 1);
    }

    WebKitNetworkRequest *req = webkit_network_request_new (uri);
    WebKitDownload *download = webkit_download_new (req);

    /* TODO: Make a download function. */
    download_cb (view, download, (gpointer)destination);

    if (webkit_download_get_destination_uri (download)) {
        webkit_download_start (download);
    } else {
        g_object_unref (download);
    }

    g_object_unref (req);
}

/* Cookie commands */

#define strprefix(str, prefix) \
    strncmp ((str), (prefix), strlen ((prefix)))

void
cmd_cookie_add (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    gchar *host;
    gchar *path;
    gchar *name;
    gchar *value;
    gchar *scheme;
    gboolean secure = 0;
    gboolean httponly = 0;
    gchar *expires_arg;
    SoupDate *expires = NULL;

    ARG_CHECK (argv, 6);

    /* Parse with same syntax as ADD_COOKIE event */
    host = argv_idx (argv, 0);
    path = argv_idx (argv, 1);
    name = argv_idx (argv, 2);
    value = argv_idx (argv, 3);
    scheme = argv_idx (argv, 4);
    if (!strprefix (scheme, "http")) {
        secure = (scheme[4] == 's');
        httponly = !strprefix (scheme + 4 + secure, "Only");
    }
    expires_arg = argv_idx (argv, 5);
    if (*expires_arg) {
        expires = soup_date_new_from_time_t (strtoul (expires_arg, NULL, 10));
    }

    /* Create new cookie */
    /* TODO: Add support for adding non-session cookies. */
    static const int session_cookie = -1;
    SoupCookie *cookie = soup_cookie_new (name, value, host, path, session_cookie);
    soup_cookie_set_secure (cookie, secure);
    soup_cookie_set_http_only (cookie, httponly);
    if (expires) {
        soup_cookie_set_expires (cookie, expires);
    }

    /* Add cookie to jar */
    uzbl.net.soup_cookie_jar->in_manual_add = 1;
    soup_cookie_jar_add_cookie (SOUP_COOKIE_JAR (uzbl.net.soup_cookie_jar), cookie);
    uzbl.net.soup_cookie_jar->in_manual_add = 0;
}

void
cmd_cookie_delete (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 4);

    static const int expired_cookie = 0;
    SoupCookie *cookie = soup_cookie_new (
        argv_idx (argv, 2),
        argv_idx (argv, 3),
        argv_idx (argv, 0),
        argv_idx (argv, 1),
        expired_cookie);

    uzbl.net.soup_cookie_jar->in_manual_add = 1;
    soup_cookie_jar_delete_cookie (SOUP_COOKIE_JAR (uzbl.net.soup_cookie_jar), cookie);
    uzbl.net.soup_cookie_jar->in_manual_add = 0;
}

void
cmd_cookie_clear (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    /* Replace the current cookie jar with a new empty jar */
    soup_session_remove_feature (uzbl.net.soup_session,
        SOUP_SESSION_FEATURE (uzbl.net.soup_cookie_jar));
    g_object_unref (G_OBJECT (uzbl.net.soup_cookie_jar));
    uzbl.net.soup_cookie_jar = uzbl_cookie_jar_new ();
    soup_session_add_feature (uzbl.net.soup_session,
        SOUP_SESSION_FEATURE (uzbl.net.soup_cookie_jar));
}

/* Display commands */

/*
 * scroll vertical 20
 * scroll vertical 20%
 * scroll vertical -40
 * scroll vertical 20!
 * scroll vertical begin
 * scroll vertical end
 * scroll horizontal 10
 * scroll horizontal -500
 * scroll horizontal begin
 * scroll horizontal end
 */
void
cmd_scroll (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 2);

    gchar *direction = argv_idx (argv, 0);
    gchar *amount_str = argv_idx (argv, 1);
    GtkAdjustment *bar = NULL;

    if (!g_strcmp0 (direction, "horizontal")) {
        bar = uzbl.gui.bar_h;
    } else if (!g_strcmp0 (direction, "vertical")) {
        bar = uzbl.gui.bar_v;
    } else {
        uzbl_debug ("Unrecognized scroll direction: %s\n", direction);
        return;
    }

    gdouble lower = gtk_adjustment_get_lower (bar);
    gdouble upper = gtk_adjustment_get_upper (bar);
    gdouble page = gtk_adjustment_get_page_size (bar);

    if (!g_strcmp0 (amount_str, "begin")) {
        gtk_adjustment_set_value (bar, lower);
    } else if (!g_strcmp0 (amount_str, "end")) {
        gtk_adjustment_set_value (bar, upper - page);
    } else {
        gchar *end;

        gdouble value = gtk_adjustment_get_value (bar);
        gdouble amount = g_ascii_strtod (amount_str, &end);
        gdouble max_value = upper - page;

        if (*end == '%') {
            value += page * amount * 0.01;
        } else if (*end == '!') {
            value = amount;
        } else {
            value += amount;
        }

        if (value < 0) {
            value = 0; /* don't scroll past the beginning of the page */
        }
        if (value > max_value) {
            value = max_value; /* don't scroll past the end of the page */
        }

        gtk_adjustment_set_value (bar, value);
    }
}

void
cmd_zoom_in (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_zoom_in (view);
}

void
cmd_zoom_out (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_zoom_out (view);
}

void
cmd_zoom_toggle_type (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    uzbl_debug ("toggle_zoom_type is deprecated; use \'toggle zoom_type\' instead\n");

    int current_type = get_zoom_type ();
    set_zoom_type (!current_type);
}

void
cmd_status_toggle (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    uzbl_debug ("toggle_status is deprecated; use \'toggle show_status\' instead\n");

    int current_status = get_show_status ();
    set_show_status (!current_status);
}

void
cmd_hardcopy (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_frame_print (webkit_web_view_get_main_frame (view));
}

/* Search commands */

static void
search_text (WebKitWebView *view, const gchar *key, const gboolean forward);

void
cmd_search_forward (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    search_text (view, argv_idx (argv, 0), TRUE);
}

void
cmd_search_reverse (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    search_text (view, argv_idx (argv, 0), FALSE);
}

void
cmd_search_clear (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_unmark_text_matches (view);

    g_free (uzbl.state.searchtx);
    uzbl.state.searchtx = NULL;
}

void
cmd_search_reset (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_view_set_highlight_text_matches (view, FALSE);
}

void
search_text (WebKitWebView *view, const gchar *key, const gboolean forward)
{
    if (*key) {
        if (g_strcmp0 (uzbl.state.searchtx, key)) {
            webkit_web_view_unmark_text_matches (view);
            webkit_web_view_mark_text_matches (view, key, FALSE, 0);

            g_free (uzbl.state.searchtx);
            uzbl.state.searchtx = g_strdup (key);
        }
    }

    if (uzbl.state.searchtx) {
        uzbl_debug ("Searching: %s\n", uzbl.state.searchtx);

        webkit_web_view_set_highlight_text_matches (view, TRUE);
        webkit_web_view_search_text (view, uzbl.state.searchtx, FALSE, forward, TRUE);
    }
}

/* Inspector commands */

void
cmd_inspector_show (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    webkit_web_inspector_show (uzbl.gui.inspector);
}

/* Execution commands */

/* FIXME: Make private again.
void
eval_js (WebKitWebView *view, const gchar *script, GString *result, const gchar *path);
*/

void
cmd_js (WebKitWebView *view, GArray *argv, GString *result)
{
    ARG_CHECK (argv, 1);

    eval_js (view, argv_idx (argv, 0), result, "(uzbl command)");
}

void
cmd_js_file (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    gchar *req_path = argv_idx (argv, 0);
    gchar *path = NULL;

    if ((path = find_existing_file (req_path))) {
        gchar *file_contents = NULL;

        GIOChannel *chan = g_io_channel_new_file (path, "r", NULL);
        if (chan) {
            gsize len;
            g_io_channel_read_to_end (chan, &file_contents, &len, NULL);
            g_io_channel_unref (chan);
        }

        uzbl_debug ("External JavaScript file loaded: %s\n", req_path);

        gchar *arg = argv_idx (argv, 1);

        /* TODO: Make more generic? */
        gchar *js = str_replace ("%s", arg ? arg : "", file_contents);
        g_free (file_contents);

        eval_js (view, js, result, path);

        g_free (js);
        g_free (path);
    }
}

static char *
extract_js_prop (JSGlobalContextRef ctx, JSObjectRef obj, const gchar *prop);
static char *
js_value_to_string (JSGlobalContextRef ctx, JSValueRef obj);

void
eval_js (WebKitWebView *view, const gchar *script, GString *result, const gchar *path)
{
    WebKitWebFrame *frame;
    JSGlobalContextRef context;
    JSObjectRef globalobject;
    JSStringRef js_file;
    JSStringRef js_script;
    JSValueRef js_result;
    JSValueRef js_exc = NULL;

    frame = webkit_web_view_get_main_frame (view);
    context = webkit_web_frame_get_global_context (frame);
    globalobject = JSContextGetGlobalObject (context);

    /* evaluate the script and get return value*/
    js_script = JSStringCreateWithUTF8CString (script);
    js_file = JSStringCreateWithUTF8CString (path);
    js_result = JSEvaluateScript (context, js_script, globalobject, js_file, 0, &js_exc);
    if (result && js_result && !JSValueIsUndefined (context, js_result)) {
        char *result_utf8;

        result_utf8 = js_value_to_string (context, js_result);

        g_string_assign (result, result_utf8);

        free (result_utf8);
    } else if (js_exc) {
        JSObjectRef exc = JSValueToObject (context, js_exc, NULL);

        char *file;
        char *line;
        char *msg;

        file = extract_js_prop (context, exc, "sourceURL");
        line = extract_js_prop (context, exc, "line");
        msg = js_value_to_string (context, exc);

        uzbl_debug ("Exception occured while executing script:\n %s:%s: %s\n", file, line, msg);

        free (file);
        free (line);
        free (msg);
    }

    /* cleanup */
    JSStringRelease (js_script);
    JSStringRelease (js_file);
}

char *
extract_js_prop (JSGlobalContextRef ctx, JSObjectRef obj, const gchar *prop)
{
    JSStringRef js_prop;
    JSObjectRef js_prop_val;

    js_prop = JSStringCreateWithUTF8CString (prop);
    js_prop_val = JSObjectGetProperty (ctx, obj, js_prop, NULL);

    JSStringRelease (js_prop);

    return js_value_to_string (ctx, js_prop_val);
}

char *
js_value_to_string (JSGlobalContextRef ctx, JSValueRef val)
{
    JSStringRef str = JSValueToStringCopy (ctx, val, NULL);
    size_t size = JSStringGetMaximumUTF8CStringSize (str);

    char *result = NULL;

    if (size) {
        result = (gchar *)malloc (size * sizeof (char));
        JSStringGetUTF8CString (str, result, size);
    }

    JSStringRelease (str);

    return result;
}

static void
spawn (GArray *argv, GString *result, gboolean exec);
static void
spawn_sh (GArray *argv, GString *result);

void
cmd_spawn (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    spawn (argv, NULL, FALSE);
}

void
cmd_spawn_sync (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);

    spawn (argv, result, FALSE);
}

void
cmd_spawn_sync_exec (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);

    if (!result) {
        GString *force_result = g_string_new ("");
        spawn (argv, force_result, TRUE);
        g_string_free (force_result, TRUE);
    } else {
        spawn (argv, result, TRUE);
    }
}

void
cmd_spawn_sh (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    spawn_sh (argv, NULL);
}

void
cmd_spawn_sh_sync (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);

    spawn_sh (argv, result);
}

/* make sure that the args string you pass can properly be interpreted (eg
 * properly escaped against whitespace, quotes etc) */
static gboolean
run_command (const gchar *command, const gchar **args, const gboolean sync,
             char **output_stdout);

void
spawn (GArray *argv, GString *result, gboolean exec)
{
    gchar *path = NULL;

    ARG_CHECK (argv, 1);

    const gchar **args = &g_array_index (argv, const gchar *, 1);
    const gchar *req_path = argv_idx (argv, 0);

    path = find_existing_file (req_path);

    if (!path) {
        uzbl_debug ("Failed to spawn child process: %s not found\n", req_path);
        return;
    }

    gchar *r = NULL;
    run_command (path, args, result != NULL, result ? &r : NULL);
    if (result) {
        g_string_assign (result, r);
        /* run each line of output from the program as a command */
        if (exec && r) {
            gchar *head = r;
            gchar *tail;
            while ((tail = strchr (head, '\n'))) {
                *tail = '\0';
                parse_cmd_line (head, NULL);
                head = tail + 1;
            }
        }
    }

    g_free (r);
    g_free (path);
}

void
spawn_sh (GArray *argv, GString *result)
{
    if (!uzbl.behave.shell_cmd) {
        uzbl_debug ("spawn_sh: shell_cmd is not set!\n");
        return;
    }
    guint i;

    gchar **cmd = split_quoted (uzbl.behave.shell_cmd, TRUE);
    if (!cmd) {
        return;
    }

    g_array_insert_val (argv, 1, cmd[0]);

    for (i = g_strv_length (cmd)-1; i; --i) {
        g_array_prepend_val (argv, cmd[i]);
    }

    const gchar *arg_start = argv_idx (argv, 0);

    if (result) {
        gchar *r = NULL;
        run_command (cmd[0], &arg_start, TRUE, &r);
        g_string_assign (result, r);
        g_free (r);
    } else {
        run_command (cmd[0], &arg_start, FALSE, NULL);
    }

    g_strfreev (cmd);
}

gboolean
run_command (const gchar *command, const gchar **args, const gboolean sync,
             char **output_stdout)
{
    GError *err = NULL;

    GArray *a = g_array_new (TRUE, FALSE, sizeof (gchar*));
    guint i;
    guint len = g_strv_length ((gchar **)args);

    sharg_append (a, command);

    for (i = 0; i < len; ++i) {
        sharg_append (a, args[i]);
    }

    gboolean result;
    if (sync) {
        if (*output_stdout) {
            g_free (*output_stdout);
        }

        result = g_spawn_sync (NULL, (gchar **)a->data, NULL, G_SPAWN_SEARCH_PATH,
                               NULL, NULL, output_stdout, NULL, NULL, &err);
    } else {
        result = g_spawn_async (NULL, (gchar **)a->data, NULL, G_SPAWN_SEARCH_PATH,
                                NULL, NULL, NULL, &err);
    }

    if (uzbl.state.verbose) {
        GString *s = g_string_new ("spawned:");
        for (i = 0; i < a->len; ++i) {
            gchar *qarg = g_shell_quote (g_array_index (a, gchar*, i));
            g_string_append_printf (s, " %s", qarg);
            g_free (qarg);
        }
        g_string_append_printf (s, " -- result: %s", (result ? "true" : "false"));
        printf ("%s\n", s->str);
        g_string_free (s, TRUE);
        if (output_stdout) {
            printf ("Stdout: %s\n", *output_stdout);
        }
    }
    if (err) {
        g_printerr ("error on run_command: %s\n", err->message);
        g_error_free (err);
    }
    g_array_free (a, TRUE);
    return result;
}

/* Uzbl commands */

void
cmd_chain (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);

    ARG_CHECK (argv, 1);

    guint i = 0;
    const gchar *cmd;
    GString *r = g_string_new ("");

    while ((cmd = argv_idx (argv, i++))) {
        GArray *a = g_array_new (TRUE, FALSE, sizeof (gchar*));
        const UzblCommandInfo *c = parse_command_parts (cmd, a);
        if (c) {
            run_parsed_command (c, a, r);
        }
        g_array_free (a, TRUE);
    }
    if (result) {
        g_string_assign (result, r->str);
    }

    g_string_free (r, TRUE);
}

void
cmd_include (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    gchar *req_path = argv_idx (argv, 0);
    gchar *path = NULL;

    if ((path = find_existing_file (req_path))) {
        run_command_file (path);
        send_event (FILE_INCLUDED, NULL,
            TYPE_STR, path,
            NULL);
        g_free (path);
    }
}

void
cmd_exit (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    /* hide window a soon as possible to avoid getting stuck with a
     * non-response window in the cleanup steps. */
    if (uzbl.gui.main_window) {
        gtk_widget_destroy (uzbl.gui.main_window);
    } else if (uzbl.gui.plug) {
        gtk_widget_destroy (GTK_WIDGET (uzbl.gui.plug));
    }

    gtk_main_quit ();
}

/* Variable commands */

void
cmd_set (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    gchar **split = g_strsplit (argv_idx (argv, 0), "=", 2);
    if (split[0] != NULL) {
        gchar *value = split[1] ? g_strchug (split[1]) : " ";
        set_var_value (g_strstrip (split[0]), value);
    }
    g_strfreev (split);
}

void
cmd_toggle (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (result);

    ARG_CHECK (argv, 1);

    const gchar *var_name = argv_idx (argv, 0);

    uzbl_cmdprop *c = get_var_c (var_name);

    if (!c) {
        if (argv->len > 1) {
            set_var_value (var_name, argv_idx (argv, 1));
        } else {
            set_var_value (var_name, "1");
        }

        return;
    }

    switch (c->type) {
    case TYPE_STR:
    {
        const gchar *next;

        if (argv->len >= 3) {
            gchar *current = get_var_value_string_c (c);

            guint i = 2;
            const gchar *first   = argv_idx (argv, 1);
            const gchar *this    = first;
                         next    = argv_idx (argv, 2);

            while (next && strcmp (current, this)) {
                this = next;
                next = argv_idx (argv, ++i);
            }

            if (!next) {
                next = first;
            }

            g_free (current);
        } else {
            next = "";
        }

        set_var_value_string_c (c, next);
        break;
    }
    case TYPE_INT:
    {
        int current = get_var_value_int_c (c);
        int next;

        if (argv->len >= 3) {
            guint i = 2;

            int first = strtol (argv_idx (argv, 1), NULL, 10);
            int  this = first;

            const gchar *next_s = argv_idx (argv, 2);

            while (next_s && (this != current)) {
                this   = strtol (next_s, NULL, 10);
                next_s = argv_idx (argv, ++i);
            }

            if (next_s) {
                next = strtol (next_s, NULL, 10);
            } else {
                next = first;
            }
        } else {
            next = !current;
        }

        set_var_value_int_c (c, next);
        break;
    }
    case TYPE_FLOAT:
    {
        float current = get_var_value_float_c (c);
        float next;

        if (argv->len >= 3) {
            guint i = 2;

            float first = strtod (argv_idx (argv, 1), NULL);
            float  this = first;

            const gchar *next_s = argv_idx (argv, 2);

            while (next_s && (this != current)) {
                this   = strtod (next_s, NULL);
                next_s = argv_idx (argv, ++i);
            }

            if (next_s) {
                next = strtod (next_s, NULL);
            } else {
                next = first;
            }
        } else {
            next = !current;
        }

        set_var_value_float_c (c, next);
        break;
    }
    default:
        g_assert_not_reached ();
    }

    send_set_var_event (var_name, c);
}

void
cmd_print (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);

    ARG_CHECK (argv, 1);

    gchar* buf;

    if (!result) {
        return;
    }

    buf = expand (argv_idx (argv, 0), 0);
    g_string_assign (result, buf);
    g_free (buf);
}

void
cmd_dump_config (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    dump_config ();
}

void
cmd_dump_config_as_events (WebKitWebView *view, GArray *argv, GString *result)
{
    UZBL_UNUSED (view);
    UZBL_UNUSED (argv);
    UZBL_UNUSED (result);

    dump_config_as_events ();
}

void
event(WebKitWebView *page, GArray *argv, GString *result) {
    (void) page; (void) result;
    GString *event_name;
    gchar **split = NULL;

    if(!argv_idx(argv, 0))
       return;

    split = g_strsplit(argv_idx(argv, 0), " ", 2);
    if(split[0])
        event_name = g_string_ascii_up(g_string_new(split[0]));
    else
        return;

    send_event(0, event_name->str, TYPE_FORMATTEDSTR, split[1] ? split[1] : "", NULL);

    g_string_free(event_name, TRUE);
    g_strfreev(split);
}
