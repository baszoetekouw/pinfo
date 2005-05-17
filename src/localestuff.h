/* locale support. Adapted from binutils */

#ifndef __LOCALESTUFF_H
#define __LOCALESTUFF_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Take care of NLS matters.  */

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#ifndef HAVE_SETLOCALE
#define setlocale(Category, Locale)	/* empty */
#endif

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(Text) gettext (Text)
#else
#undef bindtextdomain
#define bindtextdomain(Domain, Directory)	/* empty */
#undef textdomain
#define textdomain(Domain)	/* empty */
#define _(Text) Text
#endif

#define STREQ(a,b) (strcmp((a), (b)) == 0)

#endif
