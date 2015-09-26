#include <webkit2/webkit-web-extension.h>
#include <webkitdom/WebKitDOMDOMWindowUnstable.h>
#include <webkitdom/webkitdom.h>
#include <time.h>
#include <fcntl.h>

static void
dom_focus_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data);
static void
dom_blur_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data);

void
dom_focus_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data)
{
    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    g_print ("FOCUS_ELEMENT %s\n", name);
}

void
dom_blur_cb (WebKitDOMEventTarget *target, WebKitDOMEvent *event, gpointer data)
{
    WebKitDOMEventTarget *etarget = webkit_dom_event_get_target (event);
    gchar *name = webkit_dom_node_get_node_name (WEBKIT_DOM_NODE (etarget));

    g_print ("BLUR_ELEMENT %s\n", name);
}

static void
uzbl_document_loaded_callback (WebKitWebPage *web_page,
                               gpointer       user_data)
{
    WebKitDOMDocument *doc = webkit_web_page_get_dom_document (web_page);
    WebKitDOMDOMWindow *win = webkit_dom_document_get_default_view (doc);

    g_print ("ext: document loaded!\n");
    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (doc),
        "focus", G_CALLBACK (dom_focus_cb), TRUE, NULL);
    webkit_dom_event_target_add_event_listener (WEBKIT_DOM_EVENT_TARGET (doc),
        "blur",  G_CALLBACK (dom_blur_cb), TRUE, NULL);
}

static void
uzbl_web_page_created_callback (WebKitWebExtension *extension,
                                WebKitWebPage      *web_page,
                                gpointer            user_data)
{
    g_signal_connect (web_page, "document-loaded",
                      G_CALLBACK (uzbl_document_loaded_callback),
                      NULL);
    g_print ("ext: page created!\n");
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    int fd = open ("/dev/null", O_RDONLY);
    g_print ("extension loaded for %d %s.\n", getpid (), g_getenv ("UZBL_WHOOOP"));
    g_print ("there are %d open files\n", fd - 1);
    g_signal_connect (extension, "page-created",
                      G_CALLBACK (uzbl_web_page_created_callback),
                      NULL);
}
