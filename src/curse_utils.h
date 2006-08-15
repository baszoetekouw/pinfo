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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA
 ***************************************************************************/

#ifndef __CURSE_UTILS_H
#define __CURSE_UTILS_H

#include <string>

#ifndef HAVE_CURS_SET
void curs_set (int a);
#endif

/* initializes GNU locales */
void initlocale ();
/* closes the program, and removes temporary files */
void closeprogram ();

/* is curses screen open? */
extern int curses_open;

/* initializes curses interface */
void init_curses ();
/* user defined getch, capable of handling ALT keybindings */
int pinfo_getch ();
/* Block until something's on STDIN */
void waitforgetch ();
/* an interface to gnu readline */
std::string getstring (const char *prompt);
/* for some reasons mvhline does not work quite properly... */
void mymvhline (int y, int x, char ch, int len);
/* this one supports color back/foreground */
void myclrtoeol ();
/* takes care of the cursor, which is turned off */
void myendwin ();

/* handle localized `(y/n)' dialog box.  */
int yesno (const char *prompt, int def);

/* Handle SIGWINCH */
void handlewinch ();

#endif
