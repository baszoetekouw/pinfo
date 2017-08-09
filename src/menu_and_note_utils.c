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


/* read the `Next:' header entry */
void
getnextnode(char *type, char *node)
{
	int j;
#ifndef ___USE_STATIC___
	char *tmp = xmalloc(strlen(type) + 1);
#else
	static char tmp[1024 + 1];
#endif
	char *wsk;
	strcpy(tmp, type);
	wsk = strstr(tmp, "Next: ");
	if (wsk == 0)
	{
		strcpy(node, ERRNODE);
		return;
	}
	for (j = 6; wsk[j] != 0; j++)
	{
		if ((wsk[j] == ',') ||(wsk[j] == '\n'))
		{
			wsk[j] = 0;
			strcpy(node, wsk + 6);
#ifndef ___USE_STATIC___
			xfree(tmp);
#endif
			return;
		}
	}
#ifndef ___USE_STATIC___
	xfree(tmp);
#endif
}

/* read the `Prev:' header entry */
void
getprevnode(char *type, char *node)
{
	int j;
#ifndef ___USE_STATIC___
	char *tmp = xmalloc(strlen(type) + 1);
#else
	static char tmp[1024 + 1];
#endif
	char *wsk;
	strcpy(tmp, type);
	wsk = strstr(tmp, "Prev: ");
	if (wsk == 0)
	{
		strcpy(node, ERRNODE);
		return;
	}
	for (j = 6; wsk[j] != 0; j++)
	{
		if ((wsk[j] == ',') ||(wsk[j] == '\n'))
		{
			wsk[j] = 0;
			strcpy(node, wsk + 6);
#ifndef ___USE_STATIC___
			xfree(tmp);
#endif
			return;
		}
	}
#ifndef ___USE_STATIC___
	xfree(tmp);
#endif
}

/* read the `Up:' header entry */
void
getupnode(char *type, char *node)
{
	int j;
#ifndef ___USE_STATIC___
	char *tmp = xmalloc(strlen(type) + 1);
#else
	static char tmp[1024 + 1];
#endif
	char *wsk;
	strcpy(tmp, type);
	wsk = strstr(tmp, "Up: ");
	if (wsk == 0)
	{
		strcpy(node, ERRNODE);
		return;
	}
	for (j = 4; wsk[j] != 0; j++)
	{
		if ((wsk[j] == ',') ||(wsk[j] == '\n'))
		{
			wsk[j] = 0;
			strcpy(node, wsk + 4);
#ifndef ___USE_STATIC___
			xfree(tmp);
#endif
			return;
		}
	}
#ifndef ___USE_STATIC___
	xfree(tmp);
#endif
}


/* read the `Node:' header entry */
void
getnodename(char *type, char *node)
{
	int j;
#ifndef ___USE_STATIC___
	char *tmp = xmalloc(strlen(type) + 1);
#else
	static char tmp[1024 + 1];
#endif
	char *wsk;
	strcpy(tmp, type);
	wsk = strstr(tmp, "Node: ");
	if (wsk == 0)
	{
		strcpy(node, ERRNODE);
		return;
	}
	for (j = 6; wsk[j] != 0; j++)
	{
		if ((wsk[j] == ',') ||(wsk[j] == '\n'))
		{
			wsk[j] = 0;
			strcpy(node, wsk + 6);
#ifndef ___USE_STATIC___
			xfree(tmp);
#endif
			return;
		}
	}
#ifndef ___USE_STATIC___
	xfree(tmp);
#endif
}
