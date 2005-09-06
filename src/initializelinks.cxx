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

#define MENU_DOT 0
#define NOTE_DOT 1


bool
compare_hyperlink(HyperObject a, HyperObject b)
{
	/* Should a sort before b? */
	return (a.col < b.col);
}

void
sort_hyperlinks_from_current_line(
	typeof(hyperobjects.begin()) startlink,
	typeof(hyperobjects.begin()) endlink)
{
	std::sort(startlink, endlink, compare_hyperlink);
}

/*
 * checks if an item belongs to tag table. returns 1 on success and 0 on
 * failure.  It should be optimised...
 */
inline int
exists_in_tag_table(const string item)
{
	int result = gettagtablepos(item);
	if (result != -1)
		return 1;
	else
		return 0;
}

/*
 * calculates the length of string between start and end, counting `\t' as
 * filling up to 8 chars. (i.e. at line 22 tab will increment the counter by 2
 * [8-(22-int(22/8)*8)] spaces)
 *
 * Bugs: this doesn't actually work.  FIXME.
 */
int
calculate_len(const char *start, const char *end)
{
	int len = 0;
	while (start < end)
	{
		len++;
		if (*start == '\t')
		{
			len--;
			len +=(8 -((len) -(((len) >> 3) << 3)));
		}
		start++;
	}
	return len;
}

/*
 * Returns index of the first non-URL character (or length, if there
 * is no non-URL character)
 * FIXME: This is really not a sufficient test for a URL.
 */
string::size_type
findurlend(const string str, string::size_type pos)
{
	const char* const allowedchars =
		"QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890-_/~.%=|:@­";

	string::size_type idx;
	idx = str.find_first_not_of(allowedchars, pos);
	if (idx == string::npos) {
		/* All allowed characters! */
		return str.length();
	}
	if ( (str.length() > 0) && (idx > 0) && (str[idx-1] == '.') ) {
		idx--;
	}
	return idx;
}

/*
 * Searchs for a note/menu delimiter.  it may be dot, comma, tab, or newline.
 */
char *
finddot(char *str, int note)
{
	char *ptr = str;
	char *end[4] =
	{
		0, 0, 0, 0
	};
	char *closest = 0;
	int i;
	while (isspace(*ptr))	/* if there are only spaces and newline... */
	{
		if (*ptr == '\n')		/* then it's a `Menu:   \n' entry--skip it */
			return 0;
		ptr++;
	}
	end[0] = strrchr(str, '.');	/* nodename entry may end with dot, comma */
	end[1] = strrchr(str, ',');	/* tabulation, or newline */
	if (!note)
	{
		end[2] = strchr(str, '\t');
		end[3] = strchr(str, '\n');
	}
	else
		note = 2;
	if (end[0])
		closest = end[0];
	else if (end[1])
		closest = end[1];
	else if (end[2])
		closest = end[2];
	else if (end[3])
		closest = end[3];
	for (i = 1; i < note; i++)	/* find the delimiter, which was found most
								   recently */
	{
		if ((end[i] < closest) &&(end[i]))
			closest = end[i];
	}
	return closest;
}

/*
 * Returns index of beginning of username in email address.  If username has
 * length=0 (or no at sign is present), string::npos is returned.
 * Treat string as starting with pos.
 */
string::size_type
findemailstart(string str, string::size_type pos) {
	const char * const allowedchars = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890-_/~.%=|:";
	const string my_str = str.substr(pos);
	const string::size_type at_idx = my_str.find('@');
	if (at_idx == string::npos || at_idx == 0) {
		return string::npos;
	}
	const string::size_type idx = my_str.find_last_not_of(allowedchars, at_idx - 1);
	if (idx == at_idx - 1) {
		return string::npos;
	} else if (idx == string::npos) {
		return 0;
	} else {
		return idx + 1;
	}
}

