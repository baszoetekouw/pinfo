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

#include <ctype.h>

#ifdef HAVE_DECL_USE_DEFAULT_COLORS
# define COLOR_DEFAULT (-1)	/* ncurses extension to use default color, see default_colors(3NCURSES) */
#endif

regex_t *h_regexp = 0;	/* regexps to highlight */
int h_regexp_num = 0;	/* number of those regexps */

struct keybindings keys =
{
	's',		'S',		/* regexp search */
	'/',		'/',		/* regexp search, this page */
	'g',		'G',		/* goto node */
	'p',		'P',		/* previous node */
	'n',		'N',		/* next node */
	'u',		'U',		/* up node */
	KEY_UP,		'k',		/* up one line */
#ifdef KEY_END
	KEY_END
#else
	'E'
#endif
		,'e',			/* end */
	KEY_NPAGE,	' ',		/* down one page */
	KEY_HOME,	'H',		/* home */
	KEY_PPAGE,	'-',		/* up one page */
	KEY_DOWN,	'j',		/* down one line */
	't',		'T',		/* top */
	KEY_LEFT,	'h',		/* back */
	KEY_RIGHT,	'\n',		/* follow link */
	'Q',		'q',		/* quit */
	12,			'~',		/* refresh screen -- 12 is C-L */
	'!',		'!',		/* shell feed */
	'D',		'd',		/* goto dir page */
	0,			' ',		/* pgdn_auto */
	0,			'-',		/* pgup_auto */
	'f',		0,			/* search again */
	'l',		0,			/* go to line */
	KEY_IC,		0,			/* two lines up */
	KEY_DC,		1,			/* two lines down */
	']',		0, 			/* print */
	'4',		0,			/* scroll left */
	'6',		0			/* scroll right */
};

#ifdef HAVE_CURSES_COLOR
struct colours cols =
{
	COLOR_WHITE,	COLOR_BLACK,	NO_BOLD,	NO_BLINK,	/* normal */
	COLOR_GREEN,	COLOR_WHITE,	BOLD,		NO_BLINK,	/* selected menu */
	COLOR_GREEN,	COLOR_BLACK,	BOLD,		NO_BLINK,	/* menu */
	COLOR_BLUE,		COLOR_WHITE,	BOLD,		NO_BLINK,	/* selected note */
	COLOR_BLUE,		COLOR_BLACK,	BOLD,		NO_BLINK,	/* note */
	COLOR_GREEN,	COLOR_BLUE,		BOLD,		NO_BLINK,	/* top line */
	COLOR_GREEN,	COLOR_BLUE,		BOLD,		NO_BLINK,	/* bottom line */
	COLOR_YELLOW,	COLOR_BLACK,	BOLD,		NO_BLINK,	/* manual bold */
	COLOR_WHITE,	COLOR_BLACK,	BOLD,		NO_BLINK,	/* manual italic */
	COLOR_MAGENTA,	COLOR_BLACK,	BOLD,		NO_BLINK,	/* url */
	COLOR_MAGENTA,	COLOR_GREEN,	NO_BOLD,	NO_BLINK,	/* url selected */
	COLOR_WHITE,	COLOR_BLACK,	BOLD,		NO_BLINK,	/* info highlight(quoted text) */
	COLOR_YELLOW,	COLOR_BLACK,	BOLD,		NO_BLINK	/* search highlight */
};
#endif /* HAVE_CURSES_COLOR */

