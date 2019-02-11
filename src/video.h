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


#ifndef __VIDEO_H
#define __VIDEO_H
/* paints the screen while viewing info file */
void showscreen (char **message, unsigned long lines, unsigned long pos,
		long cursor, int column);
/* prints unselected menu option */
void mvaddstr_menu (int y, int x, char *line, int linenumber);
/* prints selected menu option */
void mvaddstr_menu_selected (int y, int x, char *line, int linenumber);
/* prints unselected note option */
void mvaddstr_note (int y, int x, char *line, char *nline, int linenumber);
/* prints selected note option */
void mvaddstr_note_selected (int y, int x, char *line, char *nline, int linenumber);
/* adds top line of info page */
void addtopline (char *type, int column);
#endif