void
initializelinks(char *line1, char *line2, int line)
{
	char *tmp;
	char *notestart = 0;
	char *quotestart = 0, *quoteend = 0;
	char *buf = (char*)xmalloc(strlen(line1) + strlen(line2) + 1);
	int changed;
	int line1len = strlen(line1);

	typeof(hyperobjects.size()) initial_hyperobjects_size = hyperobjects.size();

	strcpy(buf, line1);		/* copy two lines into one */
	if (strlen(line1))
		buf[strlen(line1) - 1] = ' ';	/* replace trailing '\n' with ' ' */
	strcat(buf, line2);

	/******************************************************************************
	 * First scan for some highlights ;) -- words enclosed with quotes             *
	 ******************************************************************************/
	quoteend = buf;
	do {
		changed = 0;
		if ((quotestart = strchr(quoteend, '`')) != NULL)	/* find start of quoted text */
		{
			if (quotestart < buf + line1len)	/* if it's in the first line of the two glued together */
				if ((quoteend = strchr(quotestart, '\'')) != NULL)		/* find the end of quoted text */
				{
					if (quoteend - quotestart > 1)
					{
						while (!strncmp(quoteend - 1, "n't", 3))	/* if this apostrophe is not a part of "haven't", "wouldn't", etc. */
						{
							quoteend = strchr(quoteend + 1, '\'');
							if (!quoteend)
								break;
						}
						if (quoteend)
						{
							changed = 1;
							HyperObject my_ho;
							my_ho.line = line;
							my_ho.col = calculate_len(buf, quotestart + 1);
							my_ho.breakpos = -1;	/* default */
							if (quoteend > buf + line1len)
							{
								my_ho.breakpos = buf + line1len - quotestart - 1;
							}
							my_ho.type = HIGHLIGHT;
							my_ho.node.assign(quotestart + 1,
							                          quoteend - quotestart	- 1 );
							my_ho.file = "";
							my_ho.tagtableoffset = -1;
							hyperobjects.push_back(my_ho);
						}
					}
				}
		}
	} while (changed);

	/******************************************************************************
	 * Look for e-mail url's                                                       *
	 ******************************************************************************/
	string url_tmpstr = line1;
	string::size_type urlstart = 0;
	string::size_type urlend = 0;
	do {
		changed = 0;
		if ((urlstart = findemailstart(url_tmpstr, urlend)) != string::npos)
		{
			urlend = findurlend(url_tmpstr, urlstart);	/* always successful */
			HyperObject my_ho;
			my_ho.line = line;
			my_ho.col = calculate_len(line1, line1 + urlstart);
			my_ho.breakpos = -1;
			my_ho.type = 6;
			my_ho.node = url_tmpstr.substr(urlstart, urlend - urlstart);
			my_ho.file = "";
			my_ho.tagtableoffset = -1;
			if (my_ho.node.find('.') == string::npos) {
				; /* For some reason don't include it in this case -- why? */
			} else {
				hyperobjects.push_back(my_ho);
			}
			changed = 1;
		}
	} while (changed);

	/******************************************************************************
	 * First try to scan for menu. Use as many security mechanisms, as possible    *
	 ******************************************************************************/

	if ((line1[0] == '*') &&(line1[1] == ' '))	/* menu */
	{
		/******************************************************************************
		 * Scan for normal menu of kind "*(infofile)reference:: comment",  where      *
		 * the infofile parameter is optional, and indicates, that a reference         *
		 * matches different info file.                                                *
		 ******************************************************************************/
		tmp = strstr(line1, "::");	/* "* menulink:: comment" */
		if (tmp != NULL)
		{
			if (line1[2] == '(')	/* if cross-info link */
			{
				char *end = strchr(line1, ')');
				if ((end != NULL) &&(end < tmp))		/* if the ')' char was found, and was before '::' */
				{
					HyperObject my_ho;
					long FilenameLen =(long)(end - line1 - 3);
					long NodenameLen =(long)(tmp - end - 1);
					my_ho.file.assign(line1 + 3, FilenameLen);
					my_ho.node.assign(end + 1, NodenameLen);
					my_ho.type = 0;
					my_ho.line = line;
					my_ho.col = 2;
					my_ho.breakpos = -1;
					hyperobjects.push_back(my_ho);
				}
			}
			else
				/* if not cross-info link */
			{
				HyperObject my_ho;
				long NodenameLen =(long)(tmp - line1 - 2);
				my_ho.file = "";
				my_ho.node.assign(line1 + 2, NodenameLen);
				my_ho.type = 0;
				my_ho.line = line;
				my_ho.col = 2;
				my_ho.breakpos = -1;
				if (exists_in_tag_table(my_ho.node))
				{
					hyperobjects.push_back(my_ho);
				}
			}
		}
		/******************************************************************************
		 * Scan for menu references of form                                            *
		 * "* Comment:[spaces](infofile)reference."                                    *
		 ******************************************************************************/
		else if ((tmp = strrchr(line1, ':')) != NULL)
		{
			char *start = 0, *end = 0, *dot = 0;
			dot = finddot(tmp + 1, MENU_DOT);	/* find the trailing dot */
			if (dot != NULL)
				if (dot + 7 < dot + strlen(dot))
				{
					/* skip possible '.info' filename suffix when searching for ending dot */
					if (strncmp(dot, ".info)", 6) == 0)
						dot = finddot(dot + 1, MENU_DOT);
				}
			/* we make use of sequential AND evaluation: start must not be NULL! */
			if (((start = strchr(tmp, '(')) != NULL) &&(dot != NULL) &&
					((end = strchr(start, ')')) != NULL))
			{
				if (start < dot)	/* security mechanism ;) */
				{
					if (end < dot)	/* security mechanism ;)) */
					{
						long FilenameLen =(long)(end - start - 1);
						long NodenameLen =(long)(dot - end - 1);
						HyperObject my_ho;
						my_ho.file.assign(start + 1, FilenameLen);
						my_ho.node.assign(end + 1, NodenameLen);
						my_ho.type = 1;
						my_ho.line = line;
						my_ho.col = calculate_len(line1, start);
						my_ho.breakpos = -1;
						hyperobjects.push_back(my_ho);
					}
				}
				else
				{
					goto handle_no_file_menu_label;
				}
			}
			else if (dot != NULL)	/* if not cross-info reference */
			{
handle_no_file_menu_label:
				{
					long NodenameLen;
					HyperObject my_ho;

					start = tmp + 1;	/* move after the padding spaces */
					while (isspace(*start))
						start++;
					NodenameLen =(long)(dot - start);
					my_ho.file = "";
					my_ho.node.assign(start, NodenameLen);
					my_ho.type = 1;
					my_ho.line = line;
					my_ho.col = calculate_len(line1, start);
					my_ho.breakpos = -1;
					if (exists_in_tag_table(my_ho.node))
					{
						hyperobjects.push_back(my_ho);
					}
				}
			}
		}
	}
	/******************************************************************************
	 * Handle notes. In similar way as above.                                      *
	 ******************************************************************************/
	else if ((notestart = strstr(buf, "*note")) != NULL)
		goto handlenote;
	else if ((notestart = strstr(buf, "*Note")) != NULL)
	{
handlenote:
		/******************************************************************************
		 * Scan for normal note of kind "*(infofile)reference:: comment", where       *
		 * the infofile parameter is optional, and indicates, that a reference         *
		 * matches different info file.                                                *
		 ******************************************************************************/
		/* make sure that we don't handle notes, which fit in the second line */
		/* Signed-unsigned issues FIXME */
		if ((long)(notestart - buf) < strlen(line1))
		{
			/* we can handle only those, who are in the first line, or who are split up into two lines */
			tmp = strstr(notestart, "::");	/* "*note notelink:: comment" */
			if (tmp != NULL)
			{
				if (notestart[6] == '(')	/* if cross-info link */
				{
					char *end = strchr(notestart, ')');
					if ((end != NULL) &&(end < tmp))	/* if the ')' char was found, and was before '::' */
					{
						HyperObject my_ho;
						long FilenameLen =(long)(end - notestart - 7);
						long NodenameLen =(long)(tmp - end - 1);
						my_ho.file.assign(notestart + 7, FilenameLen);
						my_ho.node.assign(end + 1, NodenameLen);
						my_ho.type = 2;
						if (notestart + 7 - buf < strlen(line1)) {
							my_ho.line = line;
							my_ho.col = calculate_len(buf, notestart + 7);
							/* if the note highlight fits int first line */
							if (tmp - buf < strlen(line1))
								my_ho.breakpos = -1;
								/* we don't need to break highlighting int several lines */
							else
								my_ho.breakpos = strlen(line1) -(long)(notestart + 7 - buf) + 1;	/* otherwise we need it */
						} else {
							my_ho.line = line + 1;
							my_ho.col = calculate_len(buf + strlen(line1), notestart + 7);
							if (tmp - buf < strlen(line1))	/* as above */
								my_ho.breakpos = -1;
							else if ((my_ho.breakpos = strlen(line1) -(long)(notestart + 7 - buf) + 1) == 0)
								my_ho.line--;
						}
						hyperobjects.push_back(my_ho);
					}
				}
				else /* if not cross-info link */
				{
					HyperObject my_ho;
					long NodenameLen =(long)(tmp - notestart - 6);
					my_ho.file = "";
					my_ho.node.assign(notestart + 6, NodenameLen);
					my_ho.type = 2;
					if (notestart + 7 - buf < strlen(line1)) {
						my_ho.line = line;
						my_ho.col = calculate_len(buf, notestart + 7) - 1;
						/* if the note highlight fits int first line */
						if (tmp - buf < strlen(line1))
							my_ho.breakpos = -1;	/* we don't need to break highlighting int several lines */
						else
							my_ho.breakpos = strlen(line1) -(long)(notestart + 7 - buf) + 1;	/* otherwise we need it */
					} else {
						my_ho.line = line + 1;
						my_ho.col = calculate_len(buf + strlen(line1), notestart + 7) - 1;
						if (tmp - buf < strlen(line1))	/* as above */
							my_ho.breakpos = -1;
						else if ((my_ho.breakpos = strlen(line1) -(long)(notestart + 7 - buf) + 1) == 0)
							my_ho.line--;
					}
					if (exists_in_tag_table(my_ho.node))
					{
						hyperobjects.push_back(my_ho);
					}
				}
			}
			/******************************************************************************
			 * Scan for note references of form                                            *
			 * "* Comment:[spaces](infofile)reference."                                    *
			 ******************************************************************************/
			else if ((tmp = strstr(notestart, ":")) != NULL)
			{
				char *start = 0, *end = 0, *dot = 0;
				dot = finddot(tmp + 1, NOTE_DOT);	/* find the trailing dot */
				if (dot != NULL)
					if (dot + 7 < dot + strlen(dot))
					{
						if (strncmp(dot, ".info)", 6) == 0)
							dot = finddot(dot + 1, NOTE_DOT);
					}
				if (((start = strchr(tmp, '(')) != NULL) &&(dot != NULL) &&
						((end = strchr(start, ')')) != NULL))	/* end may be found only if start is nonNULL!!! */
				{
					if (start < dot)	/* security mechanism ;) */
					{
						if (end < dot)	/* security mechanism ;)) */
						{
							long FilenameLen =(long)(end - start - 1);
							long NodenameLen =(long)(dot - end - 1);
							HyperObject my_ho;
							my_ho.file.assign(start + 1, FilenameLen);
							my_ho.node.assign(end + 1, NodenameLen);
							my_ho.type = 3;
							if (start - buf < strlen(line1)) {
								my_ho.line = line;
								my_ho.col = calculate_len(buf, start);
								if (dot - buf < strlen(line1))	/* if the note highlight fits in first line */
									my_ho.breakpos = -1;	/* we don't need to break highlighting int several lines */
								else
									my_ho.breakpos = strlen(line1) -(long)(start - buf);	/* otherwise we need it */
							} else {
								my_ho.line = line + 1;
								my_ho.col = calculate_len(buf + strlen(line1), start);
								my_ho.breakpos = -1;
							}
							hyperobjects.push_back(my_ho);
						}
					}
					else
					{
						goto handle_no_file_note_label;
					}
				}
				else if (dot != NULL)	/* if not cross-info reference */
				{
handle_no_file_note_label:
					{
						long NodenameLen;
						HyperObject my_ho;

						start = tmp + 1;	/* move after the padding spaces */
						while (isspace(*start))
							start++;
						NodenameLen =(long)(dot - start);
						my_ho.file = "";
						my_ho.node.assign(start, NodenameLen);
						my_ho.type = 3;
						if (start - buf < strlen(line1))
						{
							my_ho.line = line;
							my_ho.col = calculate_len(buf, start);
							if (dot - buf < strlen(line1))		/* if the note highlight fits in first line */
								my_ho.breakpos = -1;		/* we don't need to break highlighting int several lines */
							else
								my_ho.breakpos = strlen(line1) -(long)(start - buf);	/* otherwise we need it */
						}
						else
						{
							my_ho.line = line + 1;
							my_ho.col = calculate_len(strlen(line1) + buf, start);
							my_ho.breakpos = -1;
						}
						if (exists_in_tag_table(my_ho.node))
						{
							hyperobjects.push_back(my_ho);
						}
					}
				}
			}
		}
	}
	if (notestart)
		if (notestart + 6 < buf + strlen(buf) + 1)
		{
			tmp = notestart;
			if ((notestart = strstr(notestart + 6, "*Note")) != NULL)
				goto handlenote;
			notestart = tmp;
			if ((notestart = strstr(notestart + 6, "*note")) != NULL)
				goto handlenote;
		}

	/******************************************************************************
	 * Try to scan for some url-like objects in single line; mainly               *
	 * http://[address][space|\n|\t]                                              *
	 * ftp://[address][space|\n|\t]                                               *
	 * username@something.else[space|\n|\t]                                       *
	 *****************************************************************************/
	/* http:// */
	url_tmpstr = line1;
	urlstart = 0;
	urlend = 0;
	while ( (urlstart = url_tmpstr.find("http://", urlend)) != string::npos)
	{
		urlend = findurlend(url_tmpstr, urlstart);	/* always successful */
		HyperObject my_ho;
		my_ho.line = line;
		my_ho.col = calculate_len(line1, line1 + urlstart);
		my_ho.breakpos = -1;
		my_ho.type = 4;
		my_ho.node = url_tmpstr.substr(urlstart, urlend - urlstart);
		my_ho.file = "";
		my_ho.tagtableoffset = -1;
		hyperobjects.push_back(my_ho);
	}
	/* ftp:// */
	url_tmpstr = line1;
	urlstart = 0;
	urlend = 0;
	while ( (urlstart = url_tmpstr.find("ftp://", urlend)) != string::npos)
	{
		urlend = findurlend(url_tmpstr, urlstart);	/* always successful */
		HyperObject my_ho;
		my_ho.line = line;
		my_ho.col = calculate_len(line1, line1 + urlstart);
		my_ho.breakpos = -1;
		my_ho.type = 5;
		my_ho.node = url_tmpstr.substr(urlstart, urlend - urlstart);
		my_ho.file = "";
		my_ho.tagtableoffset = -1;
		hyperobjects.push_back(my_ho);
	}
	if (hyperobjects.size() > initial_hyperobjects_size) {
		typeof(hyperobjects.begin()) first_new_link
			= hyperobjects.end() - (hyperobjects.size() - initial_hyperobjects_size);
		sort_hyperlinks_from_current_line(first_new_link, hyperobjects.end());
	}
	if (buf)
	{
		xfree(buf);
		buf = 0;
	}
}
