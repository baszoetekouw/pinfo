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

#ifndef __UTILS_H
#define __UTILS_H

#include "datatypes.h"
#include <string>

extern std::string safe_user;
extern std::string safe_group;

#ifndef HAVE_CURS_SET
void curs_set (int a);
#endif

#ifdef ___DONT_USE_REGEXP_SEARCH___
extern char *pinfo_re_pattern;
#endif

/* wrappers for re_comp and re_exec */
int pinfo_re_comp (const char *name);
int pinfo_re_exec (const char *name);

/* user defined getch, capable of handling ALT keybindings */
int pinfo_getch ();
/* free() wrapper */
void xfree (void *ptr);
/* malloc() wrapper */
void *xmalloc (size_t size);
/* realloc() wrapper */
void *xrealloc (void *ptr, size_t size);
/* initializes GNU locales */
void initlocale ();
/* bail out if file name causes security problems */
void checkfilename (const std::string filename);
/* closes the program, and removes temporary files */
void closeprogram ();
/* initializes curses interface */
void init_curses ();
/* an interface to gnu readline */
char *getstring (const char *prompt);
/* for some reasons mvhline does not work quite properly... */
void mymvhline (int y, int x, char ch, int len);
/* this one supports color back/foreground */
void myclrtoeol ();
/* takes care of the cursor, which is turned off */
void myendwin ();

/* strcmp, which is insensitive to whitespaces */
int compare_tag_table_string (const char *base, const char *compared);
bool compare_tags (TagTable a, TagTable b);

/* get offset of "node" in tag_table variable */
int gettagtablepos (std::string node);

/* handle localized `(y/n)' dialog box.  */
int yesno (const char *prompt, int def);
/* copies the first part of string, which is without regexp */
void copy_stripped_from_regexp (char *src, char *dest);


/* Block until something's on STDIN */
void waitforgetch ();

/* Handle SIGWINCH */
void handlewinch ();

/* is curses screen open? */
extern int curses_open;

/* Explode a string into a vector */
std::vector<std::string>
string_explode(std::string to_explode, std::string::value_type separator);

/* Return a string converted to uppercase */
std::string string_toupper (std::string s);

#endif
