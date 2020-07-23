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

#ifndef __PARSE_CONFIG_H
#define __PARSE_CONFIG_H

#ifndef ___DONT_USE_REGEXP_SEARCH___
#include <regex.h>
#endif

#define BOLD 1
#define NO_BOLD 0
#define BLINK 1
#define NO_BLINK 0

typedef struct keybindings
{
	int totalsearch_1, totalsearch_2;
	int search_1, search_2;
	int goto_1, goto_2;
	int prevnode_1, prevnode_2;
	int nextnode_1, nextnode_2;
	int upnode_1, upnode_2;
	int up_1, up_2;
	int end_1, end_2;
	int pgdn_1, pgdn_2;
	int home_1, home_2;
	int pgup_1, pgup_2;
	int down_1, down_2;
	int top_1, top_2;
	int back_1, back_2;
	int followlink_1, followlink_2;
	int quit_1, quit_2;
	int refresh_1, refresh_2;
	int shellfeed_1, shellfeed_2;
	int dirpage_1, dirpage_2;
	int pgdn_auto_1, pgdn_auto_2;
	int pgup_auto_1, pgup_auto_2;
	int search_again_1, search_again_2;
	int goline_1, goline_2;
	int twoup_1, twoup_2;
	int twodown_1, twodown_2;
	int print_1, print_2;
	int left_1, left_2;
	int right_1, right_2;
}
keybindings;

#ifdef HAVE_CURSES_COLOR
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
}
colours;
#endif /* HAVE_CURSES_COLOR */

extern int use_manual;

int parse_config (void);
int parse_line (char *line);
char *str_toupper (char *s);
char *skip_whitespace (char *s);
char *remove_quotes (char *str);

#ifndef ___DONT_USE_REGEXP_SEARCH___
extern regex_t *h_regexp;	/* regexps to highlight */
extern int h_regexp_num;	/* number of those regexps */
#endif

#endif
