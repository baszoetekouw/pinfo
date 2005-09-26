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
#include <algorithm> // for std::sort and std::find
#include <cctype>

#include <sys/stat.h>

#include "colors.h"
#include "curse_utils.h"
#include "initializelinks.h"
#include "keyboard.h"
#include "manual.h"
#include "regexp_search.h"
#include "tmpfiles.h"
#include "utils.h"

#define HTTPSECTION 100
#define FTPSECTION 101
#define MAILSECTION 102

/* line by line stored manual */
vector<string> manual;

int selected = -1;		/* number of selected link(offset in 'manuallinks',
						   bellow) */
int manualpos = 0;		/* number of the first line, which is painted on
						   screen */

int manualcol = 0;		/* the first displayed column of manpage--
						   for moving the screen left/right */

/*
 * type for the `lastread' history entries, when viewing
 * man pages.
 */
typedef struct manhistory
{
	/* name of a manual */
	string name;
	/* section */
	string sect;
	/* what was last selected on this page */
	int selected;
	/* what was the last manualpos */
	int pos;
} manhistory;

/* manual lastread history */
vector<manhistory> manualhistory;

/* this structure describes a hyperlink in manual viewer */
typedef struct manuallink
{			/* struct for hypertext references */
	int line;		/* line of the manpage, where the reference is */
	/* column of that line */
	int col;
	/* name of the reference */
	string name;
	/* section of the reference */
	string section;
	int section_mark;
	/* determine whether there is a hyphen above */
	int carry;
}
manuallink;

/* a set of manual references of man page */
vector<manuallink> manuallinks;

/* Debugging routine */
static void
dumplink(manuallink a) {
	printf("LINK x%sx (x%sx %d) at %d %d (%d)\n\r", (a.name).c_str(),
		(a.section).c_str(), a.section_mark,
		a.line, a.col, a.carry);
}

/* this flag indicates whether the env variable
 * $MANWIDTH was changed by pinfo */
static bool manwidthChanged = false;

/*** START OF FUNCTIONS ***/

/* check if a char is a hyphen character */
static int
ishyphen(unsigned char ch)
{
	if ((ch == '-') ||(ch == SOFT_HYPHEN))
		return 1;
	return 0;
}

/* Set MANWIDTH environment variable as needed */
static void
check_manwidth(void) {
	if ((!getenv("MANWIDTH")) ||(manwidthChanged))
	{
		/* This should be rewritten dynamically,
		 * but putenv sucks really badly, and setenv isn't portable,
		 * so for now we leave it.  FIXME.
		 */
		/* x is an int.  int is no larger than 64 bits (we hope).
		 * The largest value in a signed 64-bit integer is 2^63 - 1
		 * which can be represented in 19 digits.  Therefore 29
		 * characters is enough to include the whole string (with
		 * terminator), and sprintf is safe.  The extra character
		 * is in case a negative value snuck into maxx.  Whee.
		 */
		static char env_entry[30];
		sprintf(env_entry, "MANWIDTH=%d", maxx);
		putenv(env_entry);
		manwidthChanged = true;
	}
}

/* Cleanse a line of backspaces; overwrites argument. */
/* Should probably be rewritten to not overwrite argument. */
static void
strip_manual(string& buf)
{
	/* in general, tmp buffer will hold a line with highlight marks stripped */
	/* Overwrite as we go.  Length will change as we go, too. */
	for (string::size_type i = 0; i < buf.length(); i++)
	{
		/* strip from the line "'_',0x8" -- underline marks */
		if ((buf[i] == '_') && (buf[i + 1] == 8))
			buf.erase(i, 2);
		/* and 0x8 -- overstrike marks */
		else if ((buf[i + 1] == 8) &&(buf[i + 2] == buf[i]))
			buf.erase(i, 2);
		/* else we don't do anything */
	}
}

/*
 * checks if a construction, which looks like hyperlink, belongs to the allowed
 * manual sections.
 */
static bool
is_in_manlinks(const vector<string>& manlinks, const string& to_find)
{
	/* Normalize case */
	string to_find_uppercase = string_toupper(to_find);
	typeof(manlinks.begin()) result_iter;
	result_iter = std::find(manlinks.begin(), manlinks.end(), to_find_uppercase);
	return (result_iter != manlinks.end()); /* True if found */
}

/* scan for some hyperlink, available on current screen */
static void
rescan_selected()
{
	for (typeof(manuallinks.size()) i = 0; i < manuallinks.size(); i++)
	{
		if ((manuallinks[i].line >= manualpos) &&
				(manuallinks[i].line < manualpos +(maxy - 1)))
		{
			selected = i;
			break;
		}
	}
}


/* initialize history variables for manual pages.  */
static void
set_initial_history(string name)
{
	/* filter trailing spaces */
	string::size_type len;
	for (len = name.length(); (len > 0) && isspace(name[len - 1]); len--);
	name.resize(len);

	/* find the beginning of the last token */
	string::size_type i;
	for (i = len - 1; (i > 0) && !isspace(name[i]); i--);

	/* if we've found space, then we move to the first nonspace character */
	if ( (i > 0) || (i == 0 && isspace(name[i])) ) {
		i++;
	}

	manhistory my_hist;
	/* filename->name */
	my_hist.name = name.substr(i);
	/* section unknown */
	my_hist.sect = "";
	/* selected unknown */
	my_hist.selected = -1;
	/* pos=0 */
	my_hist.pos = 0;
	manualhistory.push_back(my_hist);
}

