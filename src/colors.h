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

#ifndef NO_COLOR_CURSES
typedef struct colours
{
	int normal_fore, normal_back, normal_bold, normal_blink;
	int menuselected_fore, menuselected_back, menuselected_bold, menuselected_blink;
	int menu_fore, menu_back, menu_bold, menu_blink;
	int noteselected_fore, noteselected_back, noteselected_bold, noteselected_blink;
	int note_fore, note_back, note_bold, note_blink;
	int topline_fore, topline_back, topline_bold, topline_blink;
	int bottomline_fore, bottomline_back, bottomline_bold, bottomline_blink;
	int manualbold_fore, manualbold_back, manualbold_bold, manualbold_blink;
	int manualitalic_fore, manualitalic_back, manualitalic_bold, manualitalic_blink;
	int url_fore, url_back, url_bold, url_blink;
	int urlselected_fore, urlselected_back, urlselected_bold, urlselected_blink;
	int infohighlight_fore, infohighlight_back, infohighlight_bold, infohighlight_blink;
	int searchhighlight_fore, searchhighlight_back, searchhighlight_bold,
		searchhighlight_blink;
} colours;

extern struct colours cols;
#endif /* NO_COLOR_CURSES */

/* Monochrome defines */
#define BOLD 1
#define NO_BOLD 0
#define BLINK 1
#define NO_BLINK 0

/*
 * initialize color values/attributes/etc.  Either for color and monochrome
 * mode.
 */
void initcolors ();

#endif
