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

char **rlhistory = 0;
int rlhistorylen = 0;
int rlhistorypos = 0;

#define KEY_BCKSPC 8

char *
readlinewrapper(char *prompt)
{
	/* number of keys pressed */
	int numkeys = 0;
	/* initial buffer for the read line */
	char *buf = xmalloc(1024);
	/* start coords of input line */
	int origx, origy;
	/* cursor position in input string */
	int cursor = 0;
	/* key - a variable for getch() */
	int key = 0, i;
	/* initial value of line - "" */
	buf[0] = 0;
	/* print prompt */
	addstr(prompt);
	/* get origx,origy coordinates */
	getyx(stdscr, origy, origx);
	/* turn off echoing chars by getch() */
	noecho();
	/* create input line bar */
	mvhline(origy, origx, ' ', maxx - origx);

	/* history entry for this line */
	rlhistorylen++;
	/* move history pos to current entry */
	rlhistorypos = rlhistorylen;
	/* alloc memory for this entry */
	if (!rlhistory)
		rlhistory = xmalloc(sizeof(char *));
	else
		rlhistory = xrealloc(rlhistory, sizeof(char *) * rlhistorylen);
	rlhistory[rlhistorylen - 1] = xmalloc(1024);
	/* and copy there the current value of input line */
	strcpy(rlhistory[rlhistorylen - 1], buf);
	/* call history to be present */
	if (CallReadlineHistory)
	{
		ungetch(KEY_UP);
		numkeys = -1;
	}

	while (key != '\n')
	{
		/* read key */
		key = getch();
		switch(key)
		{
			/* move cursor left */
			case KEY_LEFT:
				if (cursor > 0)
					cursor--;
				break;
			/* move cursor right */
			case KEY_RIGHT:
				if (cursor < strlen(buf))
					cursor++;
				break;
			case KEY_END:
				cursor = strlen(buf);
				break;
			/* handle backspace: copy all */
			case KEY_BCKSPC:
			/* chars starting from curpos */
			case KEY_BACKSPACE:
				/* - 1 from buf[n+1] to buf   */
				if (cursor > 0)
				{
					for (i = cursor - 1; buf[i] != 0; i++)
						buf[i] = buf[i + 1];
					cursor--;
				}
				break;
			/* handle delete key. As above */
			case KEY_DC:
				if (cursor <= strlen(buf) - 1)
				{
					for (i = cursor; buf[i] != 0; i++)
						buf[i] = buf[i + 1];
				}
				break;
			/* backwards-history call */
			case KEY_UP:
				/* if there is history */
				if (rlhistorylen)
					/* and we have */
					if (rlhistorypos > 1)
					{			/* where to move */
						/* decrement history position */
						rlhistorypos--;
						/* if the previous pos was the input line */
						/* save it's value to history */
						if (rlhistorypos == rlhistorylen - 1)
							strcpy(rlhistory[rlhistorylen - 1], buf);
						/*  recall value from history to input buf */
						strcpy(buf, rlhistory[rlhistorypos - 1]);
					}
				cursor = strlen(buf);
				numkeys = -1;
				break;
			/* forwards-history call */
			case KEY_DOWN:
				if (rlhistorylen)
					if (rlhistorypos < rlhistorylen)
					{
						rlhistorypos++;
						strcpy(buf, rlhistory[rlhistorypos - 1]);
					}
				cursor = strlen(buf);
				numkeys = -1;
				break;
				/* eliminate nonprintable chars */
			case '\n':
			case KEY_PPAGE:
			case KEY_NPAGE:
			case KEY_F(1):
			case KEY_F(2):
			case KEY_F(3):
			case KEY_F(4):
			case KEY_F(5):
			case KEY_F(6):
			case KEY_F(7):
			case KEY_F(8):
			case KEY_F(9):
			case KEY_F(10):
				break;
			default:
				if (key >= 32)
				{
					/* if this is the first key, delete the buffer */
					if (numkeys==0 && cursor!=0)
					{
						for (i=0; buf[i]!=0; i++)
							buf[i] = 0;
						cursor = 0;
						/* and empty the line */
						move(origy, origx);
						for (i = origx; i < maxx; i++)
							addch(' ');
						move(origy, origx + cursor);
					}
					
					/* if the cursor is not at the last pos */
					if (strlen(buf + cursor))
					{
						char *tmp = 0;
						tmp = xmalloc(strlen(buf + cursor) + 1);
						strcpy(tmp, buf + cursor);
						buf[cursor] = key;
						buf[cursor + 1] = 0;
						strcat(&buf[cursor + 1], tmp);
						xfree(tmp);
						cursor++;
					}
					else
					{
						buf[cursor + 1] = 0;
						buf[cursor] = key;
						cursor++;
					}
				}
		}
		move(origy, origx);
		for (i = origx; i < maxx; i++)
			addch(' ');
		move(origy, origx);
		addstr(buf);
		move(origy, origx + cursor);

		numkeys++;

	}
	strcpy(rlhistory[rlhistorylen - 1], buf);
	if (strlen(buf))
	{
		rlhistory[rlhistorylen - 1] = xrealloc(rlhistory[rlhistorylen - 1],
				strlen(rlhistory[rlhistorylen - 1]) + 1);
	}
	else
	{
		xfree(rlhistory[rlhistorylen - 1]);
		rlhistorylen--;
		rlhistorypos = rlhistorylen;
	}
	buf = xrealloc(buf, strlen(buf) + 1);
	return buf;
}
