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
#include <vector>
using std::vector;

void info_add_highlights(int pos, int cursor, int column, const vector <string> message);

/*
 * Replace first occurence of substring in string.
 * Used for internationalization of info headers.
 */
static void
substitutestring(string& strbuf, string find, string replace) {
 	string::size_type loc = strbuf.find(find);
	if (loc != string::npos) {
		strbuf.replace(loc, find.length(), replace);
	}
}

void
addtopline(const string type, string::size_type column)
{
	string strbuf = type;
	if (strbuf[strbuf.length() - 1] == '\n') {
		/* This happened and broke the decorative filler */
		strbuf.resize(strbuf.length() - 1);
	}

	substitutestring(strbuf,"File:", _("File:"));
	substitutestring(strbuf, "Node:", _("Node:"));
	substitutestring(strbuf, "Next:", _("Next:"));
	substitutestring(strbuf, "Prev:", _("Prev:"));
	substitutestring(strbuf, "Up:", _("Up:"));

	attrset(topline);

	/* pads line with spaces -- aesthetic */
	mymvhline(0, 0, ' ', maxx);

	if (strbuf.length() > column) {
		string clipped;
		clipped = strbuf.substr(column, string::npos);
		mvaddstr(0, 0, clipped.c_str());
	}
	attrset(normal);
}

void
showscreen(const vector <string> message, long pos, long cursor, int column)
{
	/* pos is 1-based, message is 0-based */
#ifdef getmaxyx
	getmaxyx(stdscr, maxy, maxx);
#endif
#ifdef HAVE_BKGDSET
	bkgdset(' ' | normal);
#endif
	attrset(normal);
	for (long i = pos - 1; (i < message.size()) && (i + 1 < pos + maxy - 2); i++)
	{
		/* Chop off trailing newline */
		string tmpstr = message[i].substr(0, message[i].length() - 1);
		if (tmpstr.length()>column)
			mvaddstr(i + 2 - pos, 0, tmpstr.substr(column).c_str());
		else
			move(i + 2 - pos,0);
#ifdef HAVE_BKGDSET
		clrtoeol();
#else
		myclrtoeol();
#endif
	}
	clrtobot();
#ifdef HAVE_BKGDSET
	bkgdset(0);
#endif
	attrset(bottomline);
	mymvhline(maxy - 1, 0, ' ', maxx);
	move(maxy - 1, 0);
	if ((pos < message.size() - 1) &&(message.size() > pos + maxy - 2))
		printw(_("Viewing line %d/%d, %d%%"), pos + maxy - 2,
		       message.size(), ((pos + maxy - 2) * 100) / message.size());
	else
		printw(_("Viewing line %d/%d, 100%%"), message.size(), message.size());
	info_add_highlights(pos, cursor, column, message);
	attrset(normal);
	move(0, 0);
	refresh();
}

/*
 *  Prints a line, taking care for the horizontal scrolling.
 *  If the string fits in the window, it is drawn. If not,
 *  it is either cut, or completely omitted.
 *
 *  Does not alter the string passed to it.
 */
void
info_addstring(int y, string::size_type x, string txt, string::size_type column)
{
  /* Use maxx and mvaddnstr to force clipping.
   * Fairly blunt instrument, but the best I could come up with.
   * Breaks in the presence of tabs; I don't see how to handle them. */
	if (x > column)
		mvaddnstr(y, x-column, txt.c_str(), maxx-(x-column) );
	else if (x + txt.length() > column) {
		string clipped;
		clipped = txt;
		clipped.erase(0, column - x);
		mvaddnstr(y, 0, clipped.c_str(), maxx );
  }
#ifdef __DEBUG__
  refresh();
#endif /* __DEBUG__ */
}

