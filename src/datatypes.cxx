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
#include <vector>
using std::string;
using std::vector;

RCSID("$Id$")

int verbose = 1;

string filenameprefix;

string httpviewer = "lynx";
string ftpviewer = "lynx";
string maileditor = "mail";
string printutility = "lpr";
string manlinks = "1:8:2:3:4:5:6:7:9:n:l:p:o:3X11:3Xt:3X:3x";
string configuredinfopath = "/usr/share/info:/usr/local/share/info:/opt/info";
string ignoredmacros = "";
string rcfile = "";

char *tmpfilename1 = 0;
char *tmpfilename2 = 0;

SearchAgain searchagain;

vector<HyperObject> hyperobjects;

vector<Indirect> indirect;
vector<TagTable> tag_table;
long FirstNodeOffset = 0;
string FirstNodeName;
int maxx, maxy;
int CutManHeaders = 0;
int CutEmptyManLines = 0;
int ForceManualTagTable = 0;
int LongManualLinks = 0;
string ManOptions = "";
string StderrRedirection = "2> /dev/null";
int FilterB7 = 0;
int ConfirmQuit = 0;
int QuitConfirmDefault = 0;
int ClearScreenAtExit = 0;
int CallReadlineHistory = 1;

InfoHistory infohistory;

int npos = -1;
int ncursor = -1;
int nmenu = -1;
int use_apropos = 0;
int plain_apropos = 0;
int use_manual = 0;
int use_raw_filename = 0;
int quote_ignored = 0;

int winchanged = 0;

void
inithistory()
{
	infohistory.length = 0;
	infohistory.node = 0;
	infohistory.file = 0;
	infohistory.pos = 0;
	infohistory.cursor = 0;
	infohistory.menu = 0;
}

/*
 * Add history entry
 */
void
addinfohistory(const char *file, const char *node, int cursor, int menu, int pos)
{
	if (!infohistory.length)
	{
		infohistory.length++;
		infohistory.node = (char**)xmalloc(sizeof(char *) * 2);
		infohistory.node[0] = 0;
		infohistory.file = (char**)xmalloc(sizeof(char *) * 2);
		infohistory.file[0] = 0;
		infohistory.pos = (int*)xmalloc(sizeof(int) * 2);
		infohistory.cursor = (int*)xmalloc(sizeof(int) * 2);
		infohistory.menu = (int*)xmalloc(sizeof(int) * 2);
	}
	else
	{
		infohistory.length++;
		infohistory.node = (char**)xrealloc(infohistory.node, sizeof(char *) *(infohistory.length + 1));
		infohistory.file = (char**)xrealloc(infohistory.file, sizeof(char *) *(infohistory.length + 1));
		infohistory.pos = (int*)xrealloc(infohistory.pos, sizeof(int) *(infohistory.length + 1));
		infohistory.cursor = (int*)xrealloc(infohistory.cursor, sizeof(int) *(infohistory.length + 1));
		infohistory.menu = (int*)xrealloc(infohistory.menu, sizeof(int) *(infohistory.length + 1));
	}
	infohistory.node[infohistory.length] = (char*)xmalloc(strlen(node) + 1);
	strcpy(infohistory.node[infohistory.length], node);
	infohistory.file[infohistory.length] = (char*)xmalloc(strlen(file) + 1);
	strcpy(infohistory.file[infohistory.length], file);
	infohistory.pos[infohistory.length] = pos;
	infohistory.cursor[infohistory.length] = cursor;
	infohistory.menu[infohistory.length] = menu;
}

/*
 * Delete last history entry
 */
void
dellastinfohistory()
{
	if (infohistory.length)
	{
		if (infohistory.node[infohistory.length])
		{
			xfree(infohistory.node[infohistory.length]);
			infohistory.node[infohistory.length] = 0;
		}
		if (infohistory.file[infohistory.length])
		{
			xfree(infohistory.file[infohistory.length]);
			infohistory.file[infohistory.length] = 0;
		}
		if (infohistory.length)
			infohistory.length--;
		if (infohistory.length)
		{
			infohistory.node = (char**)xrealloc(infohistory.node, sizeof(char *) *(infohistory.length + 1));
			infohistory.file = (char**)xrealloc(infohistory.file, sizeof(char *) *(infohistory.length + 1));
			infohistory.pos = (int*)xrealloc(infohistory.pos, sizeof(int) *(infohistory.length + 1));
			infohistory.cursor = (int*)xrealloc(infohistory.cursor, sizeof(int) *(infohistory.length + 1));
			infohistory.menu = (int*)xrealloc(infohistory.menu, sizeof(int) *(infohistory.length + 1));
		}
		else
		{
			if (infohistory.node)
			{
				xfree(infohistory.node);
				infohistory.node = 0;
			}
			if (infohistory.file)
			{
				xfree(infohistory.file);
				infohistory.file = 0;
			}
			if (infohistory.pos)
			{
				xfree(infohistory.pos);
				infohistory.pos = 0;
			}
			if (infohistory.cursor)
			{
				xfree(infohistory.cursor);
				infohistory.cursor = 0;
			}
			if (infohistory.menu)
			{
				xfree(infohistory.menu);
				infohistory.menu = 0;
			}
		}
	}
}