/* construct man name; take care about carry */
static void
construct_manualname(string& buf, int which)
{
	if (!manuallinks[which].carry) {
		buf = manuallinks[which].name;
		/* workaround for names starting with '(' */
		if (buf[0] == '(')
			buf.erase(0);
	} else if (manuallinks[which].section_mark < HTTPSECTION) {
		/* normal manual reference */
		buf = manual[manuallinks[which].line - 1];
		strip_manual(buf);

		string::size_type idx;
		/* Delete last two characters (e.g. .1) FIXME */
		buf.resize(buf.length() - 2);
		/* Find tail with decent characters */
		idx = buf.length() - 1;
		while (    (    (isalpha(buf[idx]))
		             || (buf[idx] == '.')
		             || (buf[idx] == '_')
		           )
					  && (idx > 0)
		      ) {
			idx--;
		}
		/* workaround for man pages with leading '(' see svgalib man pages */
		if (buf[idx] == '(')
			idx++;
		/* Delete characters before tail */
		buf.erase(0, idx);
	
		buf += manuallinks[which].name;
	} else {
		/* URL reference */
		/* Start with manuallinks[which].name, with its
		 * trailing hyphen removed
		 */
		buf = manuallinks[which].name;
		buf.resize(buf.length() - 1);

		string tmpstr;
		tmpstr = manual[manuallinks[which].line + 1];
		strip_manual(tmpstr);

		/* skip whitespace */
		string::size_type idx = 0;
		while (isspace(tmpstr[idx]))
			idx++;
		tmpstr.erase(0, idx);

		/* Cut off anything past the URL end */
		string::size_type urlend_idx = findurlend(tmpstr);
		tmpstr.resize(urlend_idx);

		buf += tmpstr;
	}
}

static bool
compare_manuallink(manuallink a, manuallink b)
{
	/* Should a sort before b? */
  return (a.col < b.col);
}

static void
sort_manuallinks_from_current_line(
	typeof(manuallinks.begin()) startlink,
	typeof(manuallinks.begin()) endlink)
{
	std::sort(startlink, endlink, compare_manuallink);
}

/* initializes hyperlinks in manual */
static void
man_initializelinks(const string& line, int line_num, int carry)
{
	typeof(manuallinks.size()) initialManualLinks = manuallinks.size();
	/******************************************************************************
	 * handle url refrences                                                       *
	 *****************************************************************************/

	string::size_type urlstart = 0;
	string::size_type urlend = 0;
	while ((urlstart = line.find("http://", urlend)) != string::npos)
	{
		urlend = findurlend(line, urlstart); /* always successful */
		manuallink my_link;
		my_link.line = line_num;
		my_link.col = urlstart;
		my_link.section = "HTTPSECTION";
		my_link.section_mark = HTTPSECTION;
		my_link.name = line.substr(urlstart, urlend - urlstart - 1);
		if (ishyphen(my_link.name[urlend - urlstart - 1]))
			my_link.carry = 1;
		else
			my_link.carry = 0;
		manuallinks.push_back(my_link);
	}

	urlstart = 0;
	urlend = 0;
	while ((urlstart = line.find("ftp://", urlend)) != string::npos)
	{
		urlend = findurlend(line, urlstart); /* always successful */
		manuallink my_link;
		my_link.line = line_num;
		my_link.col = urlstart;
		my_link.section = "FTPSECTION";
		my_link.section_mark = FTPSECTION;
		my_link.name = line.substr(urlstart, urlend - urlstart - 1);
		if (ishyphen(my_link.name[urlend - urlstart - 1]))
			my_link.carry = 1;
		else
			my_link.carry = 0;
		manuallinks.push_back(my_link);
	}

	urlstart = 0;
	urlend = 0;
	while ((urlstart = findemailstart(line, urlend)) != string::npos)
	{
		urlend = findurlend(line, urlstart); /* always successful */
		manuallink my_link;
		my_link.line = line_num;
		my_link.col = urlstart;
		my_link.section = "MAILSECTION";
		my_link.section_mark = MAILSECTION;
		my_link.name = line.substr(urlstart, urlend - urlstart - 1);
		if (ishyphen(my_link.name[urlend - urlstart - 1]))
			my_link.carry = 1;
		else
			my_link.carry = 0;

		/* there should be a dot in e-mail domain */
		if (my_link.name.find('.') != string::npos) {
			manuallinks.push_back(my_link);
		}
	}
	/******************************************************************************
	 * handle normal manual refrences -- reference(section)                       *
	 ******************************************************************************/

	/* we look for '(', since manual link */
	/* has form of  'blah(x)' */
	string::size_type left_bracket_index;
	left_bracket_index = line.find('(');
	while (    (left_bracket_index != string::npos)
	        && (left_bracket_index < line.length())
	      ) {
		/* look for the closing bracket */
		string::size_type right_bracket_index;
		right_bracket_index = line.find(')', left_bracket_index + 1);
		if (right_bracket_index != string::npos) {
			string sect_str;
			sect_str = line.substr(left_bracket_index + 1,
			                      right_bracket_index - (left_bracket_index + 1));

			if (    (sect_str.find('(') == string::npos)
			     && is_in_manlinks(manlinks, sect_str)
			   ) {
				int breakpos;
				int i = left_bracket_index - 1;
				if (i < 0)
					i++;
				for (; i > 0; i--) {
					if (!isspace(line[i]))
						/* ignore spaces between linkname and '(x)' */
						break;
				}

				breakpos = i + 1;
				string prebreak;
				prebreak = line.substr(0, breakpos);
				/*
				 * scan to the first space sign or to 0 -- that means go to
				 * the beginning of the scanned token
				 */
				for (i = prebreak.size() - 1; i > 0; i--) {
					if (isspace(prebreak[i])) {
						i++;
						break;
					}
				}
				if ((i == 0) && isspace(prebreak[i])) {
					i++;
				}

				/* now we have needed string in i..breakpos. */
				string chosen_name = prebreak.substr(i);

				/* a small check */
				if (!(use_apropos && (manualhistory.size() == 1))) {
					/*
					 * In English: if the name of the link is the name of
					 * the current page and the section of the link is the
					 * current section or if we don't know the current
					 * section, then...
					 */
					if (    (!strcasecmp(chosen_name.c_str(),
					                     manualhistory[manualhistory.size() - 1].name.c_str())
					        )
							 && (    (!strcasecmp(sect_str.c_str(),
					                          manualhistory[manualhistory.size() - 1].sect.c_str())
					             )
								    || (manualhistory[manualhistory.size() - 1].sect == "")
								    || (manualhistory[manualhistory.size() - 1].sect == " ")
					        )
					   ) {
						left_bracket_index = line.find('(', left_bracket_index + 1);
						continue;
					}
				}
				manuallink my_link;
				my_link.name = chosen_name;
				my_link.line = line_num;
				my_link.col = i;
				if (LongManualLinks) {
					my_link.section = "";
					for (string::size_type b = left_bracket_index + 1;
							 line[b] != ')'; b++) {
						my_link.section += tolower(line[b]);
					}
				} else {
					/* Short manual links */
					my_link.section = line[left_bracket_index + 1];
				}
				my_link.section_mark = 0;

				/* check whether this is a carry'ed entry(i.e. in the
				 * previous line there was `-' at end, and this is the
				 * first word of this line */
				int b;
				for (b = i - 1; b >= 0; b--) {
					if (b > 0)
						if (!isspace(line[b]))
							break;
				}
				if (b >= 0)
					my_link.carry = 0;
				else
					my_link.carry = carry;
				/* increase the number of entries */
				manuallinks.push_back(my_link);
			}	/* ... if (in man links) */
		} /* ... if right bracket */
		left_bracket_index = line.find('(', left_bracket_index + 1);
	}

	if (manuallinks.size() > initialManualLinks) {
		typeof(manuallinks.begin()) first_new_link
			= manuallinks.end() - (manuallinks.size() - initialManualLinks); 
		sort_manuallinks_from_current_line(first_new_link, manuallinks.end());
	}
}

