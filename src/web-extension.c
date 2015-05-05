#include <webkit2/webkit-web-extension.h>

static void
uzbl_web_page_created_callback (WebKitWebExtension *extension,
                                WebKitWebPage      *web_page,
                                gpointer            user_data)
{
    g_print ("Hello, World!");
}

G_MODULE_EXPORT void
webkit_web_extension_initialize (WebKitWebExtension *extension)
{
    g_signal_connect (extension, "page-created",
                      G_CALLBACK (uzbl_web_page_created_callback),
                      NULL);
}