int
parse_config(void)
{
	char config_file_name[256], *home = 0;
	char line[256];
	FILE *f;
	int line_number = 0;
	if (rcfile != NULL)
	{
		f = fopen(rcfile, "r");
		if (f == NULL)
		{
			fprintf(stderr, _("Can't open config file!\n"));
			exit(1);
		}
	}
	else
	{
		if (rcfile == NULL)
		{
			if (getenv("HOME"))
				home = strdup(getenv("HOME"));
			else
				home = 0;
		}
		if (home)
		{
			strcpy(config_file_name, home);
			strcat(config_file_name, "/.pinforc");
			if (!(f = fopen(config_file_name, "r")))
			{
				strcpy(config_file_name, CONFIGDIR);
				if (!(f = fopen(config_file_name, "r")))
				{
					free(home);	/* home is nonzero; see if (home) above */
					return 0;	/* no config file available */
				}
			}
		}
		else
		{
			strcpy(config_file_name, CONFIGDIR);
			if (!(f = fopen(config_file_name, "r")))
			{
				/* free(home);    home is unallocated; see if (home) above */
				return 0;
			}
		}
	}
	while (!feof(f))
	{
		if (!(fgets(line, 255, f)))
		{
			fclose(f);
			if (home)
				free(home);
			return 0;
		}
		if (parse_line(line))
		{
			line_number++;
			fclose(f);
			fprintf(stderr, _("Parse error in config file on line %d\n"), line_number);
			exit(1);
		}
		else
			line_number++;
	}

	fclose(f);
	if (home)
		free(home);
	return 0;
}

