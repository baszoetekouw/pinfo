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
#include <vector>
using std::vector;
#include <algorithm> // for std::sort

#include "utils.h"

#define MENU_DOT 0
#define NOTE_DOT 1

bool
compare_hyperlink(HyperObject a, HyperObject b)
{
	/* Should a sort before b? */
	return (a.col < b.col);
}

static void
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
static inline bool
exists_in_tag_table(const string& item)
{
	int result = gettagtablepos(item);
	if (result != -1)
		return true;
	else
		return false;
}

/*
 * Returns index of the first non-URL character (or length, if there
 * is no non-URL character)
 * FIXME: This is really not a sufficient test for a URL.
 */
string::size_type
findurlend(const string& str, string::size_type pos)
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
 * Searches for a note/menu delimiter: period, comma, tab, or newline.
 * Returns index where found.
 * is_note is true (== NOTE_DOT) if we're looking for a note, and
 * false (== MENU_DOT) if we're looking for a menu.
 */
static string::size_type
finddot(const string & str, string::size_type pos, bool is_note)
{
	string::size_type idx = pos;
	while (isspace(str[idx])) {
		/* if there are only spaces and newline... */
		if (str[idx] == '\n') {
			/* `Menu:   \n' entry--skip it */
			return string::npos;
		}
		++idx;
	}

	string::size_type result_idx = string::npos;
	if (!is_note) {
		result_idx = str.find_first_of(".,\t\n", pos);
	} else {
		result_idx = str.find_first_of(".,", pos);
	}
	return result_idx;
}

/*
 * Returns index of beginning of username in email address.  If username has
 * length=0 (or no at sign is present), string::npos is returned.
 * Treat string as starting with pos.
 */
string::size_type
findemailstart(const string & str, string::size_type pos) {
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
		return pos + idx + 1;
	}
}

