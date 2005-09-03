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

vector<InfoHistory> infohistory;

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
	infohistory.clear();
}

/*
 * Add history entry
 */
void
addinfohistory(const char *file, const char *node, int cursor, int menu, int pos)
{
	InfoHistory my_hist;
	my_hist.node = node;
	my_hist.file = file;
	my_hist.pos = pos;
	my_hist.cursor = cursor;
	my_hist.menu = menu;
	infohistory.push_back(my_hist);
}

/*
 * Delete last history entry
 */
void
dellastinfohistory()
{
	if (infohistory.empty()) {
		return;
	}
	infohistory.pop_back();
}

