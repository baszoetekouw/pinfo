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

/* a structure, which holds the keybindings */
extern struct keybindings keys;

#endif