int
parse_line(char *line)
{
	char *temp;
	int *fore = NULL;
	int *key = NULL;
	int *back = NULL, *bold = NULL, *blink = NULL, *p = NULL;
	int i;

	if (line[0] == '#')
		return 0;

	if (!(temp = skip_whitespace(strtok(line, "="))))
		return 1;

	temp = str_toupper(temp);

	if (strlen(temp) < 1)
		return 0;

	if (!strncmp(temp, "KEY", 3))
	{
		fore = NULL;
		if (!strncmp(temp + 4, "TOTALSEARCH_1", 13))
			key = &keys.totalsearch_1;
		else if (!strncmp(temp + 4, "PGDN_AUTO_1", 11))
			key = &keys.pgdn_auto_1;
		else if (!strncmp(temp + 4, "PGDN_AUTO_2", 11))
			key = &keys.pgdn_auto_2;
		else if (!strncmp(temp + 4, "PGUP_AUTO_1", 11))
			key = &keys.pgup_auto_1;
		else if (!strncmp(temp + 4, "PGUP_AUTO_2", 11))
			key = &keys.pgup_auto_2;
		else if (!strncmp(temp + 4, "TOTALSEARCH_2", 13))
			key = &keys.totalsearch_2;
		else if (!strncmp(temp + 4, "SEARCH_AGAIN_1", 14))
			key = &keys.search_again_1;
		else if (!strncmp(temp + 4, "SEARCH_AGAIN_2", 14))
			key = &keys.search_again_2;
		else if (!strncmp(temp + 4, "SEARCH_1", 8))
			key = &keys.search_1;
		else if (!strncmp(temp + 4, "SEARCH_2", 8))
			key = &keys.search_2;
		else if (!strncmp(temp + 4, "GOTO_1", 6))
			key = &keys.goto_1;
		else if (!strncmp(temp + 4, "GOTO_2", 6))
			key = &keys.goto_2;
		else if (!strncmp(temp + 4, "PREVNODE_1", 10))
			key = &keys.prevnode_1;
		else if (!strncmp(temp + 4, "PREVNODE_2", 10))
			key = &keys.prevnode_2;
		else if (!strncmp(temp + 4, "NEXTNODE_1", 10))
			key = &keys.nextnode_1;
		else if (!strncmp(temp + 4, "NEXTNODE_2", 10))
			key = &keys.nextnode_2;
		else if (!strncmp(temp + 4, "UPNODE_1", 8))
			key = &keys.upnode_1;
		else if (!strncmp(temp + 4, "UPNODE_2", 8))
			key = &keys.upnode_2;
		else if (!strncmp(temp + 4, "UP_1", 4))
			key = &keys.up_1;
		else if (!strncmp(temp + 4, "UP_2", 4))
			key = &keys.up_2;
		else if (!strncmp(temp + 4, "TWOUP_1", 7))
			key = &keys.twoup_1;
		else if (!strncmp(temp + 4, "TWOUP_2", 7))
			key = &keys.twoup_2;
		else if (!strncmp(temp + 4, "END_1", 5))
			key = &keys.end_1;
		else if (!strncmp(temp + 4, "END_2", 5))
			key = &keys.end_2;
		else if (!strncmp(temp + 4, "PGDN_1", 6))
			key = &keys.pgdn_1;
		else if (!strncmp(temp + 4, "PGDN_2", 6))
			key = &keys.pgdn_2;
		else if (!strncmp(temp + 4, "HOME_1", 6))
			key = &keys.home_1;
		else if (!strncmp(temp + 4, "HOME_2", 6))
			key = &keys.home_2;
		else if (!strncmp(temp + 4, "PGUP_1", 6))
			key = &keys.pgup_1;
		else if (!strncmp(temp + 4, "PGUP_2", 6))
			key = &keys.pgup_2;
		else if (!strncmp(temp + 4, "DOWN_1", 6))
			key = &keys.down_1;
		else if (!strncmp(temp + 4, "DOWN_2", 6))
			key = &keys.down_2;
		else if (!strncmp(temp + 4, "TWODOWN_1", 9))
			key = &keys.twodown_1;
		else if (!strncmp(temp + 4, "TWODOWN_2", 9))
			key = &keys.twodown_2;
		else if (!strncmp(temp + 4, "TOP_1", 5))
			key = &keys.top_1;
		else if (!strncmp(temp + 4, "TOP_2", 5))
			key = &keys.top_2;
		else if (!strncmp(temp + 4, "BACK_1", 6))
			key = &keys.back_1;
		else if (!strncmp(temp + 4, "BACK_2", 6))
			key = &keys.back_2;
		else if (!strncmp(temp + 4, "FOLLOWLINK_1", 12))
			key = &keys.followlink_1;
		else if (!strncmp(temp + 4, "FOLLOWLINK_2", 12))
			key = &keys.followlink_2;
		else if (!strncmp(temp + 4, "REFRESH_1", 9))
			key = &keys.refresh_1;
		else if (!strncmp(temp + 4, "REFRESH_2", 9))
			key = &keys.refresh_2;
		else if (!strncmp(temp + 4, "SHELLFEED_1", 11))
			key = &keys.shellfeed_1;
		else if (!strncmp(temp + 4, "SHELLFEED_2", 11))
			key = &keys.shellfeed_2;
		else if (!strncmp(temp + 4, "QUIT_1", 6))
			key = &keys.quit_1;
		else if (!strncmp(temp + 4, "QUIT_2", 6))
			key = &keys.quit_2;
		else if (!strncmp(temp + 4, "DIRPAGE_1", 9))
			key = &keys.dirpage_1;
		else if (!strncmp(temp + 4, "DIRPAGE_2", 9))
			key = &keys.dirpage_2;
		else if (!strncmp(temp + 4, "GOLINE_1", 8))
			key = &keys.goline_1;
		else if (!strncmp(temp + 4, "GOLINE_2", 8))
			key = &keys.goline_2;
		else if (!strncmp(temp + 4, "PRINT_1", 7))
			key = &keys.print_1;
		else if (!strncmp(temp + 4, "PRINT_2", 7))
			key = &keys.print_2;
		else if (!strncmp(temp + 4, "LEFT_1", 6))
			key = &keys.left_1;
		else if (!strncmp(temp + 4, "LEFT_2", 6))
			key = &keys.left_2;
		else if (!strncmp(temp + 4, "RIGHT_1", 7))
			key = &keys.right_1;
		else if (!strncmp(temp + 4, "RIGHT_2", 7))
			key = &keys.right_2;
		else
			return 1;
	}
#ifdef HAVE_CURSES_COLOR
	else if (!strncmp(temp, "COL", 3))
	{
		key = NULL;
		if (!strncmp(temp + 4, "NORMAL", 6))
		{
			fore = &cols.normal_fore;
			back = &cols.normal_back;
			bold = &cols.normal_bold;
			blink = &cols.normal_blink;
		}
		else if (!strncmp(temp + 4, "MENUSELECTED", 12))
		{
			fore = &cols.menuselected_fore;
			back = &cols.menuselected_back;
			bold = &cols.menuselected_bold;
			blink = &cols.menuselected_blink;
		}
		else if (!strncmp(temp + 4, "MENU", 4))
		{
			fore = &cols.menu_fore;
			back = &cols.menu_back;
			bold = &cols.menu_bold;
			blink = &cols.menu_blink;
		}
		else if (!strncmp(temp + 4, "NOTESELECTED", 12))
		{
			fore = &cols.noteselected_fore;
			back = &cols.noteselected_back;
			bold = &cols.noteselected_bold;
			blink = &cols.noteselected_blink;
		}
		else if (!strncmp(temp + 4, "NOTE", 4))
		{
			fore = &cols.note_fore;
			back = &cols.note_back;
			bold = &cols.note_bold;
			blink = &cols.note_blink;
		}
		else if (!strncmp(temp + 4, "TOPLINE", 7))
		{
			fore = &cols.topline_fore;
			back = &cols.topline_back;
			bold = &cols.topline_bold;
			blink = &cols.topline_blink;
		}
		else if (!strncmp(temp + 4, "BOTTOMLINE", 10))
		{
			fore = &cols.bottomline_fore;
			back = &cols.bottomline_back;
			bold = &cols.bottomline_bold;
			blink = &cols.bottomline_blink;
		}
		else if (!strncmp(temp + 4, "MANUALBOLD", 10))
		{
			fore = &cols.manualbold_fore;
			back = &cols.manualbold_back;
			bold = &cols.manualbold_bold;
			blink = &cols.manualbold_blink;
		}
		else if (!strncmp(temp + 4, "MANUALITALIC", 12))
		{
			fore = &cols.manualitalic_fore;
			back = &cols.manualitalic_back;
			bold = &cols.manualitalic_bold;
			blink = &cols.manualitalic_blink;
		}
		else if (!strncmp(temp + 4, "URLSELECTED", 11))
		{
			fore = &cols.urlselected_fore;
			back = &cols.urlselected_back;
			bold = &cols.urlselected_bold;
			blink = &cols.urlselected_blink;
		}
		else if (!strncmp(temp + 4, "URL", 3))
		{
			fore = &cols.url_fore;
			back = &cols.url_back;
			bold = &cols.url_bold;
			blink = &cols.url_blink;
		}
		else if (!strncmp(temp + 4, "INFOHIGHLIGHT", 13))
		{
			fore = &cols.infohighlight_fore;
			back = &cols.infohighlight_back;
			bold = &cols.infohighlight_bold;
			blink = &cols.infohighlight_blink;
		}
		else if (!strncmp(temp + 4, "SEARCHHIGHLIGHT", 15))
		{
			fore = &cols.searchhighlight_fore;
			back = &cols.searchhighlight_back;
			bold = &cols.searchhighlight_bold;
			blink = &cols.searchhighlight_blink;
		}
		else
			return 1;
	}
#endif /* HAVE_CURSES_COLOR */
	else if (!strncmp(temp, "MANUAL", 6))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				use_manual = 1;
			else if (!strncmp(temp, "FALSE", 5))
				use_manual = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "GRAB-MOUSE", 10))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				grab_mouse = 1;
			else if (!strncmp(temp, "FALSE", 5))
				grab_mouse = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "RAW-FILENAME", 12))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				use_raw_filename = 1;
			else if (!strncmp(temp, "FALSE", 5))
				use_raw_filename = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "APROPOS", 7))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				use_apropos = 1;
			else if (!strncmp(temp, "FALSE", 5))
				use_apropos = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "VERBOSE", 7))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				verbose = 1;
			else if (!strncmp(temp, "FALSE", 5))
				verbose = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "QUIT-CONFIRMATION", 17))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				ConfirmQuit = 1;
			else if (!strncmp(temp, "FALSE", 5))
				ConfirmQuit = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "QUIT-CONFIRM-DEFAULT", 20))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "YES", 3))
				QuitConfirmDefault = 1;
			else if (!strncmp(temp, "NO", 2))
				QuitConfirmDefault = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "CUT-MAN-HEADERS", 15))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				CutManHeaders = 1;
			else if (!strncmp(temp, "FALSE", 5))
				CutManHeaders = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "CLEAR-SCREEN-AT-EXIT", 20))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				ClearScreenAtExit = 1;
			else if (!strncmp(temp, "FALSE", 5))
				ClearScreenAtExit = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "CALL-READLINE-HISTORY", 21))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				CallReadlineHistory = 1;
			else if (!strncmp(temp, "FALSE", 5))
				CallReadlineHistory = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "CUT-EMPTY-MAN-LINES", 19))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				CutEmptyManLines = 1;
			else if (!strncmp(temp, "FALSE", 5))
				CutEmptyManLines = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "DONT-HANDLE-WITHOUT-TAG-TABLE", 28))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				DontHandleWithoutTagTable = 1;
			else if (!strncmp(temp, "FALSE", 5))
				DontHandleWithoutTagTable = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "LONG-MANUAL-LINKS", 17))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				LongManualLinks = 1;
			else if (!strncmp(temp, "FALSE", 5))
				LongManualLinks = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "HTTPVIEWER", 10))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			httpviewer = strdup(temp);
			remove_quotes(httpviewer);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "FTPVIEWER", 9))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			ftpviewer = strdup(temp);
			remove_quotes(ftpviewer);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "MAILEDITOR", 10))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			maileditor = strdup(temp);
			remove_quotes(maileditor);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "PRINTUTILITY", 12))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			printutility = strdup(temp);
			remove_quotes(printutility);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "MAN-OPTIONS", 11))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			ManOptions = strdup(temp);
			remove_quotes(ManOptions);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "STDERR-REDIRECTION", 18))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			StderrRedirection = strdup(temp);
			remove_quotes(StderrRedirection);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "FILTER-0XB7", 11))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				FilterB7 = 1;
			else if (!strncmp(temp, "FALSE", 5))
				FilterB7 = 0;
			else
				return 1;
		}
	}
	else if (!strncmp(temp, "MANLINKS", 8))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			manlinks = strdup(temp);
			remove_quotes(manlinks);
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "INFOPATH", 8))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			configuredinfopath = strdup(temp);
			remove_quotes(configuredinfopath);
		}
		else
			return 1;
	}