/* loads manual from given filedescriptor */
static void
loadmanual(FILE * id)
{
	char prevlinechar = 0;
	/* tmp variable, set after reading first nonempty line of input */
	int cutheader = 0;
	int carryflag = 0;
	manualpos = 0;
	selected = -1;
	manual.clear();
	manuallinks.clear();

	/* fixed-size buffer, FIXME */
	char tmpline[1024];

	/* we read until eof */
	while (!feof(id)) {
		memset(tmpline, '\0', 1024);
		/*
		 * it happens sometimes, that the last line is weird
		 * and causes sigsegvs by not entering anything to buffer, what
		 * confuses strlen
		 */
		if (fgets(tmpline, 1024, id) == NULL) {
			strcpy(tmpline, "");
		}
		if (cutheader) {
			if (manual[cutheader] == tmpline) {
				strcpy(tmpline, "\n");
			}
		}
		if (FilterB7) {
			char *filter_pos = index(tmpline, 0xb7);
			if (filter_pos)
				*filter_pos = 'o';
		}
		if ((CutEmptyManLines) &&((tmpline[0]) == '\n') &&
				(prevlinechar == '\n')) {
			;	/* do nothing */
		} else {
			if (CutManHeaders && !cutheader) {
				cutheader = manual.size();
			}
			int manlinelen = strlen(tmpline);

			carryflag = 0;
			if (    (manlinelen >= 2)
				   && (ishyphen(tmpline[manlinelen - 2]))
			   ) {
				carryflag = 1;
			}
			prevlinechar = tmpline[0];

			/* temporary variable for determining hypertextuality of fields */
			string tmpstr;
			tmpstr = tmpline;
			strip_manual(tmpstr);
			int line_num = manual.size();
			/* Above depends on link initializing happening right before push_back. */
			man_initializelinks(tmpstr, line_num, carryflag);

			string tmpline_str = tmpline;
			manual.push_back(tmpline_str);
		}
	}
}

static void
printmanual(const vector<string>& message)
{
	/* printer fd */
	FILE *prnFD;
	int i;

	prnFD = popen(printutility.c_str(), "w");

	/* scan through all lines */
	for (i = 0; i < message.size(); i++)
	{
		fprintf(prnFD, "\r%s", message[i].c_str());
	}
	pclose(prnFD);
}

