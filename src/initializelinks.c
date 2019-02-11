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

#define MENU_DOT 0
#define NOTE_DOT 1

int
compare_hyperlink(const void *a, const void *b)
{
	return ((HyperObject *) a)->col -((HyperObject *) b)->col;
}

void
sort_hyperlinks_from_current_line(long startlink, long endlink)
{
	qsort(hyperobjects + startlink, endlink - startlink, sizeof(HyperObject), compare_hyperlink);
}

/*
 * Compares two strings, ignoring whitespaces(tabs, spaces)
 */
int
compare_tag_table_string(char *base, char *compared)
{
	int i, j;

	j = 0;

	for (i = 0; base[i] != 0; i++)
	{
		if (base[i] != compared[j])
		{
			if ((isspace(compared[j])) &&(isspace(base[i])));	/* OK--two blanks */
			else if (isspace(compared[j]))
				i--;		/* index of `base' should be unchanged after for's i++ */
			else if (isspace(base[i]))
				j--;		/* index of `compared' stands in place
							   and waits for base to skip blanks */
			else
				return (int) base[i] -(int) compared[j];
		}
		j++;
	}
	while (compared[j])		/* handle trailing whitespaces of variable `compared' */
	{
		if (!isspace(compared[j]))
			return (int) base[i] -(int) compared[j];
		j++;
	}
	return 0;
}

/*
 * checks if an item belongs to tag table. returns 1 on success and 0 on
 * failure.  It should be optimised...
 */
int
exists_in_tag_table(char *item)
{
	if (gettagtablepos(item) != -1)
		return 1;
	else
		return 0;
}


void
freelinks()			/* frees space allocated previously by node-links */
{
	if ((hyperobjects)&&(hyperobjectcount))
		xfree(hyperobjects);
	hyperobjects = 0;
	hyperobjectcount = 0;
}

/*
 * Finds url end.  It is recognized by an invalid character.
 */
/* TODO: fix possible string overflow (no bounds checking) */
char *
findurlend(char *str)
{
	char *end;
	char *allowedchars = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890-_/~.%=|:@";
	end = str;
	while (strchr(allowedchars, *end) != NULL)
		++end;
	if (end > str)
	{
		if (*(end - 1) == '.')
			end--;
	}
	assert(end>=str);
	return end;
}

/*
 * Searchs for a note/menu delimiter.  it may be dot, comma, tab, or newline.
 */
/* TODO: fix possible string overflow (no bounds checking) */
char *
finddot(char *str, int mynote)
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
	end[0] = strchr(str, '.');	/* nodename entry may end with dot, comma */
	end[1] = strchr(str, ',');	/* tabulation, or newline */
	if (!mynote)
	{
		end[2] = strchr(str, '\t');
		end[3] = strchr(str, '\n');
	}
	else
		mynote = 2;
	if (end[0])
		closest = end[0];
	else if (end[1])
		closest = end[1];
	else if (end[2])
		closest = end[2];
	else if (end[3])
		closest = end[3];
	for (i = 1; i < mynote; i++)	/* find the delimiter, which was found most
								   recently */
	{
		if ((end[i] < closest) &&(end[i]))
			closest = end[i];
	}
	assert(closest>=str);
	return closest;
}

/*
 * Moves you to the beginning of username in email address.  If username has
 * length=0, NULL is returned.
 */
/* TODO: fix possible string overflow (no bounds checking) */
char *
findemailstart(char *str)
{
	char *allowedchars = "QWERTYUIOPASDFGHJKLZXCVBNMqwertyuiopasdfghjklzxcvbnm1234567890-_/~.%=|:";
	char *at = strchr(str, '@');
	if (at)
	{
		while (at > str)
		{
			at--;
			if (strchr(allowedchars, *at) == NULL)
			{
				if (*(at + 1) != '@')
					return at + 1;
				else
					return 0;
			}
		}
		if (*at != '@')
			return at;
		else
			return 0;
	}
	return 0;
}