#ifndef ___DONT_USE_REGEXP_SEARCH___
	else if (!strncmp(temp, "HIGHLIGHTREGEXP", 15))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			char *tmp = strdup(temp);
			remove_quotes(tmp);
			if (!h_regexp_num)
				h_regexp = malloc(sizeof(regex_t));
			else
				h_regexp = realloc(h_regexp, sizeof(regex_t) *(h_regexp_num + 1));
			regcomp(&h_regexp[h_regexp_num], tmp, 0);
			free(tmp);
			h_regexp_num++;
		}
		else
			return 1;
	}
#endif
	else if (!strncmp(temp, "SAFE-USER", 9))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			char *tmp = strdup(temp);
			remove_quotes(tmp);
			safe_user = tmp;
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "SAFE-GROUP", 10))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			char *tmp = strdup(temp);
			remove_quotes(tmp);
			safe_group = tmp;
		}
		else
			return 1;
	}
	else if (!strncmp(temp, "QUOTE-IGNORED-MACROS", 20))
	{
		temp = strtok(NULL, "=");
		if (temp)
		{
			if (!(temp = str_toupper(skip_whitespace(temp))))
				return 1;
			if (!strncmp(temp, "TRUE", 4))
				quote_ignored = 1;
			else if (!strncmp(temp, "FALSE", 5))
				quote_ignored = 0;
			else
				return 1;
		}
	}

	else if (!strncmp(temp, "IGNORE-MACROS", 8))
	{
		temp = strtok(NULL, "\n");
		if (temp)
		{
			ignoredmacros = strdup(temp);
			remove_quotes(ignoredmacros);
			if (ignoredmacros[0] == '\t' || ignoredmacros[0] == ' '
					|| !strncasecmp(ignoredmacros, "FALSE", 5))
				ignoredmacros[0] = '\0';
		}
		else
			return 1;
	}
	else
		return 1;