/* add hyperobject highlights */
static void
add_highlights()
{
	/* scan through the visible objects */
	for (typeof(manuallinks.size()) i = 0; i < manuallinks.size(); i++)
	{
		/* if the object is on the current screen */
		if ((manuallinks[i].line >= manualpos) &&
				(manuallinks[i].line < manualpos +(lines_visible)))
		{
			/* if it's a simple man link */
			if (manuallinks[i].section_mark < HTTPSECTION)
			{
				if (i == selected)
					attrset(noteselected);
				else
					attrset(note);

				/* if it's a link split into two lines */
				if (manuallinks[i].carry == 1) {
					int x, y;
					getyx(stdscr, y, x);

					int ltline = manuallinks[i].line - 1;
					string tmp_string = manual[ltline];

					strip_manual(tmp_string);

					string::size_type link_begin = tmp_string.length();
					if (y > 2) {
						/* skip \n, -, and at least one more character */
						if (link_begin > 2) {
							link_begin -= 3;
						}

						/*
						 * positon link_begin to the beginning of the link to be
						 * highlighted
						 */
						while (    isalpha(tmp_string[link_begin])
						        || (tmp_string[link_begin] == '.')
						        || (tmp_string[link_begin] == '_')
						      ) {
							link_begin--;
						}

						/* Chop off \n */
						tmp_string.resize(tmp_string.length() - 1);
						/* Chop off pre-link portion */
						tmp_string = tmp_string.substr(link_begin);

						if (link_begin > manualcol) {
							/* OK, link horizontally fits into screen */
							mvaddstr(manuallinks[i].line - manualpos + 1 - 1,
							         link_begin-manualcol, tmp_string.c_str());
						} else if (link_begin + tmp_string.length() > manualcol) {
							/*
							 * we cut here a part of the link, and draw only what's
							 * visible on screen
							 */
							mvaddstr(manuallinks[i].line - manualpos + 1 - 1,
							         link_begin-manualcol, tmp_string.c_str());
						}
					}
					move(y, x);
				}
			}
			else
			{
				if (i == selected)
					attrset(urlselected);
				else
					attrset(url);
				if (manuallinks[i].carry == 1)
				{
					int ltline = manuallinks[i].line + 1;
					/*
					 * the split part to find is lying down
					 * to the line defined in manlinks(line+1)
					 */
					string tmp_string = manual[ltline];
					strip_manual(tmp_string);
					/* skip spaces */
					string::size_type wsk_idx = 0;
					while (isspace(tmp_string[wsk_idx]))
						wsk_idx++;

					/* find the end of url */
					string::size_type wskend_idx = findurlend(tmp_string, wsk_idx);
					string printable_str = tmp_string.substr(wsk_idx, wskend_idx - wsk_idx);

					/* Print */
					if (wsk_idx < manualcol) {
						mvaddstr(manuallinks[i].line - manualpos + 2, wsk_idx - manualcol,
						         printable_str.c_str());
					} else if (wskend_idx < manualcol) {
						mvaddstr(manuallinks[i].line - manualpos + 2, 0,
						         printable_str.substr(manualcol).c_str());
					}
				}
			}
			if (manuallinks[i].col>manualcol)
				mvaddstr(1 + manuallinks[i].line - manualpos,
						manuallinks[i].col - manualcol, manuallinks[i].name.c_str());
			else if (manuallinks[i].col+manuallinks[i].name.length()>manualcol)
				mvaddstr(1 + manuallinks[i].line - manualpos, 0,
						manuallinks[i].name.substr(manualcol-manuallinks[i].col).c_str());
			attrset(normal);
		}
	}
}

/*
 * calculate from which to start displaying of manual line *man. Skip `mancol'
 * columns. But remember, that *man contains also nonprinteble characters for
 * boldface etc.
 */
static const char*
getmancolumn(const char* man, int mancol)
{
	if (mancol==0) return man;
	while (mancol>0)
	{ if (*(man+1) == 8) man+=3; else man++; mancol--; }
	return man;
}

/* print a manual line */
static void
mvaddstr_manual(int y, int x, const string & my_str)
{
	static string strippedline_string;
	if ((h_regexp.size() > 0) || regex_is_current) {
		strippedline_string = my_str;
		strip_manual(strippedline_string);
	}

	move(y, x);
	for (int i = 0; i < my_str.length(); i++) {
		if ((i > 0) &&(i < my_str.length() - 1))
		{
			/* handle bold highlight */
			if ((my_str[i] == 8) &&(my_str[i - 1] == '_'))
			{
				attrset(manualbold);
				addch(my_str[i] & 0xff);
				addch(my_str[i + 1] & 0xff);
				attrset(normal);
				i++;
				goto label_skip_other;
			}
			/*
			 * if it wasn't bold, check italic, before default, unhighlighted
			 * line will be painted.  We can do it only if i<my_str.length()-3.
			 */
			else if (i < my_str.length() - 3)
				goto label_check_italic;
			else /* unhighlighted */
			{
				addch(my_str[i] & 0xff);
				goto label_skip_other;
			}
		}
		/* italic highlight */
		if (i < my_str.length() - 3)
		{
label_check_italic:
			if ((my_str[i + 1] == 8) &&(my_str[i + 2] == my_str[i]))
			{
				attrset(manualitalic);
				addch(my_str[i] & 0xff);
				i += 2;
				attrset(normal);
			}
			else
			{
				addch(my_str[i] & 0xff);
			}
		}
label_skip_other:;
	}
#ifdef HAVE_BKGDSET
	bkgdset(' ' | normal);
	clrtoeol();
	bkgdset(0);
#else
	myclrtoeol();
#endif
	attrset(normal);
#ifndef ___DONT_USE_REGEXP_SEARCH___
	if (h_regexp.size() > 0) {
		regmatch_t pmatch[1];

		/* if it is after search, then we have user defined regexps+
		   a searched regexp to highlight */
		for (int j = 0; j < h_regexp.size(); j++) {
			const char* strippedline = strippedline_string.c_str();
			const char* tmpstr = strippedline;
			while (!regexec(&h_regexp[j], tmpstr, 1, pmatch, 0)) {
				int n = pmatch[0].rm_eo - pmatch[0].rm_so;
				int rx = pmatch[0].rm_so + tmpstr - strippedline;
				int curY, curX;
				getyx(stdscr, curY, curX);

				attrset(searchhighlight);
				string str_to_print;
				str_to_print.assign(strippedline_string, rx, n);
				mvaddstr(y, rx, str_to_print.c_str());
				attrset(normal);

				tmpstr = tmpstr + pmatch[0].rm_eo;
				move(curY, curX);
			}
		}
	}
	/* Duplicate code, this time for the interactive search */
	if (regex_is_current) {
		regmatch_t pmatch[1];
		const char* strippedline = strippedline_string.c_str();
		const char* tmpstr = strippedline;
		while (!regexec(&current_regex, tmpstr, 1, pmatch, 0)) {
			int n = pmatch[0].rm_eo - pmatch[0].rm_so;
			int rx = pmatch[0].rm_so + tmpstr - strippedline;
			int curY, curX;
			getyx(stdscr, curY, curX);

			attrset(searchhighlight);
			string str_to_print;
			str_to_print.assign(strippedline_string, rx, n);
			mvaddstr(y, rx, str_to_print.c_str());
			attrset(normal);

			tmpstr = tmpstr + pmatch[0].rm_eo;
			move(curY, curX);
		}
	}
#endif
}

