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
#include "tmpfiles.h"
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <regex.h>
#include <ctype.h>

string safe_user = "nobody";
string safe_group = "nogroup";

#ifndef HAVE_CURS_SET
void
curs_set(int a)
{
}
#endif

#ifdef ___DONT_USE_REGEXP_SEARCH___
char *pinfo_re_pattern = 0;
#else
int pinfo_re_offset = -1;
#endif

/* Readline */
#include <readline/readline.h>
#include <readline/history.h>
#include <term.h>


/*
 * the below define enables malloc/realloc/free logging to stderr.
 * They start to log their argument values.
 *
 * #define ___DEBUG___
 *
 */

#ifdef ___DEBUG___
unsigned long malloc_addr[1000];
unsigned long msizes[1000];
long addrescount = 0;
/* ___DEBUG___ */
#endif


int curses_open = 0;

int shell_cursor = 1;

void
xfree(void *ptr)
{
#ifdef ___DEBUG___
	int i, j;
	int flag = 0;
	unsigned long msize = 0;
	for (i = 0; i < addrescount; i++)
		msize += msizes[i];
	fprintf(stderr, "Size: %lu, count: %ld, freeing %lu\n", msize, addrescount,(unsigned long) ptr);
	for (i = 0; i < addrescount; i++)
		if (malloc_addr[i] ==(unsigned long) ptr)
		{
			flag = 1;
			for (j = i + 1; j < addrescount; j++)
			{
				malloc_addr[j - 1] = malloc_addr[j];
				msizes[j - 1] = msizes[j];
			}
			addrescount--;
			break;
		}
	if (flag == 0)
	{
		fprintf(stderr, "ERROR!!!\n");
		getchar();
	}
/* ___DEBUG___ */
#endif
	free(ptr);
}

void *
xmalloc(size_t size)
{
	register void *value = malloc(size);
#ifdef ___DEBUG___
	unsigned long msize = 0;
	int i;
/* ___DEBUG___ */
#endif
	if (value == 0)
	{
		closeprogram();
		printf(_("Virtual memory exhausted\n"));
		exit(1);
	}
#ifdef ___DEBUG___
	for (i = 0; i < addrescount; i++)
		msize += msizes[i];
	fprintf(stderr, "Size %lu, count: %ld, allocated %lu\n", msize, addrescount,(unsigned long) value);
	malloc_addr[addrescount] =(unsigned long) value;
	msizes[addrescount] = size;
	if (addrescount < 1000)
		addrescount++;
	else
	{
		fprintf(stderr, "trace buffer exhausted\n");
	}
/* ___DEBUG___ */
#endif
	memset(value, 0, size);
	return value;
}

void *
xrealloc(void *ptr, size_t size)
{
#ifdef ___DEBUG___
	int i, j, flag = 0;
	register void *value;
	unsigned long msize = 0;
	for (i = 0; i < addrescount; i++)
		msize += msizes[i];
	fprintf(stderr, "Size: %lu, count: %ld, reallocating %lu to ", msize, addrescount,(unsigned long) ptr);
	for (i = 0; i < addrescount; i++)
		if (malloc_addr[i] ==(unsigned long) ptr)
		{
			flag = 1;
			for (j = i + 1; j < addrescount; j++)
			{
				malloc_addr[j - 1] = malloc_addr[j];
				msizes[j - 1] = msizes[j];
			}
			addrescount--;
			break;
		}
	if (flag == 0)
	{
		fprintf(stderr, "ERROR!!!\n");
		getchar();
	}
	value = realloc(ptr, size);
#else
	register void *value = realloc(ptr, size + 1024);
/* ___DEBUG___ */
#endif
	if (value == 0)
	{
		closeprogram();
		printf(_("Virtual memory exhausted\n"));
		exit(1);
	}
#ifdef ___DEBUG___
	fprintf(stderr, "%lu, with size %lu\n",(unsigned long) value,(unsigned long) size);
	malloc_addr[addrescount] =(unsigned long) value;
	msizes[addrescount] = size;
	if (addrescount < 1000)
		addrescount++;
	else
	{
		fprintf(stderr, "trace buffer exhausted\n");
	}
/* ___DEBUG___ */
#endif
	return value;
}

