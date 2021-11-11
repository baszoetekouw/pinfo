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


#include "common_includes.h"

void info_add_highlights(unsigned pos, unsigned cursor, unsigned long lines, unsigned column, char **message);

void
substitutestr(char *src, char *dest, char *from, char *to)
	/*
	 * Utility for substituting strings in given string.
	 * Used for internationalization of info headers.
	 */
{
	char *start = strstr(src, from);
	char tmp;
	if (!start)
		strcpy(dest, src);
	else
	{
		tmp = *start;
		*start = 0;
		strcpy(dest, src);
		strcat(dest, to);
		*start = tmp;
		start += strlen(from);
		strcat(dest, start);
	}
}

void
addtopline(char *type, int column)
{
	char *buf1 = xmalloc(strlen(type) + 50);
	char *buf2 = xmalloc(strlen(type) + 50);
	int buf2len;
	strcpy(buf1, type);

	substitutestr(buf1, buf2, "File:", _("File:"));
	substitutestr(buf2, buf1, "Node:", _("Node:"));
	substitutestr(buf1, buf2, "Next:", _("Next:"));
	substitutestr(buf2, buf1, "Prev:", _("Prev:"));
	substitutestr(buf1, buf2, "Up:", _("Up:"));
	attrset(topline);
	mymvhline(0, 0, ' ', maxx);	/* pads line with spaces -- estetic */
	buf2len=strlen(buf2);
	if (buf2len)
		buf2[buf2len - 1] = '\0';
	if (buf2len>column)
		mvaddstr(0, 0, buf2+column);
	attrset(normal);
	xfree(buf1);
	xfree(buf2);
}

void
showscreen(char **message, unsigned long lines, unsigned long pos, long cursor, int column)
{
#ifdef getmaxyx
	getmaxyx(stdscr, maxy, maxx);
#endif
#ifdef HAVE_BKGDSET
	bkgdset(' ' | normal);
#endif
	attrset(normal);
	for (unsigned long i = pos; (i < lines) && (i < pos + maxy - 2); i++)
	{
		int tmp;

		if (!message[i]) continue;
		tmp = strlen(message[i]) - 1;
		message[i][tmp] = 0;
		if (tmp>column)
			mvaddstr(i + 1 - pos, 0, message[i]+column);
		else
			move(i + 1 - pos,0);
#ifdef HAVE_BKGDSET
		clrtoeol();
#else
		myclrtoeol();
#endif
		message[i][tmp] = '\n';
	}
	clrtobot();
#ifdef HAVE_BKGDSET
	bkgdset(0);
#endif
	attrset(bottomline);
	mymvhline(maxy - 1, 0, ' ', maxx);
	move(maxy - 1, 0);
	if ((pos < lines - 1) &&(lines > pos + maxy - 2))
		printw(_("Viewing line %ld/%ld, %ld%%"), pos + maxy - 2, lines,((pos + maxy - 2) * 100) / lines);
	else
		printw(_("Viewing line %ld/%ld, 100%%"), lines, lines);
	info_add_highlights(pos, cursor, lines, column, message);
	attrset(normal);
	move(0, 0);
	refresh();
}

/*
 * prints a line, taking care for the horizontal scrolling.
 *  if the string fits in the window, it is drawn. If not,
 *  it is either cut, or completely omitted.
 */
void
info_addstr(int y, int x, char *txt, int column, int txtlen)
{
  int xmax, UNUSED(ymax);
  getmaxyx(stdscr, ymax, xmax);
  /* Use xmax and mvaddnstr to force clipping.
   * Fairly blunt instrument, but the best I could come up with.
   * Breaks in the presence of tabs; I don't see how to handle them. */
	if (x>column)
		mvaddnstr(y,x-column,txt, xmax-(x-column) );
	else if (x+txtlen>column)
		mvaddnstr(y,0,txt+(column-x), xmax );
#ifdef __DEBUG__
  refresh();
#endif /* __DEBUG__ */
}