#ifdef HAVE_CURSES_COLOR
	if (fore)
	{
		for (i = 0; i < 4; i++)
		{
			if (i == 0)
				p = fore;
			else if (i == 1)
				p = back;
			else if (i == 2)
				p = bold;
			else
				p = blink;

			if (!(temp = skip_whitespace(strtok(NULL, ","))))
				return 1;

			temp = str_toupper(temp);

			if (!(strncmp(temp, "COLOR_BLACK", 11)))
				*p = COLOR_BLACK;
			else if (!(strncmp(temp, "COLOR_RED", 9)))
				*p = COLOR_RED;
			else if (!(strncmp(temp, "COLOR_GREEN", 11)))
				*p = COLOR_GREEN;
			else if (!(strncmp(temp, "COLOR_BLUE", 10)))
				*p = COLOR_BLUE;
			else if (!(strncmp(temp, "COLOR_WHITE", 11)))
				*p = COLOR_WHITE;
			else if (!(strncmp(temp, "COLOR_YELLOW", 12)))
				*p = COLOR_YELLOW;
			else if (!(strncmp(temp, "COLOR_CYAN", 10)))
				*p = COLOR_CYAN;
			else if (!(strncmp(temp, "COLOR_MAGENTA", 13)))
				*p = COLOR_MAGENTA;
			else if (!(strncmp(temp, "COLOR_DEFAULT", 13)))
			{
#ifdef HAVE_DECL_USE_DEFAULT_COLORS
				*p = COLOR_DEFAULT;
#else
				fprintf(stderr, "COLOR_DEFAULT is not supported on this system\n");
				return 1;
#endif
			}
			else if (!(strncmp(temp, "BOLD", 4)))
				*p = BOLD;
			else if (!(strncmp(temp, "NO_BOLD", 7)))
				*p = 0;
			else if (!(strncmp(temp, "BLINK", 4)))
				*p = BOLD;
			else if (!(strncmp(temp, "NO_BLINK", 7)))
				*p = 0;
			else
				return 1;
		}
	}
	else