void
initializelinks(char *line1, char *line2, int line)
{
	char *tmp;
	char *notestart = 0, *urlstart = 0, *urlend = 0;
	char *quotestart = 0, *quoteend = 0;
	char *buf = xmalloc(strlen(line1) + strlen(line2) + 1);
	/* required to sort properly the hyperlinks from current line only */
	unsigned long initialhyperobjectcount = hyperobjectcount;
	int changed;
	int line1len = strlen(line1);

	strcpy(buf, line1);		/* copy two lines into one */
	if (strlen(line1))
		buf[strlen(line1) - 1] = ' ';	/* replace trailing '\n' with ' ' */
	strcat(buf, line2);
	/******************************************************************************
	 * First scan for some highlights ;) -- words enclosed with quotes             *
	 ******************************************************************************/
	quoteend = buf;
	do
	{
		changed = 0;
		/* find start of quoted text */
		if ( ((quotestart = strchr(quoteend, '`')) != NULL)
			 /* if it's in the first line of the two glued together */
			 && (quotestart < buf + line1len)
			 /* find the end of quoted text */
			 && ((quoteend = strchr(quotestart, '\'')) != NULL)
			 &&  (quoteend - quotestart > 1))
		{
			/* if this apostrophe is not a part of "haven't",
			 * "wouldn't", etc. */
			while (!strncmp(quoteend - 1, "n't", 3))
			{
				quoteend = strchr(quoteend + 1, '\'');
				if (!quoteend)
					break;
			}
			if (quoteend)
			{
				changed = 1;
				if (!hyperobjectcount)
					hyperobjects = xmalloc(sizeof(HyperObject));
				else
				{
					hyperobjects = xrealloc(hyperobjects,
							sizeof(HyperObject) *(hyperobjectcount + 1));
				}
				hyperobjects[hyperobjectcount].line = line;
				hyperobjects[hyperobjectcount].col =
					calculate_len(buf, quotestart + 1);
				hyperobjects[hyperobjectcount].breakpos = -1;	/* default */
				if (quoteend > buf + line1len)
				{
					hyperobjects[hyperobjectcount].breakpos =
						buf + line1len - quotestart - 1;
				}
				hyperobjects[hyperobjectcount].type = HIGHLIGHT;
				strncpy( hyperobjects[hyperobjectcount].node,
						quotestart + 1, quoteend - quotestart - 1);
				hyperobjects[hyperobjectcount].node[quoteend - quotestart - 1] = 0;
				strcpy(hyperobjects[hyperobjectcount].file, "");
				hyperobjects[hyperobjectcount].nodelen =
					strlen(hyperobjects[hyperobjectcount].node);
				hyperobjects[hyperobjectcount].filelen =
					strlen(hyperobjects[hyperobjectcount].file);
				hyperobjects[hyperobjectcount].tagtableoffset = -1;
				hyperobjectcount++;
			}
		}
	}
	while (changed);
	/******************************************************************************
	 * Look for e-mail url's                                                       *
	 ******************************************************************************/
	urlend = line1;
	do
	{
		changed = 0;
		if ((urlstart = findemailstart(urlend)) != NULL)
		{
			urlend = findurlend(urlstart);	/* always successful */
			if (!hyperobjectcount)
				hyperobjects = xmalloc(sizeof(HyperObject));
			else
			{
				hyperobjects = xrealloc(hyperobjects,
						sizeof(HyperObject) *(hyperobjectcount + 1));
			}
			hyperobjects[hyperobjectcount].line = line;
			hyperobjects[hyperobjectcount].col = calculate_len(line1, urlstart);
			hyperobjects[hyperobjectcount].breakpos = -1;
			hyperobjects[hyperobjectcount].type = 6;
			strncpy(hyperobjects[hyperobjectcount].node,
					urlstart, urlend - urlstart);
			hyperobjects[hyperobjectcount].node[urlend - urlstart] = 0;
			strcpy(hyperobjects[hyperobjectcount].file, "");
			hyperobjects[hyperobjectcount].nodelen =
				strlen(hyperobjects[hyperobjectcount].node);
			hyperobjects[hyperobjectcount].filelen =
				strlen(hyperobjects[hyperobjectcount].file);
			hyperobjects[hyperobjectcount].tagtableoffset = -1;
			if (strchr(hyperobjects[hyperobjectcount].node, '.') == NULL)
			{
				if (!hyperobjectcount)
					xfree(hyperobjects);
			}
			else
				hyperobjectcount++;
			changed = 1;
		}

	}
	while (changed);
	/******************************************************************************
	 * First try to scan for menu. Use as many security mechanisms, as possible    *
	 ******************************************************************************/

	if ((line1[0] == '*') &&(line1[1] == ' '))	/* menu */
	{
		/*************************************************************************
		 * Scan for normal menu of kind "*(infofile)reference:: comment",  where *
		 * the infofile parameter is optional, and indicates, that a reference   *
		 * matches different info file.                                          *
		 *************************************************************************/
		tmp = strstr(line1, "::");	/* "* menulink:: comment" */
		if (tmp != NULL)
		{
			if (!hyperobjectcount)
				hyperobjects = xmalloc(sizeof(HyperObject));
			else
			{
				hyperobjects = xrealloc(hyperobjects,
						sizeof(HyperObject) *(hyperobjectcount + 1));
			}
			if (line1[2] == '(')	/* if cross-info link */
			{
				char *end = strchr(line1, ')');
				/* if the ')' char was found, and was before '::' */
				if ((end != NULL) &&(end < tmp))
				{
					long FilenameLen =(long)(end - line1 - 3);
					long NodenameLen =(long)(tmp - end - 1);
					strncpy(hyperobjects[hyperobjectcount].file,
							line1 + 3, FilenameLen);
					hyperobjects[hyperobjectcount].file[FilenameLen] = 0;
					strncpy(hyperobjects[hyperobjectcount].node,
							end + 1, NodenameLen);
					hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
					hyperobjects[hyperobjectcount].type = 0;
					hyperobjects[hyperobjectcount].line = line;
					hyperobjects[hyperobjectcount].col = 2;
					hyperobjects[hyperobjectcount].breakpos = -1;
					hyperobjects[hyperobjectcount].nodelen =
						strlen(hyperobjects[hyperobjectcount].node);
					hyperobjects[hyperobjectcount].filelen =
						strlen(hyperobjects[hyperobjectcount].file);
					hyperobjectcount++;
				}
			}
			else
			{
				/* if not cross-info link */
				long NodenameLen =(long)(tmp - line1 - 2);
				int goodHit = 0;
				strcpy(hyperobjects[hyperobjectcount].file, "");
				strncpy(hyperobjects[hyperobjectcount].node,
						line1 + 2, NodenameLen);
				hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
				hyperobjects[hyperobjectcount].type = 0;
				hyperobjects[hyperobjectcount].line = line;
				hyperobjects[hyperobjectcount].col = 2;
				hyperobjects[hyperobjectcount].breakpos = -1;
				hyperobjects[hyperobjectcount].nodelen =
					strlen(hyperobjects[hyperobjectcount].node);
				hyperobjects[hyperobjectcount].filelen =
					strlen(hyperobjects[hyperobjectcount].file);
				if (exists_in_tag_table(hyperobjects[hyperobjectcount].node))
				{
					hyperobjectcount++;	/* yep, this was a good hit */
					goodHit = 1;
				}
				if (!goodHit && !hyperobjectcount)
				{
					xfree(hyperobjects);
					hyperobjects = 0;
				}
			}
		}
		/************************************************************
		 * Scan for menu references of form                         *
		 * "* Comment:[spaces](infofile)reference."                 *
		 ******************************************************************************/
		else if ((tmp = strrchr(line1, ':')) != NULL)
		{
			char *start = 0, *end = 0, *dot = 0;
			/* find the trailing dot */
			dot = finddot(tmp + 1, MENU_DOT);
			if (dot != NULL && (dot + 7 < dot + strlen(dot)) )
			{
				/* skip possible '.info' filename suffix when
				 * searching for ending dot */
				if (strncmp(dot, ".info)", 6) == 0)
					dot = finddot(dot + 1, MENU_DOT);
			}
			/* we make use of sequential AND evaluation: start must not be NULL! */
			if (((start = strchr(tmp, '(')) != NULL) &&(dot != NULL) &&
					((end = strchr(start, ')')) != NULL))
			{
				if ( (start < dot) && (end < dot) )	/* security mechanism ;) */
				{
					long FilenameLen =(long)(end - start - 1);
					long NodenameLen =(long)(dot - end - 1);
					if (!hyperobjectcount)
						hyperobjects = xmalloc(sizeof(HyperObject));
					else
					{
						hyperobjects = xrealloc(hyperobjects,
								sizeof(HyperObject) *(hyperobjectcount + 1));
					}
					strncpy(hyperobjects[hyperobjectcount].file,
							start + 1, FilenameLen);
					hyperobjects[hyperobjectcount].file[FilenameLen] = 0;
					strncpy(hyperobjects[hyperobjectcount].node,
							end + 1, NodenameLen);
					hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
					hyperobjects[hyperobjectcount].type = 1;
					hyperobjects[hyperobjectcount].line = line;
					hyperobjects[hyperobjectcount].col =
						calculate_len(line1, start);
					hyperobjects[hyperobjectcount].breakpos = -1;
					hyperobjects[hyperobjectcount].nodelen =
						strlen(hyperobjects[hyperobjectcount].node);
					hyperobjects[hyperobjectcount].filelen =
						strlen(hyperobjects[hyperobjectcount].file);
					hyperobjectcount++;
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
					int goodHit = 0;	/* has val of 1, if it's a good hit */
					if (!hyperobjectcount)
						hyperobjects = xmalloc(sizeof(HyperObject));
					else
					{
						hyperobjects = xrealloc(hyperobjects,
								sizeof(HyperObject) *(hyperobjectcount + 1));
					}

					start = tmp + 1;	/* move after the padding spaces */
					while (isspace(*start))
						start++;
					NodenameLen =(long)(dot - start);
					strcpy(hyperobjects[hyperobjectcount].file, "");
					strncpy(hyperobjects[hyperobjectcount].node,
							start, NodenameLen);
					hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
					hyperobjects[hyperobjectcount].type = 1;
					hyperobjects[hyperobjectcount].line = line;
					hyperobjects[hyperobjectcount].col = calculate_len(line1, start);
					hyperobjects[hyperobjectcount].breakpos = -1;
					hyperobjects[hyperobjectcount].nodelen =
						strlen(hyperobjects[hyperobjectcount].node);
					hyperobjects[hyperobjectcount].filelen =
						strlen(hyperobjects[hyperobjectcount].file);
					if (exists_in_tag_table(hyperobjects[hyperobjectcount].node))
					{
						hyperobjectcount++;		/* yep, this was a good hit */
						goodHit = 1;
					}
					if (!goodHit && !hyperobjectcount)
					{
						xfree(hyperobjects);
						hyperobjects = 0;
					}
				}
			}
		}
	}
	/******************************************************************************
	 * Handle notes. In similar way as above.                                      *
	 ******************************************************************************/
	else if (    ( (notestart = strstr(buf, "*note")) != NULL )
		      || ( (notestart = strstr(buf, "*Note")) != NULL ) )
	{
handlenote:
		/*************************************************************************
		 * Scan for normal note of kind "*(infofile)reference:: comment", where  *
		 * the infofile parameter is optional, and indicates, that a reference   *
		 * matches different info file.                                          *
		 *************************************************************************/
		/* make sure that we don't handle notes, which fit in the second line */
		/* Signed-unsigned issues FIXME */
		if (notestart  < buf + strlen(line1))
		{
			/* we can handle only those, who are in the first line,
			 * or who are split up into two lines */
			tmp = strstr(notestart, "::");	/* "*note notelink:: comment" */
			if (tmp != NULL)
			{
				if (!hyperobjectcount)
					hyperobjects = xmalloc(sizeof(HyperObject));
				else
					hyperobjects = xrealloc(hyperobjects, sizeof(HyperObject) *(hyperobjectcount + 1));
				if (notestart[6] == '(')	/* if cross-info link */
				{
					char *end = strchr(notestart, ')');
					/* if the ')' char was found, and was before '::' */
					if ((end != NULL) &&(end < tmp))
					{
						long FilenameLen =(long)(end - notestart - 7);
						long NodenameLen =(long)(tmp - end - 1);
						strncpy(hyperobjects[hyperobjectcount].file,
								notestart + 7, FilenameLen);
						hyperobjects[hyperobjectcount].file[FilenameLen] = 0;
						strncpy(hyperobjects[hyperobjectcount].node,
								end + 1, NodenameLen);
						hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
						hyperobjects[hyperobjectcount].type = 2;
						if (notestart + 7 < buf + strlen(line1))
						{
							hyperobjects[hyperobjectcount].line = line;
							hyperobjects[hyperobjectcount].col = calculate_len(buf, notestart + 7);
							/* if the note highlight fits int first line */
							if (tmp < buf + strlen(line1))
							{
								/* we don't need to break highlighting
								 * into several lines */
								hyperobjects[hyperobjectcount].breakpos = -1;
							}
							else
							{
								/* otherwise we need it */
								hyperobjects[hyperobjectcount].breakpos =
									strlen(line1) -(long)(notestart + 7 - buf) + 1;
							}
						}
						else
						{
							hyperobjects[hyperobjectcount].line = line + 1;
							hyperobjects[hyperobjectcount].col =
								calculate_len(buf + strlen(line1), notestart + 7);
							if (tmp < buf + strlen(line1))	/* as above */
								hyperobjects[hyperobjectcount].breakpos = -1;
							else if ((hyperobjects[hyperobjectcount].breakpos =
										strlen(line1) -
										(long)(notestart + 7 - buf) + 1) == 0)
							{
								hyperobjects[hyperobjectcount].line--;
							}
						}
						hyperobjects[hyperobjectcount].nodelen =
							strlen(hyperobjects[hyperobjectcount].node);
						hyperobjects[hyperobjectcount].filelen =
							strlen(hyperobjects[hyperobjectcount].file);
						hyperobjectcount++;
					}
				}
				else /* if not cross-info link */
				{
					long NodenameLen =(long)(tmp - notestart - 6);
					int goodHit = 0;
					strcpy(hyperobjects[hyperobjectcount].file, "");
					strncpy(hyperobjects[hyperobjectcount].node,
							notestart + 6, NodenameLen);
					hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
					hyperobjects[hyperobjectcount].type = 2;
					hyperobjects[hyperobjectcount].nodelen =
						strlen(hyperobjects[hyperobjectcount].node);
					hyperobjects[hyperobjectcount].filelen =
						strlen(hyperobjects[hyperobjectcount].file);
					if (notestart + 7 < buf + strlen(line1))
					{
						hyperobjects[hyperobjectcount].line = line;
						hyperobjects[hyperobjectcount].col =
							calculate_len(buf, notestart + 7) - 1;
						/* if the note highlight fits int first line */
						if (tmp < buf + strlen(line1))
						{
							/* we don't need to break highlighting into
							 * several lines */
							hyperobjects[hyperobjectcount].breakpos = -1;
						}
						else
						{
							/* otherwise we need it */
							hyperobjects[hyperobjectcount].breakpos =
								strlen(line1) -(long)(notestart + 7 - buf) + 1;
						}
					}
					else
					{
						hyperobjects[hyperobjectcount].line = line + 1;
						hyperobjects[hyperobjectcount].col =
							calculate_len(buf + strlen(line1), notestart + 7) - 1;
						if (tmp < buf + strlen(line1))	/* as above */
							hyperobjects[hyperobjectcount].breakpos = -1;
						else if ((hyperobjects[hyperobjectcount].breakpos =
									strlen(line1)
									-(long)(notestart + 7 - buf) + 1) == 0)
						{
							hyperobjects[hyperobjectcount].line--;
						}
					}
					if (exists_in_tag_table(hyperobjects[hyperobjectcount].node))
					{
						hyperobjectcount++;	/* yep, this was a good hit */
						goodHit = 1;
					}
					if (!goodHit && !hyperobjectcount)
					{
						xfree(hyperobjects);
						hyperobjects = 0;
					}
				}
			}
			/*****************************************************************
			 * Scan for note references of form                              *
			 * "* Comment:[spaces](infofile)reference."                      *
			 *****************************************************************/
			else if ((tmp = strstr(notestart, ":")) != NULL)
			{
				char *start = 0, *end = 0, *dot = 0;
				dot = finddot(tmp + 1, NOTE_DOT);	/* find the trailing dot */
				if ( (dot != NULL) && ( dot + 7 < dot + strlen(dot)) )
				{
					if (strncmp(dot, ".info)", 6) == 0)
						dot = finddot(dot + 1, NOTE_DOT);
				}
				if (    ((start = strchr(tmp, '(')) != NULL)
					 && (dot != NULL)
					 && ((end = strchr(start, ')')) != NULL))
					/* end may be found only if start is nonNULL!!! */
				{
					if ( (start < dot) && (end < dot) )	/* security mechanism ;) */
					{
						long FilenameLen =(long)(end - start - 1);
						long NodenameLen =(long)(dot - end - 1);
						if (!hyperobjectcount)
							hyperobjects = xmalloc(sizeof(HyperObject));
						else
						{
							hyperobjects = xrealloc(hyperobjects,
									sizeof(HyperObject) *(hyperobjectcount + 1));
						}
						strncpy(hyperobjects[hyperobjectcount].file,
								start + 1, FilenameLen);
						hyperobjects[hyperobjectcount].file[FilenameLen] = 0;
						strncpy(hyperobjects[hyperobjectcount].node,
								end + 1, NodenameLen);
						hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
						hyperobjects[hyperobjectcount].type = 3;
						if (start < buf + strlen(line1))
						{
							hyperobjects[hyperobjectcount].line = line;
							hyperobjects[hyperobjectcount].col =
								calculate_len(buf, start);
							if (dot < buf + strlen(line1))
							{
								/* if the note highlight fits in first line
								 * we don't need to break highlighting into
								 * several lines */
								hyperobjects[hyperobjectcount].breakpos = -1;
							}
							else
							{
								/* otherwise we need it */
								hyperobjects[hyperobjectcount].breakpos =
									strlen(line1) -(long)(start - buf);
							}
						}
						else
						{
							hyperobjects[hyperobjectcount].line = line + 1;
							hyperobjects[hyperobjectcount].col =
								calculate_len(buf + strlen(line1), start);
							hyperobjects[hyperobjectcount].breakpos = -1;
						}
						hyperobjects[hyperobjectcount].nodelen =
							strlen(hyperobjects[hyperobjectcount].node);
						hyperobjects[hyperobjectcount].filelen =
							strlen(hyperobjects[hyperobjectcount].file);
						hyperobjectcount++;
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
						int goodHit = 0;
						if (!hyperobjectcount)
							hyperobjects = xmalloc(sizeof(HyperObject));
						else
						{
							hyperobjects = xrealloc(hyperobjects,
									sizeof(HyperObject) *(hyperobjectcount + 1));
						}

						start = tmp + 1;	/* move after the padding spaces */
						while (isspace(*start))
							start++;
						NodenameLen =(long)(dot - start);
						strcpy(hyperobjects[hyperobjectcount].file, "");
						strncpy(hyperobjects[hyperobjectcount].node,
								start, NodenameLen);
						hyperobjects[hyperobjectcount].node[NodenameLen] = 0;
						hyperobjects[hyperobjectcount].type = 3;
						hyperobjects[hyperobjectcount].nodelen =
							strlen(hyperobjects[hyperobjectcount].node);
						hyperobjects[hyperobjectcount].filelen =
							strlen(hyperobjects[hyperobjectcount].file);
						if (start < buf + strlen(line1))
						{
							hyperobjects[hyperobjectcount].line = line;
							hyperobjects[hyperobjectcount].col =
								calculate_len(buf, start);
							if (dot < buf + strlen(line1))
							{
								/* if the note highlight fits in first line
								 * we don't need to break highlighting into
								 * several lines */
								hyperobjects[hyperobjectcount].breakpos = -1;
							}
							else
							{
								/* otherwise we need it */
								hyperobjects[hyperobjectcount].breakpos =
									strlen(line1) -(long)(start - buf);
							}
						}
						else
						{
							hyperobjects[hyperobjectcount].line = line + 1;
							hyperobjects[hyperobjectcount].col =
								calculate_len(strlen(line1) + buf, start);
							hyperobjects[hyperobjectcount].breakpos = -1;
						}
						if (exists_in_tag_table(hyperobjects[hyperobjectcount].node))
						{
							hyperobjectcount++;	/* yep, this was a good hit */
							goodHit = 1;
						}
						if (!goodHit && !hyperobjectcount)
						{
							xfree(hyperobjects);
							hyperobjects = 0;
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
	urlend = line1;
	while ((urlstart = strstr(urlend, "http://")) != NULL)
	{
		urlend = findurlend(urlstart);	/* always successful */
		if (!hyperobjectcount)
			hyperobjects = xmalloc(sizeof(HyperObject));
		else
		{
			hyperobjects = xrealloc(hyperobjects,
					sizeof(HyperObject) *(hyperobjectcount + 1));
		}
		hyperobjects[hyperobjectcount].line = line;
		hyperobjects[hyperobjectcount].col = calculate_len(line1, urlstart);
		hyperobjects[hyperobjectcount].breakpos = -1;
		hyperobjects[hyperobjectcount].type = 4;
		strncpy(hyperobjects[hyperobjectcount].node, urlstart, urlend - urlstart);
		hyperobjects[hyperobjectcount].node[urlend - urlstart] = 0;
		strcpy(hyperobjects[hyperobjectcount].file, "");
		hyperobjects[hyperobjectcount].tagtableoffset = -1;
		hyperobjects[hyperobjectcount].nodelen =
			strlen(hyperobjects[hyperobjectcount].node);
		hyperobjects[hyperobjectcount].filelen =
			strlen(hyperobjects[hyperobjectcount].file);
		hyperobjectcount++;
	}
	/* ftp:// */
	urlend = line1;
	while ((urlstart = strstr(urlend, "ftp://")) != NULL)
	{
		urlend = findurlend(urlstart);	/* always successful */
		if (!hyperobjectcount)
			hyperobjects = xmalloc(sizeof(HyperObject));
		else
		{
			hyperobjects = xrealloc(hyperobjects,
					sizeof(HyperObject) *(hyperobjectcount + 1));
		}
		hyperobjects[hyperobjectcount].line = line;
		hyperobjects[hyperobjectcount].col = calculate_len(line1, urlstart);
		hyperobjects[hyperobjectcount].breakpos = -1;
		hyperobjects[hyperobjectcount].type = 5;
		strncpy(hyperobjects[hyperobjectcount].node, urlstart, urlend - urlstart);
		hyperobjects[hyperobjectcount].node[urlend - urlstart] = 0;
		strcpy(hyperobjects[hyperobjectcount].file, "");
		hyperobjects[hyperobjectcount].tagtableoffset = -1;
		hyperobjects[hyperobjectcount].nodelen =
			strlen(hyperobjects[hyperobjectcount].node);
		hyperobjects[hyperobjectcount].filelen =
			strlen(hyperobjects[hyperobjectcount].file);
		hyperobjectcount++;
	}
	if (initialhyperobjectcount != hyperobjectcount)
		sort_hyperlinks_from_current_line(initialhyperobjectcount, hyperobjectcount);
	if (buf)
	{
		xfree(buf);
		buf = 0;
	}
}