/* show the currently visible part of manpage */
static void
showmanualscreen()
{
#ifdef getmaxyx
	/* refresh maxy, maxx values */
	getmaxyx(stdscr, maxy, maxx);
#endif
	attrset(normal);
	/* print all visible text lines */
	for (int i = manualpos;
	     (i < manualpos + (lines_visible)) && (i < manual.size()); 
	     i++) {
		int len = manual[i].length();
		if (len)
			manual[i][len - 1] = ' ';
		/* if we have something to display */
		if (len>manualcol) {
			string yet_another_tmpstr = getmancolumn(manual[i].c_str(),manualcol);
			mvaddstr_manual((i - manualpos) + 1, 0, yet_another_tmpstr);
		}
		else	/* otherwise, just clear the line to eol */
		{
			move((i - manualpos) + 1, 0);
			bkgdset(' ' | normal);
			clrtoeol();
		}
		if (len)
			manual[i][len - 1] = '\n';
	}
#ifdef HAVE_BKGDSET
	bkgdset(' ' | normal);
#endif
	/* and clear to bottom */
	clrtobot();
#ifdef HAVE_BKGDSET
	bkgdset(0);
#endif
	attrset(normal);
	/* add highlights */
	add_highlights();
	/* draw bottomline with user informations */
	attrset(bottomline);
	mymvhline(0, 0, ' ', maxx);
	mymvhline(maxy - 1, 0, ' ', maxx);
	move(maxy - 1, 0);
	if (((manualpos + maxy) < manual.size()) &&(manual.size() > lines_visible))
		printw(_("Viewing line %d/%d, %d%%"),(manualpos - 1 + maxy), manual.size(),((manualpos - 1 + maxy) * 100) / manual.size());
	else
		printw(_("Viewing line %d/%d, 100%%"), manual.size(), manual.size());
	move(maxy - 1, 0);
	attrset(normal);
}

