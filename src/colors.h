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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 *  USA
 ***************************************************************************/

#ifndef __COLORS_H
#define __COLORS_H

/* numbers of color pairs in curses color definitions */

#define NORMAL           1
#define MENUSELECTED     2
#define NOTESELECTED     3
#define MENU             4
#define NOTE             5
#define TOPLINE          6
#define BOTTOMLINE       7
#define MANUALBOLD       8
#define MANUALITALIC     9
#define URL              10
#define URLSELECTED      11
#define INFOHIGHLIGHT    12
#define SEARCHHIGHLIGHT  13

/* those bellow hold color attributes for named screen widgets */

extern int menu;
extern int menuselected;
extern int note;
extern int noteselected;
extern int normal;
extern int topline;
extern int bottomline;
extern int manualbold;
extern int manualitalic;
extern int url;
extern int urlselected;
extern int infohighlight;
extern int searchhighlight;

/*
 * initialize color values/attributes/etc.  Either for color and monochrome
 * mode.
 */
void initcolors ();

#endif
