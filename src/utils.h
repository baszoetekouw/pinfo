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

#ifndef __UTILS_H
#define __UTILS_H

#include "datatypes.h"
#include <string>

extern std::string safe_user;
extern std::string safe_group;

/* bail out if file name causes security problems */
void checkfilename (const std::string filename);

/* strcmp, which is insensitive to whitespaces */
bool compare_tags (TagTable a, TagTable b);

/* get offset of "node" in tag_table variable */
int gettagtablepos (const std::string & node);

/* Explode a string into a vector */
std::vector<std::string>
string_explode(const std::string & to_explode,
               std::string::value_type separator);

/* Return a string converted to uppercase */
std::string string_toupper (std::string s);

/*
 * calculates the length of string between start and end, counting `\t' as
 * filling up to 8 chars. (i.e. at line 22 tab will increment the counter by 2
 * [8-(22-int(22/8)*8)] spaces)
 */
int
calculate_len(const char *start, const char *end);

/*
 *  * create a temporary file in a safe way, and return its name in a newly
 *   * allocated string
 *    */
char *
make_tempfile();


#endif
