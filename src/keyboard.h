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

#ifndef __KEYBOARD_H
#define __KEYBOARD_H

/* escape or alt key */
#define META_KEY 0x1b

/* adapted from Midnight Commander */

/* macro to get CTRL+key sequence */
#define KEY_CTRL(x) ((x)&31)
/* macro to get ALT+key sequence */
#define KEY_ALT(x) (0x200 | (x))
#define is_enter_key(c) ((c) == '\r' || (c) == '\n' || (c) == KEY_ENTER)

/***********************************/

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

/* a structure, which holds the keybindings */
extern struct keybindings keys;

#endif
