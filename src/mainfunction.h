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

#ifndef __MAINFUNCTION_H
#define __MAINFUNCTION_H

/*
 * return value type for work(). it is the name of node, where to go, after
 * viewing of current node ends. (viewing always takes place inside of the
 * work() function
 */
typedef struct
{
	char *node;		/* name of node */
	char *file;		/* name of file, where the node is */
}
WorkRVal;

/* this determines whether we are in a position, found after search */
extern int aftersearch;

/*
 * this is main function which handles almost all of the work (keyboard
 * actions while viewing info). Arguments:
 * message: a pointer to char** node content, stored line by line.
 * type: a pointer to char*, which holds the header of info node.
 * lines: pointer to a long, which holds the number of lines in node.
 * id: file descriptor of current info file
 * tag_table_pos: position in tag table of the current node (needed for history)
 */
WorkRVal work (char ***message, char **type, unsigned long *lines,
		FILE * id, int tag_table_pos);
#endif
