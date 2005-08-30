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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 ***************************************************************************/

#include "common_includes.h"
#include <string>
using std::string;

RCSID("$Id$")

void
freeindirect()
{
	if (indirect)
	{
		xfree(indirect);
		indirect = 0;
	}
	IndirectEntries = 0;
}

void
freetagtable()
{
	if (tag_table)
	{
		xfree(tag_table);
		tag_table = 0;
	}
	TagTableEntries = 0;
}

/*
 * Read the `$foo' header entry
 * Eliminates former duplicate code
 */
static inline void
get_foo_node(const char * const foo, char *type, char *node)
{
	string tmpstr = type;
	string::size_type start_idx;
	start_idx = tmpstr.find(foo);
	if (start_idx == string::npos) {
		strcpy(node, ERRNODE);
		return;
	}

	start_idx += strlen(foo);
	string::size_type end_idx;
	end_idx = tmpstr.find_first_of(",\n", start_idx);
	if (end_idx != string::npos) {
		strcpy(node, tmpstr.substr(start_idx, end_idx - start_idx).c_str() );
	}
	/* Otherwise what?  EOF? */
}

/* read the `Next:' header entry */
void
getnextnode(char *type, char *node)
{
	get_foo_node("Next: ", type, node);
}

/* read the `Prev:' header entry */
void
getprevnode(char *type, char *node)
{
	get_foo_node("Prev: ", type, node);
}

/* read the `Up:' header entry */
void
getupnode(char *type, char *node)
{
	get_foo_node("Up: ", type, node);
}


/* read the `Node:' header entry */
void
getnodename(char *type, char *node)
{
	get_foo_node("Node: ", type, node);
}