void
initlocale()
{
#ifdef ___DEBUG___
	int i;
	for (i = 0; i < 1000; i++)
		malloc_addr[i] = 0;
/* ___DEBUG___ */
#endif
	sbrk(100000);
	setlocale(LC_ALL, "");
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

/*
 * Check filename for dangerous characters and bail out if
 * we find any.
 */
void
checkfilename(const string filename)
{
	if ( (filename.find('<') != string::npos) ||
	     (filename.find('>') != string::npos) ||
	     (filename.find('|') != string::npos) ||
	     (filename.find('(') != string::npos) ||
	     (filename.find(')') != string::npos) ||
	     (filename.find('!') != string::npos) ||
	     (filename.find('`') != string::npos) ||
	     (filename.find('&') != string::npos) ||
	     (filename.find(';') != string::npos)
     ) {
		printf(_("Illegal characters in filename!\n*** %s\n"), filename.c_str());
		exit(1);
	}
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

char *
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

	return buf;
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

/*
 * Compares two strings, ignoring whitespaces(tabs, spaces)
 */
int
compare_tag_table_string(const char *base, const char *compared)
{
	int i = 0;
	int j = 0;
	while (base[i] != '\0') {
		if (base[i] != compared[j]) {
			if (isspace(compared[j]) && isspace(base[i])) {
				/* OK--two blanks */
				j++;
				i++;
			} else if (isspace(compared[j])) {
				/* index of `base' stands in place
				 * and waits for compared to skip blanks */
				j++;
			}	else if (isspace(base[i])) {
				/* index of `compared' stands in place
				 * and waits for base to skip blanks */
				i++;
			} else {
				/* This catches all ordinary differences, and compared being shorter */
				return (int) base[i] - (int) compared[j];
			}
		} else {
			i++;
			j++;
		}
	}
	/* handle trailing whitespaces of variable `compared' */
	while (compared[j] != '\0')	{
		if (!isspace(compared[j]))
			return (int) '\0' - (int) compared[j]; /* Negative, as base is shorter */
		j++;
	}
	return 0;
}

bool
compare_tags(TagTable a, TagTable b) {
	/* Should a be sorted before b? */
	int result = compare_tag_table_string(a.nodename.c_str(), b.nodename.c_str());
	if (result < 0)
		return true;
	else
		return false;
}

int
gettagtablepos(string node)
{
  TagTable dummy;
	dummy.nodename = node;
	std::pair<typeof(tag_table.begin()), typeof(tag_table.begin())> my_result;
	/* The following does binary search */
	my_result = std::equal_range(tag_table.begin(), tag_table.end(),
	                             dummy, compare_tags);
	if (my_result.first == my_result.second) {
		/* Degenerate range: it's a miss. */
		return -1;
	} else {
		/* It's a hit.  Grab the first one in the range. */
		/* And convert to int (zero-based indexing) for output */
		int result = my_result.first - tag_table.begin();
		return result;
	}
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

/* returns 0 on success, 1 on error */
int
pinfo_re_comp(const char *name)
{
#ifdef ___DONT_USE_REGEXP_SEARCH___
	if (pinfo_re_pattern)
	{
		free(pinfo_re_pattern);
		pinfo_re_pattern = 0;
	}
	pinfo_re_pattern = strdup(name);
	return 0;
#else
	if (pinfo_re_offset == -1)
	{
		pinfo_re_offset = h_regexp_num;
		if (!h_regexp_num)
			h_regexp = (regex_t*)malloc(sizeof(regex_t));
		else
			h_regexp = (regex_t*)realloc(h_regexp, sizeof(regex_t) *(h_regexp_num + 1));
	}
	else
	{
		regfree(&h_regexp[pinfo_re_offset]);
	}
	return regcomp(&h_regexp[pinfo_re_offset], name, REG_ICASE);
#endif
}

int
pinfo_re_exec(const char *name)
{
#ifdef ___DONT_USE_REGEXP_SEARCH___
	char *found;
	if (pinfo_re_pattern)
	{
		found = strstr(name, pinfo_re_pattern);
		if (found != NULL)
			return 1;
		else
			return 0;
	}
#else
	regmatch_t pmatch[1];
	return !regexec(&h_regexp[pinfo_re_offset], name, 1, pmatch, 0);
#endif
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
copy_stripped_from_regexp(char *src, char *dest)
{
	char *forbidden = "*.\\()[]\n";
	while (strchr(forbidden, *src) == NULL)
	{
		if (*src == 0)
			break;
		*dest = *src;
		src++;
		dest++;
	}
	*dest = 0;
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

/*
 * Create a vector of strings.  If the strings are concatenated together
 * with separator in between them, the original string will be recovered.
 */
vector<string>
string_explode(string to_explode, string::value_type separator) {
	vector<string> result;
	
	string::size_type old_idx = 0;
	string::size_type new_idx = to_explode.find(separator, old_idx);
	while (new_idx != string::npos) {
		result.push_back(to_explode.substr(old_idx, new_idx - old_idx));
		old_idx = new_idx + 1;
		new_idx = to_explode.find(separator, old_idx);
	}
	/* Get the last one */
	result.push_back(to_explode.substr(old_idx));

	return result;
}

string
string_toupper(string str)
{
	for (string::size_type i = 0; i < str.length(); i++)
		if (islower(str[i]))
			str[i] = toupper(str[i]);
	return str;
}