void
info_add_highlights(int pos, int cursor, int column, const vector <string> message)
{
	for (typeof(hyperobjects.size()) i = 0; i < hyperobjects.size(); i++) {
		if ((hyperobjects[i].line < pos - 1) ||
				(hyperobjects[i].line >= pos - 1 +(maxy - 2)))
			continue; /* Off screen */

		/* first set of ifs sets the required attributes */
		if (hyperobjects[i].type < 2)	{	/* menu */
			if (i == cursor)
				attrset(menuselected);
			else
				attrset(menu);
		} else if (hyperobjects[i].type < 4) {	/* note */
			if (i == cursor)
				attrset(noteselected);
			else
				attrset(note);
		}	else if (hyperobjects[i].type < HIGHLIGHT) {	/* url */
			if (i == cursor)
				attrset(urlselected);
			else
				attrset(url);
		} else  { /* quoted text -- highlight it */
			attrset(infohighlight);
		}

		/* now we start actual drawing */
		string mynode;
		if (hyperobjects[i].file == "") {
			mynode = hyperobjects[i].node;
		} else {
			mynode = "(";
			mynode += hyperobjects[i].file;
			mynode += ")";
			mynode += hyperobjects[i].node;
		}
		if (hyperobjects[i].breakpos == -1) {
			info_addstring(1 + hyperobjects[i].line + 1 - pos,
					hyperobjects[i].col,
					mynode,
					column);
		} else {
			int j;
			string part1, part2;
			part1 = mynode.substr(0, hyperobjects[i].breakpos);
			info_addstring(1 + hyperobjects[i].line + 1 - pos,
					hyperobjects[i].col,
					part1,
					column);
			j = hyperobjects[i].breakpos;
			/* skip leading spaces after newline */
			while (mynode[j] == ' ')
				j++;
			part2 = mynode.substr(j, string::npos);
			if (hyperobjects[i].line + 1 - pos + 3 < maxy)
				info_addstring(1 + hyperobjects[i].line + 1 - pos + 1,
						j - hyperobjects[i].breakpos,
						part2,
						column);
		}
		attrset(normal);
	}

#ifndef ___DONT_USE_REGEXP_SEARCH___
	if (h_regexp.size() > 0) {
		regmatch_t pmatch[1];
		for (int i = pos - 1; 
		     (i < message.size()) && (i + 1 < pos + (maxy - 2)); i++) {
			for (int j = 0; j < h_regexp.size(); j++) {
				const char * message_i = message[i].c_str();
				const char *rest_of_str = message_i;
				while (!regexec(&h_regexp[j], rest_of_str, 1, pmatch, 0)) {
					int num_chars = pmatch[0].rm_eo - pmatch[0].rm_so;
					int x = calculate_len(message_i, rest_of_str + pmatch[0].rm_so);
					int txtoffset = (rest_of_str - message_i) + pmatch[0].rm_so;
					string tmpstr = message[i].substr(txtoffset, num_chars);
					attrset(searchhighlight);
					mvaddstr(i + 1 - pos + 1, x, tmpstr.c_str());
					attrset(normal);
					rest_of_str = rest_of_str + pmatch[0].rm_eo;
				}
			}
		}
	}
	/* Duplicate code, this time for the interactive search. */
	if (regex_is_current) {
		regmatch_t pmatch[1];
		for (int i = pos - 1; 
		     (i < message.size()) && (i + 1 < pos + (maxy - 2)); i++) {
			const char * message_i = message[i].c_str();
			const char *rest_of_str = message_i;
			while (!regexec(&current_regex, rest_of_str, 1, pmatch, 0)) {
				int num_chars = pmatch[0].rm_eo - pmatch[0].rm_so;
				int x = calculate_len(message_i, rest_of_str + pmatch[0].rm_so);
				int txtoffset = (rest_of_str - message_i) + pmatch[0].rm_so;
				string tmpstr = message[i].substr(txtoffset, num_chars);
				attrset(searchhighlight);
				mvaddstr(i + 1 - pos + 1, x, tmpstr.c_str());
				attrset(normal);
				rest_of_str = rest_of_str + pmatch[0].rm_eo;
			}
		}
	}
#endif
}
