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

#ifndef ___DONT_USE_REGEXP_SEARCH___
#include "regex.h"
#include <ctype.h>
/* adapted partialy from midnight commander view regexp search */

enum
{
	match_file, match_normal
};

int
__regexp_search(char *pattern, char *string)
{
	int match_type = match_normal;
	static char *old_pattern = NULL;
	static int old_type;
	regmatch_t pmatch[1];
	int i, flags = REG_ICASE;
	int rval;

	if (!old_pattern || strcmp(old_pattern, pattern) || old_type != match_type)
	{
		if (old_pattern)
		{
			free(old_pattern);
			old_pattern = 0;
		}
		for (i = 0; pattern[i] != 0; i++)
		{
			if (isupper((unsigned char) pattern[i]))
			{
				flags = 0;
				break;
			}
		}
		flags |= REG_EXTENDED;
		if (pinfo_re_offset == -1)
		{
			pinfo_re_offset = h_regexp_num;
			if (!h_regexp_num)
				h_regexp = malloc(sizeof(regex_t));
			else
				h_regexp = realloc(h_regexp, sizeof(regex_t) *(h_regexp_num + 1));
		}
		else
		{
			regfree(&h_regexp[pinfo_re_offset]);
		}
		/* invalid regexp */
		if (regcomp(&h_regexp[pinfo_re_offset], pattern, flags))
		{
			return -1;
		}
		old_pattern = strdup(pattern);
		old_type = match_type;
	}
	rval = regexec(&h_regexp[pinfo_re_offset], string, 1, pmatch, 0);
	if (rval != 0)
		return -1;
	else
		return pmatch[0].rm_so;
}

int
regexp_search(char *pattern, char *string)
{
	int newlines = 0, ptr_offset = -1;
	char *__newlines[2];
	char *str = string;
	char *start = str;
	while (*str)
	{
		if (*str == '\n')
		{
			__newlines[newlines] = str + 1;
			newlines++;
		}
		if (newlines == 2)
		{
			*str = 0;
			ptr_offset = __regexp_search(pattern, start);
			*str = '\n';
			newlines = 1;
			if (ptr_offset != -1)
				return (start - string) + ptr_offset;
			if (*(__newlines[0] + 1) != 0)
				start = __newlines[0] + 1;
			if (ptr_offset == -1)
			{
				__newlines[0] = __newlines[1];
			}
		}
		str++;
	}
	ptr_offset = __regexp_search(pattern, start);
	if (ptr_offset != -1)
	{
		return (start - string) + ptr_offset;
	}
	else
		return -1;
}
#else /* non-regexp version of search */
int
__regexp_search(char *pattern, char *string)
{
	char *found = strstr(string, pattern);
	if (found == NULL)
		return -1;
	else
		return (long)(found - string);
}

int
regexp_search(char *pattern, char *string)
{
	int newlines = 0, ptr_offset = -1;
	char *found;
	char *__newlines[2];
	char *str = string;
	char *start = str;
	while (*str)
	{
		if (*str == '\n')
		{
			__newlines[newlines] = str + 1;
			newlines++;
		}
		if (newlines == 2)
		{
			*str = 0;
			ptr_offset = __regexp_search(pattern, start);
			*str = '\n';
			newlines = 1;
			if (ptr_offset != -1)
				return (start - string) + ptr_offset;
			if (*(__newlines[0] + 1) != 0)
				start = __newlines[0] + 1;
			if (ptr_offset == -1)
			{
				__newlines[0] = __newlines[1];
			}
		}
		str++;
	}
	ptr_offset = __regexp_search(pattern, start);
	if (ptr_offset != -1)
	{
		return (start - string) + ptr_offset;
	}
	else
		return -1;
}

#endif