#endif /* HAVE_CURSES_COLOR */
		if (key)
		{
			if (!(temp = skip_whitespace(strtok(NULL, "="))))
				return 0;
			if (!(strncmp(temp, "KEY_", 4)) ||
					!(strncmp(temp, "key_", 4)))
			{
				str_toupper(temp);
				/* what other keys should be interesting?  all in curs_getch? */
				if (!(strncmp(temp + 4, "BREAK", 5)))
					*key = KEY_BREAK;
				else if (!(strncmp(temp + 4, "DOWN", 4)))
					*key = KEY_DOWN;
				else if (!(strncmp(temp + 4, "UP", 2)))
					*key = KEY_UP;
				else if (!(strncmp(temp + 4, "LEFT", 4)))
					*key = KEY_LEFT;
				else if (!(strncmp(temp + 4, "RIGHT", 5)))
					*key = KEY_RIGHT;
				else if (!(strncmp(temp + 4, "IC", 2)))
					*key = KEY_IC;
				else if (!(strncmp(temp + 4, "DC", 2)))
					*key = KEY_DC;
				else if (!(strncmp(temp + 4, "HOME", 4)))
					*key = KEY_HOME;
				else if (!(strncmp(temp + 4, "BACKSPACE", 9)))
					*key = KEY_BACKSPACE;
				else if (!(strncmp(temp + 4, "NPAGE", 5)))
					*key = KEY_NPAGE;
				else if (!(strncmp(temp + 4, "PPAGE", 5)))
					*key = KEY_PPAGE;
				else if (!(strncmp(temp + 4, "F(1)", 4)))
					*key = KEY_F(1);
				else if (!(strncmp(temp + 4, "F(2)", 4)))
					*key = KEY_F(2);
				else if (!(strncmp(temp + 4, "F(3)", 4)))
					*key = KEY_F(3);
				else if (!(strncmp(temp + 4, "F(4)", 4)))
					*key = KEY_F(4);
				else if (!(strncmp(temp + 4, "F(5)", 4)))
					*key = KEY_F(5);
				else if (!(strncmp(temp + 4, "F(6)", 4)))
					*key = KEY_F(6);
				else if (!(strncmp(temp + 4, "F(7)", 4)))
					*key = KEY_F(7);
				else if (!(strncmp(temp + 4, "F(8)", 4)))
					*key = KEY_F(8);
				else if (!(strncmp(temp + 4, "F(9)", 4)))
					*key = KEY_F(9);
				else if (!(strncmp(temp + 4, "F(10)", 5)))
					*key = KEY_F(10);
				else if (!(strncmp(temp + 4, "F(11)", 5)))
					*key = KEY_F(11);
				else if (!(strncmp(temp + 4, "F(12)", 5)))
					*key = KEY_F(12);
				else if (!(strncmp(temp + 4, "END", 3)))
					*key = KEY_END;
				else if (!(strncmp(temp + 4, "CTRL", 4)))
				{
					if (!(temp = skip_whitespace(temp + 8)))
						return 1;
					if (temp[0] == '(')
					{
						if (temp[1] == '\'')
							*key = KEY_CTRL(temp[2]);
						else if (isdigit(temp[1]))
						{
							char *tail = temp +(strlen(temp));
							*key = KEY_CTRL((int) strtol(temp + 1, &tail, 10));
						}
						else
							return 1;
					}
					else
						return 1;
				}
				else if (!(strncmp(temp + 4, "ALT", 3)))
				{
					if (!(temp = skip_whitespace(temp + 7)))
						return 1;
					if (temp[0] == '(')
					{
						if (temp[1] == '\'')
							*key = KEY_ALT(tolower(temp[2]));
						else if (isdigit(temp[1]))
						{
							char *tail = temp +(strlen(temp));
							*key = KEY_ALT((int) strtol(temp + 1, &tail, 10));
						}
						else
							return 1;
					}
					else
						return 1;
				}
				else
					return 1;
			}
			else if (!(strncmp(temp, "\'", 1)))
			{
				if (!(strncmp(temp + 1, "\\", 1)))
				{
					if (temp[2] == 'n')
						*key = '\n';
					else if (temp[2] == '\\')
						*key = '\\';
					else if (temp[2] == 't')
						*key = '\t';
					else if (temp[2] == '\'')
						*key = '\'';
					else
						*key = temp[2];
				}
				else
				{
					*key = temp[1];
				}
			}
			else if (isdigit(temp[0]))
			{
				char *tail = temp +(strlen(temp));
				*key =(int) strtol(temp, &tail, 10);
			}
		}

	return 0;
}

char *
str_toupper(char *str)
{
	unsigned int i;
	for (i = 0; i < strlen(str); ++i)
		if (islower(str[i]))
			str[i] = toupper(str[i]);

	return str;
}

char *
skip_whitespace(char *str)
{
	int i = 0;

	if (!str)
		return NULL;

	while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n')
		i++;

	return str + i;
}

char *
remove_quotes(char *str)
{
	size_t i = 0;

	for (i = 0; i < strlen(str); i++)
		if (str[i] == '\"')
			str[i] = ' ';

	return str;
}
