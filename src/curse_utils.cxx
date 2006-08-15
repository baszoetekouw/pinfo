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

#include <string>
using std::string;

#include <clocale> // for setlocale

#include "colors.h"
#include "datatypes.h"
#include "keyboard.h"
#include "tmpfiles.h"

/* Readline */
#include <readline/readline.h>
#include <readline/history.h>
#include <term.h>

#ifndef HAVE_CURS_SET
void
curs_set(int a)
{
}
#endif

int curses_open = 0;

int shell_cursor = 1;

void
initlocale()
{
#ifdef HAVE_SETLOCALE
	std::setlocale(LC_ALL, "");
#endif
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
}

void
mymvhline(int y, int x, char ch, int len)
{
	int i;
	for (i = 0; i < len; i++)
		mvaddch(y, x + i, ch);
}

/* custom function that readline will use to display text */
void
my_rl_display()
{
	/* go to the bottom line, empty it, and print the prompt and buffer */
	attrset(bottomline);
	mymvhline(maxy - 1, 0, ' ', maxx);
	move(maxy-1,0);
	printw("%s%s", rl_prompt, rl_line_buffer);
	refresh();
}

string
getstring(const char *prompt)
{
	char *buf;

	curs_set(1);
	move(maxy - 1, 0);
	refresh();

	rl_readline_name = PACKAGE;
	
	/* set display function for readline to my_rl_display and call readline */
	rl_redisplay_function = my_rl_display;
	buf = readline(prompt);
	if (buf && *buf) 
		add_history(buf);
	
	curs_set(0);

	string my_string;
	if (buf == NULL) {
		my_string = "";
	} else {
		my_string = buf;
		free(buf);
	}
	return my_string;
}

void
init_curses()
{
	initscr();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	/*  meta(stdscr, TRUE); */
	initcolors();
	shell_cursor = curs_set(0);
#ifdef NCURSES_MOUSE_VERSION
	mousemask(BUTTON1_CLICKED | BUTTON1_DOUBLE_CLICKED, NULL);
/* NCURSES_MOUSE_VERSION */
#endif
	curses_open = 1;
}

int
pinfo_getch()
{
	int key = getch();
	/* following key will be alt's value */
	if (key == META_KEY)
	{
		key = getch();
		key |= 0x200;
	}
	return key;
}

void
waitforgetch()
{
	fd_set rdfs;
	FD_ZERO(&rdfs);
	FD_SET(0, &rdfs);
	select(1, &rdfs, NULL, NULL, NULL);
}

int
yesno(const char *prompt, int def)
{
	const char *yes = _("yes");
	const char *no = _("no");
	int key;

	attrset(bottomline);
	mymvhline(maxy - 1, 0, ' ', maxx);
	move(maxy - 1, 0);
	/* if default answer is yes */
	if (def)
		printw("%s([%c]/%c)", prompt, *yes, *no);
	else
		printw("%s([%c]/%c)", prompt, *no, *yes);
	nodelay(stdscr, FALSE);
	while (1)
	{
		key = getch();
		if (key == ERR)
			return -1;
		if (is_enter_key(key))
			break;
		else
		{
			if (tolower(key) == tolower(*yes))
			{
				def = 1;
				break;
			}
			else
			{
				if (tolower(key) == tolower(*no))
				{
					def = 0;
					break;
				}
				else
					beep();
			}
		}
	}

	nodelay(stdscr, TRUE);
	if (def)
		addstr(yes);
	else
		addstr(no);
	attrset(normal);
	return def;
}

void
myclrtoeol()
{
	int x, y, i;
	getyx(stdscr, y, x);
	for (i = x; i < maxx; i++)
		mvaddch(y, i, ' ');
}

void
myendwin()
{
	curs_set(shell_cursor);
	endwin();
}

void
handlewinch()
{
	myendwin();
	init_curses();
	doupdate();
	getmaxyx(stdscr, maxy, maxx);
	ungetch(keys.refresh_1);
}

void
closeprogram()
{
	if (curses_open)
		myendwin();
	if (ClearScreenAtExit)
		system("clear");
	else
		printf("\n");
	rmtmpfiles();
}
