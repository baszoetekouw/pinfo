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

#include "common_includes.h"

#ifdef HAVE_CURSES_COLOR
extern struct colours cols;
#endif /* HAVE_CURSES_COLOR */

int normal;
int menuselected;
int menu;
int noteselected;
int note;
int topline;
int bottomline;
int manualbold;
int manualitalic;
int url;
int urlselected;
int infohighlight;
int searchhighlight;

void
initcolors()
{
#ifdef HAVE_CURSES_COLOR
	if (has_colors())
	{
		start_color();
#ifdef HAVE_DECL_USE_DEFAULT_COLORS
		use_default_colors();
#endif
		normal = COLOR_PAIR(NORMAL);	/* normal text */
		init_pair(NORMAL, cols.normal_fore, cols.normal_back);
		if (cols.normal_bold)
			normal |= A_BOLD;
		if (cols.normal_blink)
			normal |= A_BLINK;

		menuselected = COLOR_PAIR(MENUSELECTED);		/* selected menu */
		init_pair(MENUSELECTED, cols.menuselected_fore, cols.menuselected_back);
		if (cols.menuselected_bold)
			menuselected |= A_BOLD;
		if (cols.menuselected_blink)
			menuselected |= A_BLINK;

		menu = COLOR_PAIR(MENU);	/* just menu */
		init_pair(MENU, cols.menu_fore, cols.menu_back);
		if (cols.menu_bold)
			menu |= A_BOLD;
		if (cols.menu_blink)
			menu |= A_BLINK;

		noteselected = COLOR_PAIR(NOTESELECTED);		/* selected note */
		init_pair(NOTESELECTED, cols.noteselected_fore, cols.noteselected_back);
		if (cols.noteselected_bold)
			noteselected |= A_BOLD;
		if (cols.noteselected_blink)
			noteselected |= A_BLINK;

		note = COLOR_PAIR(NOTE);	/* just note */
		init_pair(NOTE, cols.note_fore, cols.note_back);
		if (cols.note_bold)
			note |= A_BOLD;
		if (cols.note_blink)
			note |= A_BLINK;

		topline = COLOR_PAIR(TOPLINE);	/* topline color */
		init_pair(TOPLINE, cols.topline_fore, cols.topline_back);
		if (cols.topline_bold)
			topline |= A_BOLD;
		if (cols.topline_blink)
			topline |= A_BLINK;

		bottomline = COLOR_PAIR(BOTTOMLINE);	/* bottomline color */
		init_pair(BOTTOMLINE, cols.bottomline_fore, cols.bottomline_back);
		if (cols.bottomline_bold)
			bottomline |= A_BOLD;
		if (cols.bottomline_blink)
			bottomline |= A_BLINK;

		manualbold = COLOR_PAIR(MANUALBOLD);	/* manual bold color */
		init_pair(MANUALBOLD, cols.manualbold_fore, cols.manualbold_back);
		if (cols.manualbold_bold)
			manualbold |= A_BOLD;
		if (cols.manualbold_blink)
			manualbold |= A_BLINK;

		manualitalic = COLOR_PAIR(MANUALITALIC);		/* manual italic color */
		init_pair(MANUALITALIC, cols.manualitalic_fore, cols.manualitalic_back);
		if (cols.manualitalic_bold)
			manualitalic |= A_BOLD;
		if (cols.manualitalic_blink)
			manualitalic |= A_BLINK;

		url = COLOR_PAIR(URL);	/* url(http, ftp) color */
		init_pair(URL, cols.url_fore, cols.url_back);
		if (cols.url_bold)
			url |= A_BOLD;
		if (cols.url_blink)
			url |= A_BLINK;

		urlselected = COLOR_PAIR(URLSELECTED);	/* selected url */
		init_pair(URLSELECTED, cols.urlselected_fore, cols.urlselected_back);
		if (cols.urlselected_bold)
			urlselected |= A_BOLD;
		if (cols.urlselected_blink)
			urlselected |= A_BLINK;

		infohighlight = COLOR_PAIR(INFOHIGHLIGHT);	/* highlight for info quotes */
		init_pair(INFOHIGHLIGHT, cols.infohighlight_fore, cols.infohighlight_back);
		if (cols.infohighlight_bold)
			infohighlight |= A_BOLD;
		if (cols.infohighlight_blink)
			infohighlight |= A_BLINK;

		searchhighlight = COLOR_PAIR(SEARCHHIGHLIGHT);	/* highlight for info quotes */
		init_pair(SEARCHHIGHLIGHT, cols.searchhighlight_fore, cols.searchhighlight_back);
		if (cols.searchhighlight_bold)
			searchhighlight |= A_BOLD;
		if (cols.searchhighlight_blink)
			searchhighlight |= A_BLINK;
	}
	else
	{
#endif /* HAVE_CURSES_COLOR */
		normal = A_NORMAL;
		menu = A_BOLD;
		note = A_BOLD;
		url = A_BOLD;
		menuselected = A_REVERSE;
		noteselected = A_REVERSE;
		urlselected = A_REVERSE;
		topline = A_REVERSE;
		bottomline = A_REVERSE;
		manualbold = A_BOLD;
		manualitalic = A_BOLD;
		infohighlight = A_BOLD;
		searchhighlight = A_BOLD;
#ifdef HAVE_CURSES_COLOR
	}
#endif /* HAVE_CURSES_COLOR */
}