void
initializelinks(const string & line1, const string & line2, int line)
{
	bool changed;

	typeof(hyperobjects.size()) initial_hyperobjects_size = hyperobjects.size();

	string buf;
	buf = line1;
	if (buf != "") {
		/* replace trailing '\n' with ' ' */
		buf[buf.length() - 1] = ' ';
	}
	buf += line2;

	/******************************************************************************
	 * First scan for some highlights ;) -- words enclosed with quotes             *
	 ******************************************************************************/
	string::size_type quotestart = 0;
	string::size_type quoteend = 0;
	do {
		changed = false;
		if (    ( (quotestart = buf.find('`', quoteend)) != string::npos )
		     && (quotestart < line1.length())
		     && ( (quoteend = buf.find('\'', quotestart)) != string::npos )
		     && (quoteend - quotestart > 1)
		   ) {
			while (    (buf.length() > quoteend + 1) 
				      && (buf.substr(quoteend - 1, 3) == "n't")
			      ) {
				/* if this apostrophe is not a part of "haven't", "wouldn't", etc. */
				/* FIXME: This is totally insufficient */
				quoteend = buf.find('\'', quoteend + 1);
				if (quoteend == string::npos) {
					break;
				}
			}
			if (quoteend == string::npos) {
				continue;
			}
			changed = true;
			HyperObject my_ho;
			my_ho.line = line;
			my_ho.col = calculate_len(buf.c_str(), buf.c_str() + quotestart + 1);
			my_ho.breakpos = -1;	/* default */
			if (quoteend > line1.length()) {
				my_ho.breakpos = line1.length() - (quotestart + 1);
			}
			my_ho.type = HIGHLIGHT;
			my_ho.node = buf.substr(quotestart + 1, quoteend - (quotestart + 1));
			my_ho.file = "";
			my_ho.tagtableoffset = -1;
			hyperobjects.push_back(my_ho);
		}
	} while (changed);

	/******************************************************************************
	 * Look for e-mail url's                                                       *
	 ******************************************************************************/
	string url_tmpstr = line1;
	string::size_type urlstart = 0;
	string::size_type urlend = 0;
	do {
		changed = false;
		if ((urlstart = findemailstart(url_tmpstr, urlend)) != string::npos) {
			urlend = findurlend(url_tmpstr, urlstart);	/* always successful */
			HyperObject my_ho;
			my_ho.line = line;
			my_ho.col = calculate_len(line1.c_str(), line1.c_str() + urlstart);
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
			changed = true;
		}
	} while (changed);

	/******************************************************************************
	 * First try to scan for menu. Use as many security mechanisms, as possible    *
	 ******************************************************************************/
	string::size_type tmp_idx = string::npos;
	if (    (line1.length() >= 3)
	     && (line1[0] == '*')
	     && (line1[1] == ' ')
	   ) {
		/* It looks like a menu line. */
		if ((tmp_idx = line1.find("::")) != string::npos) {
			/******************************************************************************
			 * Scan for normal menu of kind "*(infofile)reference:: comment",  where      *
			 * the infofile parameter is optional, and indicates, that a reference         *
			 * matches different info file.                                                *
			 ******************************************************************************/
			if (line1[2] == '(') {
				/* cross-info link */
				string::size_type end = line1.find(')');
				if ((end != string::npos) && (end < tmp_idx)) {
					/* if the ')' char was found, and was before '::' */
					HyperObject my_ho;
					my_ho.file = line1.substr(3, end - 3);
					my_ho.node = line1.substr(end + 1, tmp_idx - (end + 1));
					my_ho.type = 0;
					my_ho.line = line;
					my_ho.col = 2;
					my_ho.breakpos = -1;
					hyperobjects.push_back(my_ho);
				}
			} else {
				/* not cross-info link */
				HyperObject my_ho;
				my_ho.file = "";
				my_ho.node = line1.substr(2, tmp_idx - 2);
				my_ho.type = 0;
				my_ho.line = line;
				my_ho.col = 2;
				my_ho.breakpos = -1;
				if (exists_in_tag_table(my_ho.node)) {
					hyperobjects.push_back(my_ho);
				}
			}
		} else if ((tmp_idx = line1.find_last_of(':')) != string::npos) {
			/******************************************************************************
			 * Scan for menu references of form                                            *
			 * "* Comment:[spaces](infofile)reference."                                    *
			 ******************************************************************************/
			string::size_type start;
			string::size_type end;
			string::size_type dot;
			if (    ( (start = line1.find('(', tmp_idx)) != string::npos )
					 && ( (end = line1.find(')', start + 1)) != string::npos )
					 && ( (dot = finddot(line1, end + 1, MENU_DOT)) != string::npos )
			   ) {
				if (    (dot + 7 < line1.length())
				     && ( line1.substr(dot, 6) == ".info)" )
				   ) {
					/* skip possible '.info' filename suffix
					 * when searching for ending dot */
						dot = finddot(line1, dot + 1, MENU_DOT);
				}
				HyperObject my_ho;
				my_ho.file = line1.substr(start + 1, end - (start + 1));
				my_ho.node = line1.substr(end + 1, dot - (end + 1));
				my_ho.type = 1;
				my_ho.line = line;
				my_ho.col = calculate_len(line1.c_str(), line1.c_str() + start);
				my_ho.breakpos = -1;
				hyperobjects.push_back(my_ho);
			} else {
				/* not cross-info reference */
				HyperObject my_ho;

				start = tmp_idx + 1;
				/* move after the padding spaces */
				while (isspace(line1[start])) {
					start++;
				}
				if ((dot = finddot(line1, start, MENU_DOT)) != string::npos) {
					if (    (dot + 7 < line1.length())
					     && ( line1.substr(dot, 6) == ".info)" )
					   ) {
						/* skip possible '.info' filename suffix
						 * when searching for ending dot */
							dot = finddot(line1, dot + 1, MENU_DOT);
					}
					my_ho.file = "";
					my_ho.node = line1.substr(start, dot - start);
					my_ho.type = 1;
					my_ho.line = line;
					my_ho.col = calculate_len(line1.c_str(), line1.c_str() + start);
					my_ho.breakpos = -1;
					if (exists_in_tag_table(my_ho.node)) {
						hyperobjects.push_back(my_ho);
					}
				}
			}
		}
	} else {
		/******************************************************************************
		 * Handle notes. In similar way as above.                                      *
		 ******************************************************************************/
    /* This is in an else so that menu lines never contain notes.
     * Is this right?  FIXME */
		string::size_type notestart = string::npos;
		string::size_type old_noteend = 0;
		while (    ((notestart = line1.find("*note", old_noteend)) != string::npos)
					  || ((notestart = line1.find("*Note", old_noteend)) != string::npos)
					) {
			/******************************************************************************
			 * Scan for normal note of kind "*(infofile)reference:: comment", where       *
			 * the infofile parameter is optional, and indicates, that a reference         *
			 * matches different info file.                                                *
			 ******************************************************************************/
			if ((tmp_idx = buf.find("::", notestart)) != string::npos) {
				if (buf[notestart + 6] == '(') {
					/* cross-info link */
					string::size_type end = buf.find(')', notestart);
					if ((end != string::npos) && (end < tmp_idx)) {
						/* the ')' char was found, and was before '::' */
						HyperObject my_ho;
						my_ho.file = buf.substr(notestart + 7, end - (notestart + 7));
						my_ho.node = buf.substr(end + 1, tmp_idx - (end + 1));
						my_ho.type = 2;
						if (notestart + 7 < line1.length()) {
							my_ho.line = line;
							my_ho.col = calculate_len(buf.c_str(), buf.c_str() + notestart + 7);
							if (tmp_idx < line1.length()) {
								/* if the note highlight fits into the first line */
								/* we don't need to break highlighting into several lines */
								my_ho.breakpos = -1;
							} else {
								/* otherwise we need it */
								my_ho.breakpos = line1.length() - (notestart + 7) + 1;
							}
						} else {
							my_ho.line = line + 1;
							my_ho.col = calculate_len(buf.c_str() + line1.length(), buf.c_str() + notestart + 7);
							if (tmp_idx < line1.length())	{
								my_ho.breakpos = -1;
							} else {
								my_ho.breakpos = line1.length() - (notestart + 7) + 1;
								if (my_ho.breakpos == 0) {
									my_ho.line--;
								}
							}
						}
						hyperobjects.push_back(my_ho);
					}
				} else {
					/* not cross-info link */
					HyperObject my_ho;
					my_ho.file = "";
					my_ho.node = buf.substr(notestart + 6, tmp_idx - (notestart + 6));
					my_ho.type = 2;
					if (notestart + 7 < line1.length()) {
						my_ho.line = line;
						my_ho.col = calculate_len(buf.c_str(), buf.c_str() + notestart + 7) - 1;
						if (tmp_idx < line1.length()) {
							/* if the note highlight fits into the first line */
							/* we don't need to break highlighting into several lines */
							my_ho.breakpos = -1;
						} else {
							/* otherwise we need it */
							my_ho.breakpos = line1.length() - (notestart + 7) + 1;
						}
					} else {
						my_ho.line = line + 1;
						my_ho.col = calculate_len(buf.c_str() + line1.length(), buf.c_str() + notestart + 7) - 1;
						if (tmp_idx < line1.length()) {
							my_ho.breakpos = -1;
						} else {
							my_ho.breakpos = line1.length() - (notestart + 7) + 1;
							if (my_ho.breakpos == 0) {
								my_ho.line--;
							}
						}
					}
					if (exists_in_tag_table(my_ho.node)) {
						hyperobjects.push_back(my_ho);
					}
				}
			} else if ((tmp_idx = buf.find(':', notestart)) != string::npos) {
				/******************************************************************************
				 * Scan for note references of form                                            *
				 * "* Comment:[spaces](infofile)reference."                                    *
				 ******************************************************************************/
				/* find the end of the note */
				string::size_type dot = finddot(buf, tmp_idx + 1, NOTE_DOT);
				if (dot != string::npos) {
					if (dot + 7 < buf.length()) {
						/* skip possible '.info' filename suffix
						 * when searching for ending dot */
						if ( buf.substr(dot, 6) == ".info)" ) {
							dot = finddot(buf, dot + 1, NOTE_DOT);
						}
					}
				}
				if (dot != string::npos) {
					string::size_type start;
					string::size_type end;
					if (    ( (start = buf.find('(', tmp_idx)) != string::npos )
					     && (start < dot)
							 && ( (end = buf.find(')', start)) != string::npos )
			  		   && (end < dot)
				  	 ) {
						HyperObject my_ho;
						my_ho.file = buf.substr(start + 1, dot - (end + 1));
						my_ho.node = buf.substr(end + 1, dot - (end + 1));
						my_ho.type = 3;
						if (start < line1.length()) {
							my_ho.line = line;
							my_ho.col = calculate_len(buf.c_str(), buf.c_str() + start);
							if (dot < line1.length()) {
								/* if the note highlight fits in first line */
								/* we don't need to break highlighting into several lines */
								my_ho.breakpos = -1;
							} else {
								/* otherwise we need it */
								my_ho.breakpos = line1.length() - start;	
							}
						} else {
							my_ho.line = line + 1;
							my_ho.col = calculate_len(buf.c_str() + line1.length(), buf.c_str() + start);
							my_ho.breakpos = -1;
						}
						hyperobjects.push_back(my_ho);
					} else {
						/* not cross-info reference */
						HyperObject my_ho;

						start = tmp_idx + 1;
						/* move after the padding spaces */
						while (isspace(buf[start]))
							start++;

						my_ho.file = "";
						my_ho.node = buf.substr(start, dot - start);
						my_ho.type = 3;
						if (start < line1.length()) {
							my_ho.line = line;
							my_ho.col = calculate_len(buf.c_str(), buf.c_str() + start);
							if (dot < line1.length()) {
								/* if the note highlight fits in first line */
								/* we don't need to break highlighting into several lines */
								my_ho.breakpos = -1;
							} else {
								/* otherwise we need it */
								my_ho.breakpos = line1.length() - start;
							}
						} else {
							my_ho.line = line + 1;
							my_ho.col = calculate_len(buf.c_str() + line1.length(), buf.c_str() + start);
							my_ho.breakpos = -1;
						}
						if (exists_in_tag_table(my_ho.node)) {
							hyperobjects.push_back(my_ho);
						}
					}
				}
			}
			old_noteend = notestart + 6;
			if (old_noteend > line1.length()) {
				old_noteend = line1.length(); /* Don't start searches past here */
			}
		}
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
	while ( (urlstart = url_tmpstr.find("http://", urlend)) != string::npos) {
		urlend = findurlend(url_tmpstr, urlstart);	/* always successful */
		HyperObject my_ho;
		my_ho.line = line;
		my_ho.col = calculate_len(line1.c_str(), line1.c_str() + urlstart);
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
		my_ho.col = calculate_len(line1.c_str(), line1.c_str() + urlstart);
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
}