/* viewer function. Handles keyboard actions--main event loop */
static int
manualwork()
{
	/* key, which contains the value entered by user */
	int key = 0;

	bool statusline_locked = false;

	getmaxyx(stdscr, maxy, maxx);
	check_manwidth();

	/* get manualpos from history.  it is set in handlemanual() */
	manualpos = manualhistory[manualhistory.size() - 1].pos;
	/* if there was a valid selected entry, apply it */
	if (manualhistory[manualhistory.size() - 1].selected != -1)
		selected = manualhistory[manualhistory.size() - 1].selected;
	else /* otherwise scan for selected on currently viewed page */
		rescan_selected();

	/* clean screen */
	erase();

	/* user events loop. finish when key_quit */
	while (1)
	{
		/* make getch not wait for user */
		nodelay(stdscr, TRUE);
		/* action -- return ERR */
		key = pinfo_getch();
		/* if there was nothing in buffer */
		if (key == ERR)
		{
			/* then show screen */
			if (!statusline_locked) {
				showmanualscreen();
			}
			wrefresh(stdscr);
			waitforgetch();
			key = pinfo_getch();
		}
		nodelay(stdscr, FALSE);
		statusline_locked = false;
		if (winchanged)
		{
			handlewinch();
			winchanged = 0;
			key = pinfo_getch();
		}
		/************************ keyboard handling **********************************/
		if (key != 0)
		{
			if ((key == keys.print_1) ||
					(key == keys.print_2))
			{
				if (yesno(_("Are you sure you want to print?"), 0))
					printmanual(manual);
			}
			/*====================================================*/
			if ((key == keys.goto_1) ||
					(key == keys.goto_2))
			{
				manuallink my_link;

				/* get user's value */
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				curs_set(1);
				my_link.name = getstring(_("Enter manual name: "));
				curs_set(0);
				noecho();
				move(maxy - 1, 0);
#ifdef HAVE_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);

				my_link.carry = 0;
				my_link.section_mark = 0;
				my_link.section = " ";
				my_link.line = -1;
				my_link.col = -1;
				manuallinks.push_back(my_link);
				return manuallinks.size() - 1;
			}
			/*====================================================*/
			if ((key == keys.goline_1) ||
					(key == keys.goline_2))
			{
				long newpos;
				/* get user's value */
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				curs_set(1);
				string token_string = getstring(_("Enter line: "));
				curs_set(0);
				noecho();
				move(maxy - 1, 0);
#ifdef HAVE_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);
				/* convert string to long.  careful with nondigit strings.  */
				if (token_string != "")
				{
					bool digit_val = true;
					for (int i = 0; i < token_string.length(); i++)
					{
						if (!isdigit(token_string[i]))
							digit_val = false;
					}
					/* move cursor position */
					if (digit_val)
					{
						newpos = atol(token_string.c_str());
						newpos -=(maxy - 1);
						/* FIXME signed/unsigned */
						if ((newpos >= 0) &&(newpos < (signed) manual.size() -(lines_visible)))
							manualpos = newpos;
						else if (newpos > 0)
						/* FIXME signed/unsigned */
							manualpos = (signed) manual.size() -(lines_visible);
						else
							manualpos = 0;
					}
				}
			}
			/*=====================================================*/
			if ((key == keys.shellfeed_1) ||
					(key == keys.shellfeed_2))
			{
				/* for user's shell commands */
				FILE *pipe;

				/* get command name */
				curs_set(1);
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				/* get users cmd */
				string token_string = getstring(_("Enter command: "));
				noecho();
				move(maxy - 1, 0);
#ifdef HAVE_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);

				myendwin();
				system("clear");
				/* open pipe */
				pipe = popen(token_string.c_str(), "w");
				if (pipe != NULL)
				{
					/* and flush the msg to stdin */
					for (int i = 0; i < manual.size(); i++)
						fprintf(pipe, "%s", manual[i].c_str());
					pclose(pipe);
				}
				getchar();
				doupdate();
				curs_set(0);
			}
			/*=====================================================*/
			if ((key == keys.refresh_1) ||
					(key == keys.refresh_2))
			{
				myendwin();
				doupdate();
				refresh();
				curs_set(0);
			}
			/*=====================================================*/
			/* search in current node */
			if ((key == keys.search_1) ||
					(key == keys.search_2))
			{
				int success = 0;
				/* procedure of getting regexp string */
				move(maxy - 1, 0);
				attrset(bottomline);
				echo();
				curs_set(1);
				string token_string;
				/*
				 * searchagain handler. see keys.totalsearch at mainfunction.c
				 * for comments
				 */
				if (!searchagain.search)
				{
					token_string = getstring(_("Enter regexp: "));
					searchagain.lastsearch = token_string;
					searchagain.type = key;
				}
				else
				{
					token_string = searchagain.lastsearch;
					searchagain.search = 0;
				}		/* end of searchagain handler */
				if (token_string == "")
				{
					goto skip_search;
				}
				curs_set(0);
				noecho();
				move(maxy - 1, 0);
#ifdef HAVE_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);
				/* compile regexp expression */
				if (pinfo_re_comp(token_string.c_str()) != 0) {
					/* print error message */
					attrset(bottomline);
					mymvhline(maxy - 1, 0, ' ', maxx);
					move(maxy - 1, 0);
					printw(_("Invalid regular expression;"));
					printw(" ");
					printw(_("Press any key to continue..."));
					getch();
					goto skip_search;
				}
				/* and search for it in all subsequential lines */
				for (int i = manualpos + 1; i < manual.size() - 1; i++)
				{
					string tmpstr;
					/*
					 * glue two following lines together, to find expres- sions
					 * split up into two lines
					 */
					tmpstr = manual[i];
					tmpstr += manual[i+1];
					strip_manual(tmpstr);

					/* execute search */
					if (pinfo_re_exec(tmpstr.c_str())) {
						/* if found, enter here... */
						success = 1;
						string newtmpstr = manual[i + 1];
						strip_manual(newtmpstr);
						/*
						 * if it was found in the second line of the glued
						 * expression.
						 */
						if (pinfo_re_exec(newtmpstr.c_str()))
							manualpos = i + 1;
						else
							manualpos = i;
						break;
					}
				}
				rescan_selected();
				if (!success)
				{
					attrset(bottomline);
					mvaddstr(maxy - 1, 0, _("Search string not found..."));
					statusline_locked = true;
				}
				regex_is_current = true;
			}
			/*=====================================================*/
			/* search again */
			/* see mainfunction.c for comments */
			if ((key == keys.search_again_1) ||
					(key == keys.search_again_2))
			{
				if (searchagain.type != 0)
				{
					searchagain.search = 1;
					ungetch(searchagain.type);
				}
			}
skip_search:
			/*=====================================================*/
			if ((key == keys.twoup_1) ||
					(key == keys.twoup_2))
			{
				ungetch(keys.up_1);
				ungetch(keys.up_1);
			}
			/*=====================================================*/
			if ((key == keys.up_1) ||
					(key == keys.up_2))
			{
				bool selectedchanged = false;
				/* if there are links at all */
				if (selected != -1)
				{
					/* if one is selected */
					if (selected > 0)
						/*
						 * scan for a next visible one, which is above the
						 * current.
						 */
						for (int i = selected - 1; i >= 0; i--)
						{
							if ((manuallinks[i].line >= manualpos) &&
									(manuallinks[i].line < manualpos +(maxy - 1)))
							{
								selected = i;
								selectedchanged = true;
								break;
							}
						}
				}
				/* if new link not found */
				if (!selectedchanged)
				{
					/* move one position up */
					if (manualpos >= 1)
						manualpos--;
					/* and scan for selected again :) */
					for (typeof(manuallinks.size()) i = 0; i < manuallinks.size(); i++)
					{
						if (manuallinks[i].line == manualpos)
						{
							selected = i;
							break;
						}
					}
				}
			}
			/*=====================================================*/
			if ((key == keys.end_1) ||
					(key == keys.end_2))
			{
				/* FIXME signed/unsigned */
				manualpos = (signed) manual.size() -(maxy - 1);
				if (manualpos < 0)
					manualpos = 0;
				selected = manuallinks.size() - 1;
			}
			/*=====================================================*/
			if ((key == keys.nextnode_1) ||
					(key == keys.nextnode_2))
			{
				for (int i = manualpos + 1; i < manual.size(); i++)
				{
					if (manual[i][1] == 8)
					{
						manualpos = i;
						break;
					}
				}
			}
			/*=====================================================*/
			if ((key == keys.prevnode_1) ||
					(key == keys.prevnode_2))
			{
				for (int i = manualpos - 1; i > 0; i--)
				{
					if (manual[i][1] == 8)
					{
						manualpos = i;
						break;
					}
				}
			}
			/*=====================================================*/
			if ((key == keys.pgdn_1) ||
					(key == keys.pgdn_2))
			{
				/* FIXME signed/unsigned */
				if (manualpos +(lines_visible) < (signed) manual.size() -(maxy - 1))
				{
					manualpos +=(lines_visible);
					rescan_selected();
				}
				/* FIXME signed/unsigned */
				else if ((signed) manual.size() -(maxy - 1) >= 1)
				{
					/* FIXME signed/unsigned */
					manualpos = (signed) manual.size() -(maxy - 1);
					selected = manuallinks.size() - 1;
				}
				else
				{
					manualpos = 0;
					selected = manuallinks.size() - 1;
				}
			}
			/*=====================================================*/
			if ((key == keys.home_1) || (key == keys.home_2))
			{
				manualpos = 0;
				rescan_selected();
			}
			/*=====================================================*/
			if ((key == keys.pgup_1) | (key == keys.pgup_2))
			{
				if (manualpos >(maxy - 1))
					manualpos -=(maxy - 1);
				else
					manualpos = 0;
				rescan_selected();
			}
			/*=====================================================*/
			/* top+bottom line \|/ */
			/* see keys.up for comments */
			if ((key == keys.twodown_1) || (key == keys.twodown_2))
			{
				ungetch(keys.down_1);
				ungetch(keys.down_1);
			}
			/*=====================================================*/
			/* top+bottom line \|/ */
			/* see keys.up for comments */
			if ((key == keys.down_1) || (key == keys.down_2))
			{
				bool selectedchanged = false;
				/* signed/unsigned issues with selected FIXME */
				if (selected < manuallinks.size()) {
					for (typeof(manuallinks.size()) i = selected + 1;
					     i < manuallinks.size(); i++) {
						if ((manuallinks[i].line >= manualpos) &&
								(manuallinks[i].line < manualpos +(lines_visible))) {
							selected = i;
							selectedchanged = true;
							break;
						}
					}
				}
				if (!selectedchanged) {
					/* FIXME signed/unsigned */
					if (manualpos < (signed) manual.size() -(maxy - 1))
						manualpos++;
					if (selected < manuallinks.size()) {
						for (typeof(manuallinks.size()) i = selected + 1;
						     i < manuallinks.size(); i++) {
							if ((manuallinks[i].line >= manualpos) &&
									(manuallinks[i].line < manualpos +(lines_visible))) {
								selected = i;
								selectedchanged = true;
								break;
							}
						}
					}
				}
			}
			/*=====================================================*/
			if ((key == keys.back_1) ||
					(key == keys.back_2))
			{
				if (manualhistory.size() - 1)
					return -2;
			}
			/*=====================================================*/
			if ((key == keys.followlink_1) ||
					(key == keys.followlink_2))
			{
				manualhistory[manualhistory.size() - 1].pos = manualpos;
				manualhistory[manualhistory.size() - 1].selected = selected;
				if (selected >= 0)
					if ((manuallinks[selected].line >= manualpos) &&
							(manuallinks[selected].line < manualpos +(maxy - 1)))
					{
						if (manuallinks[selected].section == "HTTPSECTION")
						{
							string tmp_manualname; /* Filled by construct_manualname */
							construct_manualname(tmp_manualname, selected);
							string tmp_cmd;
							tmp_cmd = httpviewer;
							tmp_cmd += " ";
							tmp_cmd += tmp_manualname;
							myendwin();
							system(tmp_cmd.c_str());
							doupdate();
						}
						else if (manuallinks[selected].section == "FTPSECTION")
						{
							string tmp_manualname; /* Filled by construct_manualname */
							construct_manualname(tmp_manualname, selected);
							string tmp_cmd;
							tmp_cmd = ftpviewer;
							tmp_cmd += " ";
							tmp_cmd += tmp_manualname;
							myendwin();
							system(tmp_cmd.c_str());
							doupdate();
						}
						else if (manuallinks[selected].section == "MAILSECTION")
						{
							string tmp_manualname; /* Filled by construct_manualname */
							construct_manualname(tmp_manualname, selected);
							string tmp_cmd;
							tmp_cmd = maileditor;
							tmp_cmd += " ";
							tmp_cmd += tmp_manualname;
							myendwin();
							system(tmp_cmd.c_str());
							doupdate();
						}
						else
						{
							return selected;
						}
					}
			}
			/*=====================================================*/
			if ((key==keys.left_1)||(key==keys.left_2))
				if (manualcol>0) manualcol--;
			if ((key==keys.right_1)||(key==keys.right_2))
				manualcol++;
			/*=====================================================*/
			/********* end of keyboard handling *********************/
			/********* mouse handler ********************************/
#ifdef NCURSES_MOUSE_VERSION
			if (key == KEY_MOUSE)
			{
				MEVENT mouse;
				int done = 0;
				getmouse(&mouse);
				if (mouse.bstate == BUTTON1_CLICKED)
				{
					if ((mouse.y > 0) &&(mouse.y < maxy - 1))
					{
						for (int i = selected; i >= 0; i--)
						{
							if (manuallinks[i].line == mouse.y + manualpos - 1)
							{
								if (manuallinks[i].col <= mouse.x - 1)
								{
									if (manuallinks[i].col + manuallinks[i].name.length() >= mouse.x - 1)
									{
										selected = i;
										done = 1;
										break;
									}
								}
							}
						}
						if (!done)
							for (int i = selected; i < manuallinks.size(); i++)
							{
								if (manuallinks[i].line == mouse.y + manualpos - 1)
								{
									if (manuallinks[i].col <= mouse.x - 1)
									{
										if (manuallinks[i].col + manuallinks[i].name.length() >= mouse.x - 1)
										{
											selected = i;
											done = 1;
											break;
										}
									}
								}
							}
					}		/* end: mouse not on top/bottom line */
					if (mouse.y == 0)
						ungetch(keys.up_1);
					if (mouse.y == maxy - 1)
						ungetch(keys.down_1);
				}		/* end: button_clicked */
				if (mouse.bstate == BUTTON1_DOUBLE_CLICKED)
				{
					if ((mouse.y > 0) &&(mouse.y < maxy - 1))
					{
						for (int i = selected; i >= 0; i--)
						{
							if (manuallinks[i].line == mouse.y + manualpos - 1)
							{
								if (manuallinks[i].col <= mouse.x - 1)
								{
									if (manuallinks[i].col + manuallinks[i].name.length() >= mouse.x - 1)
									{
										selected = i;
										done = 1;
										break;
									}
								}
							}
						}
						if (!done)
							for (int i = selected; i < manuallinks.size(); i++)
							{
								if (manuallinks[i].line == mouse.y + manualpos - 1)
								{
									if (manuallinks[i].col <= mouse.x - 1)
									{
										if (manuallinks[i].col + manuallinks[i].name.length() >= mouse.x - 1)
										{
											selected = i;
											done = 1;
											break;
										}
									}
								}
							}
						if (done)
							ungetch(keys.followlink_1);
					}		/* end: mouse not at top/bottom line */
					if (mouse.y == 0)
						ungetch(keys.pgup_1);
					if (mouse.y == maxy - 1)
						ungetch(keys.pgdn_1);
				}		/* end: button doubleclicked */
			}
#endif
			/*****************************************************************************/
		}
		if ((key == keys.quit_2) ||(key == keys.quit_1))
		{
			if (!ConfirmQuit)
				break;
			else
			{
				if (yesno(_("Are you sure you want to quit?"), QuitConfirmDefault))
					break;
			}
		}
	}
	closeprogram();
	return -1;
}

