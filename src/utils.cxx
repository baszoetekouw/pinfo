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
#include <utility> // for std::pair
#include <algorithm> // for std::equal_range

#include "datatypes.h" // for TagTable

string safe_user = "nobody";
string safe_group = "nogroup";

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

/*
 * Compares two strings, ignoring whitespaces(tabs, spaces)
 */
static int
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
gettagtablepos(const string& node)
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

/*
 * Create a vector of strings.  If the strings are concatenated together
 * with separator in between them, the original string will be recovered.
 */
vector<string>
string_explode(const string& to_explode, string::value_type separator) {
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

