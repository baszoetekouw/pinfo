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
#include <vector>
using std::vector;

#ifndef ___DONT_USE_REGEXP_SEARCH___
#include <cctype> // for isupper

#include <regex.h>

vector<regex_t> h_regexp;	/* configured regexps to highlight */
regex_t current_regex; /* Selected interactively */
bool prior_regex = false; /* No prior regexes */
bool regex_is_current = false; /* No regex yet */
bool regex_is_global = false; /* Regex not global */
#endif

#ifdef ___DONT_USE_REGEXP_SEARCH___
char *pinfo_re_pattern = 0;
#else
int pinfo_re_offset = -1;
#endif

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
  /* first see if we can compile the regexp */
  regex_t preg;
  if (regcomp(&preg, name, REG_ICASE) != 0)
  {
    /* compilation failed, so return */
    return -1;
  }

	if (prior_regex) {
		regfree(&current_regex);
	}

  /* then copy the compiled expression into the newly allocated space */
  memcpy(&current_regex, &preg, sizeof(preg));
	prior_regex = true;
	return 0;
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
	return !regexec(&current_regex, name, 1, pmatch, 0);
#endif
}

#ifndef ___DONT_USE_REGEXP_SEARCH___

/* adapted partialy from midnight commander view regexp search */

enum
{
	match_file, match_normal
};

int
__regexp_search(const char *pattern, char *string)
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
			if (std::isupper((unsigned char) pattern[i]))
			{
				flags = 0;
				break;
			}
		}
		flags |= REG_EXTENDED;
		if (prior_regex) {
			regfree(&current_regex);
		}
		/* invalid regexp */
		if (regcomp(&current_regex, pattern, flags)) {
			return 0;
		}
		old_pattern = strdup(pattern);
		old_type = match_type;
	}
	rval = regexec(&current_regex, string, 1, pmatch, 0);
	if (rval != 0)
		return -1;
	else
		return pmatch[0].rm_so;
}

int
regexp_search(const char *pattern, char *string)
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