void
info_add_highlights(unsigned pos, unsigned cursor, unsigned long lines, unsigned column, char **message)
{
	for (unsigned long i = 0; i < hyperobjectcount; i++)
	{
		if ((hyperobjects[i].line >= pos) &&
				(hyperobjects[i].line < pos +(maxy - 2)))
		{
			/* first part of if's sets the required attributes */
			if (hyperobjects[i].type < 2)		/* menu */
			{
				if (i == cursor)
					attrset(menuselected);
				else
					attrset(menu);
			}
			else if (hyperobjects[i].type < 4)	/* note */
			{
				if (i == cursor)
					attrset(noteselected);
				else
					attrset(note);
			}
			else if (hyperobjects[i].type < HIGHLIGHT)	/* url */
			{
				if (i == cursor)
					attrset(urlselected);
				else
					attrset(url);
			}
			else /* quoted text -- highlight it */
			{
				attrset(infohighlight);
			}
			/* now we start actual drawing */
			if (hyperobjects[i].file[0] == 0)
			{
				if (hyperobjects[i].breakpos == -1)
				{
					info_addstr(1 + hyperobjects[i].line - pos,
							hyperobjects[i].col,
							hyperobjects[i].node,
							column,
							hyperobjects[i].nodelen);

				}
				else
				{
					int j;
					char tmp = hyperobjects[i].node[hyperobjects[i].breakpos];
					hyperobjects[i].node[hyperobjects[i].breakpos] = 0;
					info_addstr(1 + hyperobjects[i].line - pos,
							hyperobjects[i].col,
							hyperobjects[i].node,
							column,
							hyperobjects[i].breakpos);
					hyperobjects[i].node[hyperobjects[i].breakpos] = tmp;
					j = hyperobjects[i].breakpos;
					/* skip leading spaces after newline */
					while (hyperobjects[i].node[j] == ' ')
						j++;
					if (hyperobjects[i].line - pos + 3 < maxy)
						info_addstr(1 + hyperobjects[i].line - pos + 1,
								j - hyperobjects[i].breakpos,
								hyperobjects[i].node + j,
								column,
								hyperobjects[i].nodelen-j);
				}
			}
			else
			{
				if (hyperobjects[i].breakpos == -1)
				{
					char *buf=xmalloc(hyperobjects[i].filelen+hyperobjects[i].nodelen+3);
					snprintf(buf,hyperobjects[i].filelen+hyperobjects[i].nodelen+3,
							"(%s)%s",hyperobjects[i].file,hyperobjects[i].node);
					info_addstr(1 + hyperobjects[i].line - pos,
							hyperobjects[i].col,
							buf,
							column,
							hyperobjects[i].filelen+hyperobjects[i].nodelen+2);
					xfree(buf);
				}
				else
				{
					static char buf[1024];
					char tmp;
					int j;
					strcpy(buf, "(");
					strcat(buf, hyperobjects[i].file);
					strcat(buf, ")");
					strcat(buf, hyperobjects[i].node);
					tmp = buf[hyperobjects[i].breakpos];
					buf[hyperobjects[i].breakpos] = 0;
					info_addstr(1 + hyperobjects[i].line - pos,
							hyperobjects[i].col,
							buf,
							column,
							hyperobjects[i].breakpos+2);
					buf[hyperobjects[i].breakpos] = tmp;
					j = hyperobjects[i].breakpos;
					/* skip leading spaces after newline */
					while (buf[j] == ' ')
						j++;
					if (hyperobjects[i].line - pos + 3 < maxy)
						info_addstr(1 + hyperobjects[i].line - pos + 1,
								j - hyperobjects[i].breakpos,
								buf + j,
								column,
								hyperobjects[i].filelen+hyperobjects[i].nodelen+2-j);
				}
			}
			attrset(normal);
		}
	}
#ifndef ___DONT_USE_REGEXP_SEARCH___
	if ((h_regexp_num) ||(aftersearch))
	{
		regmatch_t pmatch[1];
		if (maxy<2) maxy=2;
		unsigned long maxpos = pos +(maxy - 2);
		int maxregexp;
		if (maxpos > lines)
		{
			maxpos = lines;
		}

		maxregexp = aftersearch ? h_regexp_num + 1 : h_regexp_num;
		/*
		 * if it is after search, then we have user defined regexps+
		 * a searched regexp to highlight
		 */
		/* loop over all the lines currently in the window */
		for (unsigned i = pos; (i < lines) && (i < pos + maxy - 2); i++)
		{
			char *str = message[i];

			/* loop over all regexps we might want to show */
			int j;
			for (j = 0; j < maxregexp; j++)
			{
				/* check if this regexp is present on this line */
				while (!regexec(&h_regexp[j], str, 1, pmatch, 0))
				{
					int x, y;
					char tmp;

					/* yes, found something, so highlight it */
					int n = pmatch[0].rm_eo - pmatch[0].rm_so;

					if (n==0) { /* matched empty string! */
						/* display error message */
						char msg[81];
						snprintf(msg, 81, "%s",
								_("Warning: matched empty string") );
						attrset(bottomline);
						mvhline(maxy - 1, 0, ' ', maxx);
						mvaddstr(maxy - 1, 0, msg);
						move(0, 0);
						attrset(normal);

						break;
					}

					/* point str at start of match */
					str += pmatch[0].rm_so;

					/* calculate position on screen */
					x = calculate_len(message[i], str);
					y = i - pos + 1;

					/* save the char after the end of the match, 
					 * and replace it by \0 */
					tmp = str[n];
					str[n] = 0;
					
					/* write out the highlighted match to screen */
					attrset(searchhighlight);
					mvaddstr(y, x, str);
					attrset(normal);

					/* restore the original char at the end of the match */
					str[n] = tmp;

					/* skip past this match */
					str += n;
				}
			}
		}
	}

#endif
}