/* this is something like main() function for the manual viewer code.  */
int
handlemanual(const string& name)
{
	int return_value;
	struct stat statbuf;
	FILE *id;

	string manualname_string; /* Filled by construct_manualname */

	if (tmpfilename1 != "")
	{
		unlink(tmpfilename1.c_str());
	}

	init_curses();
	getmaxyx(stdscr, maxy, maxx);
	myendwin();

	check_manwidth();

	if (!plain_apropos) {
		string cmd_string = "man ";
		cmd_string += ManOptions;
		cmd_string += " ";
		cmd_string += name;
		cmd_string += " ";
		cmd_string += StderrRedirection;
		cmd_string += " > ";
		cmd_string += tmpfilename1;

		int cmd_result;
		cmd_result = system(cmd_string.c_str());
		if (cmd_result != 0) {
			unlink(tmpfilename1.c_str());
			printf(_("Error: No manual page found\n"));
			plain_apropos = 1; /* Fallback */
		} else {
			id = fopen(tmpfilename1.c_str(), "r");
		}
	}
	if (plain_apropos) {
		plain_apropos = 0;
		if (!use_apropos) {
			return 1;
		}
		printf(_("Calling apropos \n"));
		apropos_tmpfilename = tmpdirname;
		apropos_tmpfilename += "/apropos_result";
		string cmd_string = "apropos ";
		cmd_string += name;
		cmd_string += " > ";
		cmd_string += apropos_tmpfilename;
		if (system(cmd_string.c_str()) != 0) {
			printf(_("Nothing appropriate\n"));
			unlink(apropos_tmpfilename.c_str());
			return 1;
		} else {
			id = fopen(apropos_tmpfilename.c_str(), "r");
		}
	}

	init_curses();

	set_initial_history(name);
	/* load manual to memory */
	loadmanual(id);
	fclose(id);
	do {
		/* manualwork handles all actions when viewing man page */
		return_value = manualwork();
		/* Return value may specify link to follow */

		getmaxyx(stdscr, maxy, maxx);
		check_manwidth();

		/* Changing page, so clear regexp */
		regex_is_current = false;

		/* -1 is quit key */
		if (return_value != -1)
		{
			if (tmpfilename2 != "")
			{
				unlink(tmpfilename2.c_str());
			}

			bool historical = false;
			string cmd_string = "man ";
			cmd_string += ManOptions;
			cmd_string += " ";
			if (return_value == -2) {
				/* key_back was pressed */
				if ( (manualhistory.size() - 2) == 0 && apropos_tmpfilename != "")
				{
					id = fopen(apropos_tmpfilename.c_str(), "r");
					loadmanual(id);
					fclose(id);
					continue;
				}
				if (manualhistory[manualhistory.size() - 2].sect == "") {
					cmd_string += manualhistory[manualhistory.size() - 2].name;
				} else {
					cmd_string += manualhistory[manualhistory.size() - 2].sect;
					cmd_string += " ";
					cmd_string += manualhistory[manualhistory.size() - 2].name;
				}
				manualhistory.pop_back();
				historical = true;
			} else {
				/*
				 * key_back was not pressed; and return_value is an offset to
				 * manuallinks
				 */
				construct_manualname(manualname_string, return_value);
				cmd_string += manuallinks[return_value].section;
				cmd_string += " ";
				cmd_string += manualname_string;
			}
			cmd_string += " ";
			cmd_string += StderrRedirection;
			cmd_string += " > ";
			cmd_string += tmpfilename2;
			system(cmd_string.c_str());
			stat(tmpfilename2.c_str(), &statbuf);
			if (statbuf.st_size > 0) {
				string cmd_string2 = "mv ";
				cmd_string2 += tmpfilename2;
				cmd_string2 += " ";
				cmd_string2 += tmpfilename1;
				/* create tmp file containing man page */
				system(cmd_string2.c_str());
				/* open man page */
				id = fopen(tmpfilename1.c_str(), "r");
				if (id != NULL) {
					manhistory my_hist;
					/* now we create history entry for new page */
					if (!historical)
					{
						/*
						 * we can write so since this code applies
						 * only when it's not a history call
						 */
						my_hist.name = manualname_string;
						my_hist.sect = manuallinks[return_value].section;
						my_hist.pos = 0;
						my_hist.selected = -1;
						manualhistory.push_back(my_hist);
					}
					/* loading manual page and its defaults... */
					loadmanual(id);
					fclose(id);
				} else {
					return_value = -1;
				}
			}
		}
	} while (return_value != -1);
	if (apropos_tmpfilename != "")
		unlink(apropos_tmpfilename.c_str());
	/* raw-manpage for scanning */
	return 0;
}


