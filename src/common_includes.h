#ifndef __COMMON_INCLUDES_H
#define __COMMON_INCLUDES_H

#include "localestuff.h"

#if defined(USE_NCURSES) && !defined(RENAMED_NCURSES)
#include <ncurses.h>
#else
#include <curses.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rcsid.h"
#include "datatypes.h"
#include "filehandling_functions.h"
#include "video.h"
#include "menu_and_note_utils.h"
#include "mainfunction.h"
#include "utils.h"
#include "signal_handler.h"
#include "colors.h"
#include "regexp_search.h"
#include "manual.h"
#include "parse_config.h"
#include "keyboard.h"
#include "initializelinks.h"
#include "printinfo.h"

/*
 * Readline isn't safe for nonlinux terminals (i.e. vt100)
 * But if you have readline linked with ncurses you may enable readline with
 * ./configure --with-readline
 *
 */
#ifndef HAS_READLINE
#include "readlinewrapper.h"
#endif /* HAS_READLINE */

#ifndef HAVE_SIGBLOCK
#include "sigblock.h"
#endif

#define _REGEX_RE_COMP		/* I hear voices, that it is needed by RH5.2 ;) */
#endif
