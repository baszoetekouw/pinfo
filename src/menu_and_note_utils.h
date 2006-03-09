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

#ifndef __MENU_AND_NOTE_UTILS_H
#define __MENU_AND_NOTE_UTILS_H

#define ERRNODE "ERR@!#$$@#!%%^#@!OR"

/* checks whether a line contains menu */
int ismenu (const char *line);
/* checks whether a line contains note */
int isnote (char *line, char *nline);
/* reads menu token from line */
int getmenutoken (char *line);
/* reads note token from line */
int getnotetoken (char *line, char *nline);
/* gets nextnode token from top line */
void getnextnode (char *type, char *node);
/* gets prevnode token from top line */
void getprevnode (char *type, char *node);
/* gets the up node token from top line */
void getupnode (char *type, char *node);
/* reads the nodename from top line */
void getnodename (char *type, char *node);
void freeindirect ();
void freetagtable ();
#endif
