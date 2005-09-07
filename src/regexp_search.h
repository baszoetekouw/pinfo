/***************************************************************************
 *  Pinfo is a ncurses based lynx style info documentation browser
 *
 *  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
 *  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
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

#ifndef __REGEXP_SEARCH_H
#define __REGEXP_SEARCH_H

#ifndef ___DONT_USE_REGEXP_SEARCH___
#include <vector>
#include <regex.h>
#endif

/* wrappers for re_comp and re_exec */
int pinfo_re_comp (const char *name);
int pinfo_re_exec (const char *name);

#ifdef ___DONT_USE_REGEXP_SEARCH___
extern char *pinfo_re_pattern;
#endif

#ifndef ___DONT_USE_REGEXP_SEARCH___

extern std::vector<regex_t> h_regexp;	/* regexps to highlight */

int regexp_search (char *pattern, char *string);
#endif

#endif
