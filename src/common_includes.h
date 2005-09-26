/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
 *  Copyright 2005  Nathanael Nerode <neroden@gcc.gnu.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 ***************************************************************************/

#ifndef __COMMON_INCLUDES_H
#define __COMMON_INCLUDES_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* Take care of NLS matters.  */
#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory)	/* empty */
# undef textdomain
# define textdomain(Domain)	/* empty */
# define _(Text) Text
#endif

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
#include <ctype.h>

#include "datatypes.h"
#include "filehandling_functions.h"
#include "mainfunction.h"
#include "utils.h"

/* I hear voices, that it is needed by RH5.2 ;) */
#define _REGEX_RE_COMP

/* This comes up appallingly often.
 * Note that this is correct: maxy is one beyond the end of screen,
 * and a header and footer line take up space as well.
 */
#define lines_visible (maxy - 2)

#endif
