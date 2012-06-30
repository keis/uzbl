/**
 * Uzbl tweaks and extension for soup
 */

#ifndef __UZBL_SOUP__
#define __UZBL_SOUP__

#include <libsoup/soup.h>

/**
 * Attach uzbl specific behaviour to the given SoupSession
 */
void uzbl_soup_init (SoupSession *session);

void add_scheme_handler (const gchar *scheme,
                         const gchar *command);

void authenticate   (const char  *authinfo,
                     const char  *username,
                     const char  *password);
#endif
