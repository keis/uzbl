#include <webkit2/webkit-web-extension.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include "uzbl-ext.h"
#include "extio.h"
#include "util.h"
#include "js.h"

struct _UzblExt {
    GIOStream *stream;
    WebKitWebPage *web_page;
};

UzblExt*
uzbl_ext_new ()
{
    UzblExt *ext = g_new (UzblExt, 1);
    return ext;
}

static JSValueRef
uzbl_js_evaluate(JSGlobalContextRef jsctx, const gchar *script, const gchar *path)
{
    JSObjectRef globalobject = JSContextGetGlobalObject (jsctx);
    JSValueRef js_exc = NULL;

    JSStringRef js_script = JSStringCreateWithUTF8CString (script);
    JSStringRef js_file = JSStringCreateWithUTF8CString (path);
    JSValueRef js_result = JSEvaluateScript (jsctx, js_script, globalobject, js_file, 0, &js_exc);

    if (js_exc) {
        JSObjectRef exc = JSValueToObject (jsctx, js_exc, NULL);

        gchar *file = uzbl_js_to_string (jsctx, uzbl_js_get (jsctx, exc, "sourceURL"));
        gchar *line = uzbl_js_to_string (jsctx, uzbl_js_get (jsctx, exc, "line"));
        gchar *msg = uzbl_js_to_string (jsctx, exc);

        g_debug ("Exception occured while executing script:\n %s:%s: %s\n", file, line, msg);

        g_free (file);
        g_free (line);
        g_free (msg);
    }

    JSStringRelease (js_file);
    JSStringRelease (js_script);

    return js_result;
}

void
read_message_cb (GObject *stream,
                 GAsyncResult *res,
                 gpointer user_data);

void
uzbl_ext_init_io (UzblExt *ext, int in, int out)
{
    GInputStream *input = g_unix_input_stream_new (in, TRUE);
    GOutputStream *output = g_unix_output_stream_new (out, TRUE);

    ext->stream = g_simple_io_stream_new (input, output);

    uzbl_extio_read_message_async (input, read_message_cb, ext);
}

void
read_message_cb (GObject *source,
                 GAsyncResult *res,
                 gpointer user_data)
{
    UzblExt *ext = (UzblExt*) user_data;
    GInputStream *stream = G_INPUT_STREAM (source);
    GError *error = NULL;
    GVariant *message;
    ExtIOMessageType messagetype;

    if (!(message = uzbl_extio_read_message_finish (stream, res, &messagetype, &error))) {
        g_warning ("reading message from core: %s", error->message);
        g_error_free (error);
        return;
    }

    switch (messagetype) {
    case EXT_RUN_JS:
        {
            UzblJSContext ctx;
            gchar *path;
            gchar *script;
            uzbl_extio_get_message_data (EXT_RUN_JS, message, &ctx, &path, &script);
            g_debug ("let's run some javascript %d %s %s", ctx, path, script);
            WebKitFrame *mf = webkit_web_page_get_main_frame (ext->web_page);
            JSGlobalContextRef jsctx = JSGlobalContextRetain(
                webkit_frame_get_javascript_global_context (mf));
            JSValueRef value = uzbl_js_evaluate (jsctx, script, path);
            JSGlobalContextRelease (jsctx);
            g_free (script);
            break;
        }
    default:
        {
            gchar *pmsg = g_variant_print (message, TRUE);
            g_debug ("got unrecognised message %s", pmsg);
            g_free (pmsg);
            break;
        }
    }
    g_variant_unref (message);

    uzbl_extio_read_message_async (stream, read_message_cb, ext);
}

static void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data);

static void
document_loaded_callback (WebKitWebPage *web_page,
                          gpointer       user_data);
static void
dom_focus_callback (WebKitDOMEventTarget *target,
                    WebKitDOMEvent       *event,
                    gpointer              user_data);
static void
dom_blur_callback (WebKitDOMEventTarget *target,
                   WebKitDOMEvent       *event,
                   gpointer              user_data);


G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension,
                                                GVariant           *user_data)
{
    gchar *pretty_data = g_variant_print (user_data, TRUE);
    g_debug ("Initializing web extension with %s", pretty_data);
    g_free (pretty_data);

    UzblExt *ext = uzbl_ext_new ();
    gint proto;
    gint64 in;
    gint64 out;

    g_variant_get (user_data, "(ixx)", &proto, &in, &out);
    uzbl_ext_init_io (ext, in, out);

    uzbl_extio_send_new_message (g_io_stream_get_output_stream (ext->stream),
                                 EXT_HELO, EXTIO_PROTOCOL);

    if (proto != EXTIO_PROTOCOL) {
        g_warning ("Extension loaded into incompatible version of uzbl (expected %d, was %d)", EXTIO_PROTOCOL, proto);
        return;
    }

    g_signal_connect (extension, "page-created",
                      G_CALLBACK (web_page_created_callback),
                      ext);
}

void
web_page_created_callback (WebKitWebExtension *extension,
                           WebKitWebPage      *web_page,
                           gpointer            user_data)
{
    UZBL_UNUSED (extension);
    UzblExt *ext = (UzblExt*)user_data;

    g_debug ("Web page created");
    ext->web_page = web_page;
    g_signal_connect (web_page, "document-loaded",
                      G_CALLBACK (document_loaded_callback), ext);
}

void
document_loaded_callback (WebKitWebPage *web_page,
                          gpointer       user_data)
{
    UzblExt *ext = (UzblExt*)user_data;
    WebKitDOMDocument *doc = webkit_web_page_get_dom_document (web_page);

    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (doc),
        "focus", G_CALLBACK (dom_focus_callback), TRUE, ext);
    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (doc),
        "blur",  G_CALLBACK (dom_blur_callback), TRUE, ext);
}

void
dom_focus_callback (WebKitDOMEventTarget *target,
                    WebKitDOMEvent       *event,
                    gpointer              user_data)
{
    UZBL_UNUSED (target);

    UzblExt *ext = (UzblExt*)user_data;
    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    uzbl_extio_send_new_message (g_io_stream_get_output_stream (ext->stream),
                                 EXT_FOCUS, name);
}

void
dom_blur_callback (WebKitDOMEventTarget *target,
                   WebKitDOMEvent       *event,
                   gpointer user_data)
{
    UZBL_UNUSED (target);

    UzblExt *ext = (UzblExt*)user_data;
    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    uzbl_extio_send_new_message (g_io_stream_get_output_stream (ext->stream),
                                 EXT_BLUR, name);
}
