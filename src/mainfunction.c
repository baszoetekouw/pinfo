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

#ifndef MIN
#define        MIN(a,b)(((a)<(b))?(a):(b))
#endif

void rescan_cursor();	/* set the cursor to 1st item on visible screen */
void next_infomenu();	/* go to the next menu item for sequential reading */
int getnodeoffset(int tag_table_pos, int *Indstart);	/* get node offset in file */

int aftersearch = 0;
/*
 * this flag is turned on when the engine receives a simulated `key.back',
 * caused by the sequential auto-pgdn reading code
 */
int toggled_by_menu = 0;
unsigned pos;
long cursor;
long infomenu, infocolumn=0;

/* strdup src string, but when doing so, strip leading/trailing spaces and
 * substitute multiple spaces with a single space. This is needed when *Node
 * links span multiple lines and the margin is embedded in the string */
static char *
canonical_strdup(const char *src)
{
	char *dst;
	int dstlen, srclen, copylen;

	while (*src == ' ')
		++src;
	for (dstlen = 0, srclen = 0; src[srclen]; ++srclen)
	{
		if (src[srclen] != ' ' || (src[srclen + 1] != 0 && src[srclen + 1] != ' '))
			++dstlen;
	}
	dst = xmalloc(dstlen + 1);
	for (copylen = 0, srclen = 0; copylen < dstlen; ++srclen)
	{
		if (src[srclen] != ' ' || src[srclen + 1] != ' ')
			dst[copylen++] = src[srclen];
	}
	dst[dstlen] = 0;
	return dst;
}

WorkRVal
work(char ***message, char **type, unsigned long *lines, FILE * id, int tag_table_pos)
{
#define Message	(*message)
#define Lines	(*lines)
#define Type	(*type)
	static WorkRVal rval =
	{0, 0};
	FILE *mypipe;
	int fileoffset;
	int indirectstart = -1;
	int cursorchanged = 0;
	int key = 0;
	int return_value;
	int statusline = FREE;
	char *token;
	char *tmp = NULL;
	if (rval.file)		/* if the static variable was allocated, free it */
	{
		xfree(rval.file);
		rval.file = 0;
	}
	if (rval.node)
	{
		xfree(rval.node);
		rval.node = 0;
	}

	pos = 1, cursor = 0, infomenu = -1;	/* default position, and selected number */

#ifdef getmaxyx
	getmaxyx(stdscr, maxy, maxx);	/* initialize maxx, maxy */
#else
	maxx = 80;
	maxy = 25;
#endif /*  getmaxyx */
	/* free memory allocated previously by hypertext links */
	freelinks();
	for (unsigned long i = 1; i < Lines; i++)	/* initialize node-links for every line */
	{
		initializelinks(Message[i], Message[i + 1], i);
	}
	initializelinks(Message[Lines], "", Lines);

	next_infomenu();		/* infomenu will remain -1 if it's the last pos, or if there's no menu item */

	if (npos != -1)
		pos = npos;			/* set eventual history pos */

	/* if we're in a node found using 's'earch function. */
	if (aftersearch)
	{
		pos = aftersearch;	/* set pos to the found position */
		/*      aftersearch=0;  * don't reset this--we want to know if we mus highlight something */
	}

	if (ncursor != -1)
	{
		cursor = ncursor;		/* set eventual cursor pos  */
		infomenu = nmenu;		/* same with last sequential reading menu pos */
	}
	else
	{
		rescan_cursor();		/* scan for cursor position */
	}
	if (toggled_by_menu)		/* this node will not be shown to the user--it shouldn't go to history */
		dellastinfohistory();	/* delete the history entry for this node--it's not even seen by the user */
	npos = -1;			/* turn off the `next-time' pos/cursor modifiers */
	ncursor = -1;
	nmenu = -1;
	addtopline(Type,infocolumn);
	while (1)
	{
		/*
		 * read key, and show screen only if there is nothing in the input
		 * buffer.  Otherwise the scrolling would be too slow.
		 */
		nodelay(stdscr, TRUE);
		key = pinfo_getch();
		if (key == ERR)
		{
			if (statusline == FREE)
				showscreen(Message, Lines, pos, cursor,infocolumn);
			waitforgetch();
			key = pinfo_getch();
		}
		nodelay(stdscr, FALSE);
		statusline = FREE;
		if (winchanged)		/* SIGWINCH */
		{
			handlewinch();
			winchanged = 0;
			addtopline(Type,infocolumn);
			key = pinfo_getch();
		}
		/***************************** keyboard handling ****************************/
		if (key != 0)
		{
			if ((key == keys.print_1) ||
					(key == keys.print_2))
			{
				if (yesno(_("Are you sure you want to print?"), 0) == 1)
					printnode(message, lines);
			}
			/*==========================================================================*/
			if ((key == keys.pgdn_auto_1) ||
					(key == keys.pgdn_auto_2) ||
					(toggled_by_menu))
			{
				int wastoggled = toggled_by_menu;
				toggled_by_menu = 0;
				/* if hyperobject type <= 1, then we have a menu */
				if ((pos >= Lines -(maxy - 2)) ||(wastoggled))
				{
					if ((infomenu != -1) &&(!wastoggled))
					{
						cursor = infomenu;
						key = keys.followlink_1;	/* the handler for keys.followlink must be bellow this statement! */
					}
					else
						/* we shouldn't select a menu item if this node is called via `up:' from bottom, or if there is no menu */
					{
						char *typestr = strdup(Type);
						getnextnode(Type, typestr);
						if (strcmp(typestr, ERRNODE) != 0)
						{
							key = keys.nextnode_1;
						}
						else
						{
							getnodename(Type, typestr);
							if (strcmp(FirstNodeName, typestr) != 0)	/* if it's not end of all menus */
							{
								if (wastoggled)	/* if we're in the temporary called up node */
									toggled_by_menu = KILL_HISTORY;
								else	/* if we are calling the up node from non-temporary bottom node */
									toggled_by_menu = KEEP_HISTORY;
								key = keys.upnode_1;
								ungetch(KEY_NOTHING);
							}
						}	/* end: else if nextnode==ERRNODE */
					}		/* end: if we shouldn't select a menu item */
				}		/* end: if position is right */
			}
			/*==========================================================================*/
			if ((key == keys.goline_1) ||
					(key == keys.goline_2))
			{
				unsigned long newpos;
				attrset(bottomline);	/* read user's value */
				move(maxy - 1, 0);
				echo();
				curs_set(1);
				token = getstring(_("Enter line: "));
				curs_set(0);
				noecho();
				move(maxy - 1, 0);
				myclrtoeol();
				attrset(normal);
				if (token)	/*
							 * convert string to long.
							 * careful with nondigit strings.
							 */
				{
					int digit_val = 1;
					for (unsigned i = 0; token[i] != 0; i++)
					{
						if (!isdigit(token[i]))
							digit_val = 0;
					}
					if (digit_val)	/* go to specified line */
					{
						newpos = atol(token);
						newpos -=(maxy - 1);
						if ((newpos > 0) &&(newpos < Lines -(maxy - 2)))
							pos = newpos;
						else if ((newpos > 0) &&((Lines -(maxy - 2)) > 0))
							pos = Lines -(maxy - 2);
						else
							pos = 1;
					}
					xfree(token);
					token = 0;
				}
			}
			/*==========================================================================*/
			if ((key == keys.shellfeed_1) ||
					(key == keys.shellfeed_2))
			{
				/* get command name */
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				curs_set(1);
				token = getstring(_("Enter command: "));
				noecho();
				move(maxy - 1, 0);
				myclrtoeol();
				attrset(normal);

				myendwin();
				xsystem("clear");
				mypipe = popen(token, "w");	/* open mypipe */
				if (mypipe != NULL)
				{
					for (unsigned long i = 1; i <= Lines; i++)	/* and flush the msg to stdin */
						fprintf(mypipe, "%s", Message[i]);
					pclose(mypipe);
					getchar();
				}
				doupdate();
				curs_set(0);
				if (mypipe == NULL)
					mvaddstr(maxy - 1, 0, _("Operation failed..."));
				xfree(token);
				token = 0;
			}
			/*==========================================================================*/
			if ((key == keys.dirpage_1) ||
					(key == keys.dirpage_2))
			{
				rval.file = malloc(10);
				strcpy(rval.file, "dir");
				rval.node = malloc(2);
				strcpy(rval.node, "");
				aftersearch = 0;
				return rval;
			}
			/*==========================================================================*/
			if ((key == keys.refresh_1) ||
					(key == keys.refresh_2))
			{
				myendwin();
				doupdate();
				refresh();
				curs_set(0);
			}
			/*==========================================================================*/
			if ((key == keys.totalsearch_1) ||	/* search in all nodes later than this one */
					(key == keys.totalsearch_2))
			{
				int tmpaftersearch = aftersearch;
				indirectstart = -1;
				move(maxy - 1, 0);
				attrset(bottomline);
				echo();
				curs_set(1);
				if (!searchagain.search)	/* if searchagain key wasn't hit */
				{
					token = getstring(_("Enter regular expression: "));	/* get the token */
					strcpy(searchagain.lastsearch, token);	/* and save it to searchagain buffer */
					/*
					 * give a hint, which key to ungetch to call this procedure
					 * by searchagain
					 */
					searchagain.type = key;
				}
				else /* it IS searchagain */
				{
					token = xmalloc(strlen(searchagain.lastsearch) + 1);
					/* allocate space for token */
					strcpy(token, searchagain.lastsearch);
					/* copy the token from searchagain buffer */
					searchagain.search = 0;
					/* reset the searchagain swith(until it's set again
					   by the keys.searchagain key handler) */
				}
				if (strlen(token) == 0)
				{
					xfree(token);
					goto skip_search;
				}
				curs_set(0);
				noecho();
				attrset(normal);

				/* Calculate current info file offset...  */
				fileoffset = 0;
				for (unsigned i = 1; i <= pos + 1; i++)	/* count the length of curnode */
					fileoffset += strlen(Message[i]);
				fileoffset += strlen(Type);	/* add also header length */

				fileoffset += getnodeoffset(tag_table_pos, &indirectstart);	/* also load the variable indirectstart */

				/* Searching part...  */
				aftersearch = 0;

				/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
				return_value = -1;
				if (indirect)	/* the info is of indirect type; we'll search through several files */
				{
					FILE *fd = NULL;
					long tokenpos;
					long starttokenpos;
					long filelen;
					for (int j = indirectstart; j <= (int) IndirectEntries; j++)
					{
						fd = openinfo(indirect[j].filename, 1);	/* get file
																	 * length. */
						fseek(fd, 0, SEEK_END);
						filelen = ftell(fd);

						/*
						 * seek to the beginning of search area. At the first
						 * time it is `fileoffset', then it is the first node's
						 * offset
						 */
						if (j == indirectstart)

							fseek(fd, fileoffset, SEEK_SET);
						else
							fseek(fd, FirstNodeOffset, SEEK_SET);
						starttokenpos = ftell(fd);

						tmp = xmalloc(filelen - starttokenpos + 10);	/* read data */
						size_t num_read = fread(tmp, 1, filelen - starttokenpos, fd);
						tmp[num_read] = 0;

						tokenpos = regexp_search(token, tmp);	/* search */

						if (tokenpos != -1)	/* if something was found */
						{
							/*
							 * add the offset of the part of file, which wasn't
							 * read to the memory
							 */
							tokenpos += starttokenpos;
							{	/* local scope for tmpvar */
								int tmpvar = 0;
								tag_table[0].offset = 0;
								for (unsigned i = TagTableEntries; i >= 1; i--)
								{
									if ((tag_table[i].offset > tag_table[tmpvar].offset) &&
											((tag_table[i].offset - indirect[j].offset + FirstNodeOffset) <= tokenpos))
									{
										return_value = i;
										tmpvar = i;
									}
								}
							}
							/* this means, that indirect entry was found.  */
							if (return_value != -1)
							{
								fseek(fd, tag_table[return_value].offset - indirect[j].offset + FirstNodeOffset, SEEK_SET);
								/* seek to the found node offset */
								while (fgetc(fd) != INFO_TAG);
								fgetc(fd);	/* skip newline */

								aftersearch = 1;

								/*
								 * count, how many lines stands befor the token
								 * line.
								 */
								while (ftell(fd) < tokenpos)
								{
									int chr = fgetc(fd);
									if (chr == '\n')
										aftersearch++;
									else if (chr == EOF)
										break;
								}
								/*
								 * the number ofline where a token is found, is
								 * now in the variable `aftersearch'
								 */
								if (aftersearch > 1)
									aftersearch--;
								else
									aftersearch = 1;
							}	/* end: if (indirect entry was found) */
							if (aftersearch)	/* if something was found */
							{
								if (tmp)	/* free tmp buffer */
								{
									xfree(tmp);
									tmp = 0;
								}
								break;
							}
						}	/* end: if (tokenpos) */
					}		/* end: indirect file loop */
					if (tmp)	/* free tmp buffer */
					{
						xfree(tmp);
						tmp = 0;
					}
					fclose(fd);
				}		/* end: if (indirect) */
				else /* if not indirect */
				/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
				{
					long filelen;
					long filepos = ftell(id);
					long tokenpos;
					long starttokenpos;

					fseek(id, 0, SEEK_END);	/* calculate filelength */
					filelen = ftell(id);

					/* seek at the start of search area. */
					fseek(id, fileoffset, SEEK_SET);

					/* remember the number of skipped bytes.*/
					starttokenpos = ftell(id);

					/* read data */
					tmp = xmalloc(filelen - starttokenpos + 10);
					size_t num_read = fread(tmp, 1, filelen - starttokenpos, id);
					tmp[num_read] = 0;

					/* search */
					tokenpos = regexp_search(token, tmp);

					if (tokenpos != -1)	/* if we've found something */
					{
						/*
						 * add offset of the start of search area to this token
						 * position.
						 */
						tokenpos += starttokenpos;
						{		/* local scope for tmpvar */
							int tmpvar = 0;
							tag_table[0].offset = 0;
							for (unsigned i = TagTableEntries; i >= 1; i--)
							{
								if ((tag_table[i].offset > tag_table[tmpvar].offset) &&
										(tag_table[i].offset <= tokenpos))
								{
									return_value = i;
									tmpvar = i;
								}
							}
						}
						/*
						 * this means, that we've found our entry, and we're
						 * one position too far with the `i' counter.
						 */
						if (return_value != -1)
						{
							fseek(id, tag_table[return_value].offset, SEEK_SET);
							/* seek to the node, which holds found line */
							while (fgetc(id) != INFO_TAG);
							fgetc(id);	/* skip newline */

							aftersearch = 1;
							/* count lines in found node, until found line is
							 * met. */
							while (ftell(id) < tokenpos)
							{
								int chr = fgetc(id);
								if (chr == '\n')
									aftersearch++;
								else if (chr == EOF)
									break;
							}
							if (aftersearch > 1)
								aftersearch--;
							else
								aftersearch = 1;
							fseek(id, filepos, SEEK_SET);	/* seek to old
															 * filepos. */
						}
					}		/* end: if (tokenpos) <--> token found */
					if (tmp)	/* free tmp buffer */
					{
						xfree(tmp);
						tmp = 0;
					}
				}		/* end: if (!indirect) */
				/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
				xfree(token);
				token = 0;

				if (!aftersearch)
				{
					attrset(bottomline);
					mymvhline(maxy - 1, 0, ' ', maxx);
					move(maxy - 1, 0);
					mvaddstr(maxy - 1, 0, _("Search string not found..."));
					statusline = LOCKED;
				}

				if (!aftersearch)
					aftersearch = tmpaftersearch;

				if (return_value != -1)
				{
					infohistory.pos[infohistory.length] = pos;
					infohistory.cursor[infohistory.length] = cursor;
					infohistory.menu[infohistory.length] = infomenu;
					rval.node = canonical_strdup(tag_table[return_value].nodename);
					rval.file = xmalloc(1);
					rval.file[0] = 0;
					return rval;
				}
			}			/* end: if key_totalsearch */
			/*==========================================================================*/
			if ((key == keys.search_1) ||		/* search in current node */
					(key == keys.search_2))
			{
				int success = 0;
				move(maxy - 1, 0);
				attrset(bottomline);
				echo();
				curs_set(1);
				if (!searchagain.search)	/* searchagain handler. see totalsearch */
				{
					token = getstring(_("Enter regular expression: "));
					strcpy(searchagain.lastsearch, token);
					searchagain.type = key;
				}
				else
				{
					token = xmalloc(strlen(searchagain.lastsearch) + 1);
					strcpy(token, searchagain.lastsearch);
					searchagain.search = 0;
				}		/* end of searchagain handler */
				if (strlen(token) == 0)
				{
					xfree(token);
					goto skip_search;
				}
				curs_set(0);
				noecho();
				attrset(normal);
				/* compile the read token */
				if (pinfo_re_comp(token) != 0)
				{
					/* we're not in a search! */
					aftersearch = 0;
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
				/* scan for the token in the following lines.  */
				for (unsigned long i = pos + 1; i < Lines; i++)
				{
					tmp = xmalloc(strlen(Message[i]) + strlen(Message[i + 1]) + 2);
					/*
					 * glue two following lines into one -- to find matches
					 * split up into two lines.
					 */
					strcpy(tmp, Message[i]);
					strcat(tmp, Message[i + 1]);
					if (pinfo_re_exec(tmp))	/* execute the search command */
					{		/* if found, enter here */
						success = 1;
						/* if token was found in the second line, make pos=i+1.  */
						if (pinfo_re_exec(Message[i + 1]))
							pos = i + 1;
						else /* othwerwise, pos=i. This happens when we havesplit expression. */
							pos = i;
						xfree(tmp);	/* free tmp buffer */
						tmp = 0;
						aftersearch = 1;
						break;
					}
					else /* nothing found */
					{
						xfree(tmp);	/* free tmp buffer */
						tmp = 0;
					}
				}
				if (!success)
				{
					attrset(bottomline);
					mymvhline(maxy - 1, 0, ' ', maxx);
					move(maxy - 1, 0);
					mvaddstr(maxy - 1, 0, _("Search string not found..."));
					statusline = LOCKED;
				}
				xfree(token);	/* free user's search token */
				token = 0;
				rescan_cursor();	/* rescan cursor position in the new place */
			}
skip_search:
			/*==========================================================================*/
			if ((key == keys.search_again_1) ||	/* search again */
					(key == keys.search_again_2))
			{
				if (searchagain.type != 0)	/* if a search was made before */
				{
					searchagain.search = 1;	/* mark, that search routines should *
											 * use the searchagain token value   */
					ungetch(searchagain.type);	/* ungetch the proper *
												 * search key         */
				}
			}
			/*==========================================================================*/

			if ((key == keys.goto_1) ||	/* goto node */
					(key == keys.goto_2))
			{
				return_value = -1;
				move(maxy - 1, 0);
				attrset(bottomline);
				curs_set(1);

				const char ** tag_completions = completions_from_tag_table(tag_table, TagTableEntries);
				token = getstring_with_completion(_("Enter node name: "), tag_completions);	/* read user's wish */
				if (tag_completions!=NULL)
				{
					free(tag_completions);
				}

				curs_set(0);
				noecho();
				attrset(normal);
				for (unsigned i = 1; i <= TagTableEntries; i++)
				{
					/* if the name was found in the tag table */
					if (strcmp(token, tag_table[i].nodename) == 0)
					{
						return_value = i;
						break;
					}
				}
				if (return_value != -1)	/* if the name was in tag table */
				{
					xfree(token);
					token = 0;
					infohistory.pos[infohistory.length] = pos;
					infohistory.cursor[infohistory.length] = cursor;
					infohistory.menu[infohistory.length] = infomenu;
					rval.node = canonical_strdup(tag_table[return_value].nodename);
					rval.file = xmalloc(1);
					rval.file[0] = 0;
					aftersearch = 0;
					return rval;
				}
				else
					/* if the name wasn't in tag table */
				{
					/*
					 * scan for filename: filenames may be specified in format:
					 * (file)node
					 */
					char *gotostartptr = strchr(token, '(');
					if (gotostartptr)	/* if there was a `(' */
					{
						char *gotoendptr = strchr(token, ')');	/* search for `)' */
						/* if they're in the right order...  */
						if (gotoendptr > gotostartptr)
						{
							rval.file = xmalloc(gotoendptr - gotostartptr + 1);
							strncpy(rval.file, gotostartptr + 1, gotoendptr - gotostartptr - 1);
							rval.file[gotoendptr - gotostartptr - 1] = 0;
							gotoendptr++;
							while (gotoendptr)	/* skip whitespaces until nodename */
							{
								if (*gotoendptr != ' ')
									break;
								gotoendptr++;
							}	/* skip spaces */
							rval.node = canonical_strdup(gotoendptr);
							xfree(token);
							token = 0;
							aftersearch = 0;
							return rval;
						}
					}
					/* handle the `file.info' format of crossinfo goto. */
					else if (strstr(token, ".info"))
					{
						rval.file = xmalloc(strlen(token) + 1);
						strcpy(rval.file, token);
						xfree(token);
						token = 0;
						rval.node = xmalloc(5);
						strcpy(rval.node, "");
						aftersearch = 0;
						return rval;
					}
					else /* node not found */
					{
						attrset(bottomline);
						mymvhline(maxy - 1, 0, ' ', maxx);
						move(maxy - 1, 0);
						printw(_("Node %s not found"), token);
						attrset(normal);
						move(0, 0);
					}
				}
				statusline = LOCKED;
				xfree(token);
				token = 0;
			}
			/*==========================================================================*/
			if ((key == keys.prevnode_1) ||	/* goto previous node */
					(key == keys.prevnode_2))
			{
				token = xmalloc(strlen(Type));
				getprevnode(Type, token);
				return_value = gettagtablepos(token);
				xfree(token);
				token = 0;
				if (return_value != -1)
				{
					infohistory.pos[infohistory.length] = pos;
					infohistory.cursor[infohistory.length] = cursor;
					infohistory.menu[infohistory.length] = infomenu;
					rval.node = canonical_strdup(tag_table[return_value].nodename);
					rval.file = xmalloc(1);
					rval.file[0] = 0;
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.nextnode_1) ||	/* goto next node */
					(key == keys.nextnode_2))
			{
				token = xmalloc(strlen(Type));
				getnextnode(Type, token);
				return_value = gettagtablepos(token);
				xfree(token);
				token = 0;
				if (return_value != -1)
				{
					infohistory.pos[infohistory.length] = pos;
					infohistory.cursor[infohistory.length] = cursor;
					infohistory.menu[infohistory.length] = infomenu;
					rval.node = canonical_strdup(tag_table[return_value].nodename);
					rval.file = xmalloc(1);
					rval.file[0] = 0;
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.upnode_1) ||		/* goto up node */
					(key == keys.upnode_2))
			{
				token = xmalloc(strlen(Type));
				getupnode(Type, token);
				if (strncmp(token, "(dir)", 5) == 0)
				{
					ungetch(keys.dirpage_1);
				}
				return_value = gettagtablepos(token);
				xfree(token);
				token = 0;
				if (return_value != -1)
				{
					if (toggled_by_menu == KEEP_HISTORY)
					{
						infohistory.pos[infohistory.length] = pos;
						infohistory.cursor[infohistory.length] = cursor;
						infohistory.menu[infohistory.length] = infomenu;
					}
					rval.node = canonical_strdup(tag_table[return_value].nodename);
					rval.file = xmalloc(1);
					rval.file[0] = 0;
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.twoup_1) || (key == keys.twoup_2))
			{
				ungetch(keys.up_1);
				ungetch(keys.up_1);
			}
			/*==========================================================================*/
			if ((key == keys.up_1) ||
					(key == keys.up_2))
			{
				cursorchanged = 0;
				if (cursor != -1)	/* if we must handle cursor... */
				{
					if ((cursor > 0) &&(hyperobjectcount))	/* if we really must handle it ;) */
						/*
						 * look if there's a cursor(link) pos available above,
						 * and if it is visible now.
						 */
						for (int i = cursor - 1; i >= 0; i--)
						{
							if ((hyperobjects[i].line >= pos) &&
									(hyperobjects[i].line < pos +(maxy - 1)))
							{
								/* don't play with `highlight' objects */
								if (hyperobjects[i].type < HIGHLIGHT)
								{
									cursor = i;
									cursorchanged = 1;
									break;
								}
							}
						}
				}
				if (!cursorchanged)	/* if the cursor wasn't changed */
				{
					if (pos > 1)	/* lower the nodepos */
						pos--;
					/* and scan for a hyperlink in the new line */
					for (unsigned long i = 0; i < hyperobjectcount; i++)
					{
						if (hyperobjects[i].line == pos)
						{
							if (hyperobjects[i].type < HIGHLIGHT)
							{
								cursor = i;
								break;
							}
						}
					}
				}
			}
			/*==========================================================================*/
			if ((key == keys.end_1) ||
					(key == keys.end_2))
			{
				pos = Lines -(maxy - 2);
				if (pos < 1)
					pos = 1;
				cursor = hyperobjectcount - 1;
			}
			/*==========================================================================*/
			if ((key == keys.pgdn_1) ||
					(key == keys.pgdn_2))
			{
				if (pos +(maxy - 2) < Lines -(maxy - 2))
				{
					pos +=(maxy - 2);
					rescan_cursor();
				}
				else if (Lines -(maxy - 2) >= 1)
				{
					pos = Lines -(maxy - 2);
					cursor = hyperobjectcount - 1;
				}
				else
				{
					pos = 1;
					cursor = hyperobjectcount - 1;
				}
			}
			/*==========================================================================*/
			if ((key == keys.home_1) ||
					(key == keys.home_2))
			{
				pos = 1;
				rescan_cursor();
			}
			/*==========================================================================*/
			if ((key == keys.pgup_1) |
					(key == keys.pgup_2))
			{
				if (pos >(maxy - 2))
					pos -=(maxy - 2);
				else
					pos = 1;
				rescan_cursor();
			}
			/*==========================================================================*/
			if ((key == keys.pgup_auto_1) ||
					(key == keys.pgup_auto_2))
			{
				if (pos == 1)
					ungetch(keys.upnode_1);
			}
			/*==========================================================================*/
			if ((key == keys.twodown_1) ||
					(key == keys.twodown_2))	/* top+bottom line \|/ */
			{
				ungetch(keys.down_1);
				ungetch(keys.down_1);
			}
			/*==========================================================================*/
			if ((key == keys.down_1) ||
					(key == keys.down_2))	/* top+bottom line \|/ */
			{
				cursorchanged = 0;	/* works similar to keys.up */
				if (cursor < 0) cursor = -1;
				if (cursor < 0 || (unsigned long) cursor < hyperobjectcount)
				{
					for (unsigned long i = (unsigned long) (cursor + 1); i < hyperobjectcount; i++)
					{
						if ((hyperobjects[i].line >= pos) &&
								(hyperobjects[i].line < pos +(maxy - 2)))
						{
							if (hyperobjects[i].type < HIGHLIGHT)
							{
								cursor = i;
								cursorchanged = 1;
								break;
							}
						}
					}
				}
				if (!cursorchanged)
				{
					if (pos <= Lines -(maxy - 2))
						pos++;
					for (unsigned long i = cursor + 1; i < hyperobjectcount; i++)
					{
						if ((hyperobjects[i].line >= pos) &&
								(hyperobjects[i].line < pos +(maxy - 2)))
						{
							if (hyperobjects[i].type < HIGHLIGHT)
							{
								cursor = i;
								cursorchanged = 1;
								break;
							}
						}
					}
				}
			}
			/*==========================================================================*/
			if ((key == keys.top_1) ||
					(key == keys.top_2))
			{
				infohistory.pos[infohistory.length] = pos;
				infohistory.cursor[infohistory.length] = cursor;
				infohistory.menu[infohistory.length] = infomenu;
				rval.node = canonical_strdup(FirstNodeName);
				rval.file = xmalloc(1);
				rval.file[0] = 0;
				aftersearch = 0;
				return rval;
			}
			/*==========================================================================*/
			if ((key == keys.back_1) ||
					(key == keys.back_2))
			{
				if (infohistory.length > 1)
				{
					dellastinfohistory();	/* remove history entry for this node */
					/* now we deal with the previous node history entry */

					rval.node = canonical_strdup(infohistory.node[infohistory.length]);
					rval.file = xmalloc(strlen(infohistory.file[infohistory.length]) + 1);
					strcpy(rval.file, infohistory.file[infohistory.length]);

					npos = infohistory.pos[infohistory.length];
					ncursor = infohistory.cursor[infohistory.length];
					nmenu = infohistory.menu[infohistory.length];
					dellastinfohistory();	/* remove history entry for previous node */
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.followlink_1) ||
					(key == keys.followlink_2))
			{
				infohistory.pos[infohistory.length] = pos;
				infohistory.cursor[infohistory.length] = cursor;
				infohistory.menu[infohistory.length] = infomenu;
				if (!toggled_by_menu)
					infohistory.menu[infohistory.length] = cursor;
				if ((cursor >= 0) && ((unsigned long) cursor < hyperobjectcount))
					if (
						( (hyperobjects[cursor].line >= pos)
						  && (hyperobjects[cursor].line < pos +(maxy - 2) )
						) || (toggled_by_menu) )
					{
						toggled_by_menu = 0;
						if (hyperobjects[cursor].type < 4)	/* normal info link */
						{
							rval.node = canonical_strdup(hyperobjects[cursor].node);
							rval.file = xmalloc(strlen(hyperobjects[cursor].file) + 1);
							strcpy(rval.file, hyperobjects[cursor].file);
							aftersearch = 0;
							return rval;
						}
						else if (hyperobjects[cursor].type < HIGHLIGHT)	/* we deal with an url */
						{
							if (hyperobjects[cursor].type == 4)	/* http */
							{
								char *tempbuf = xmalloc(strlen(hyperobjects[cursor].node) + strlen(httpviewer) + 10);
								strcpy(tempbuf, httpviewer);
								strcat(tempbuf, " ");
								strcat(tempbuf, hyperobjects[cursor].node);
								myendwin();
								xsystem(tempbuf);
								doupdate();
								xfree(tempbuf);
							}
							else if (hyperobjects[cursor].type == 5)	/* ftp */
							{
								char *tempbuf = xmalloc(strlen(hyperobjects[cursor].node) + strlen(ftpviewer) + 10);
								strcpy(tempbuf, ftpviewer);
								strcat(tempbuf, " ");
								strcat(tempbuf, hyperobjects[cursor].node);
								myendwin();
								xsystem(tempbuf);
								doupdate();
								xfree(tempbuf);
							}
							else if (hyperobjects[cursor].type == 6)	/* mail */
							{
								char *tempbuf = xmalloc(strlen(hyperobjects[cursor].node) + strlen(maileditor) + 10);
								strcpy(tempbuf, maileditor);
								strcat(tempbuf, " ");
								strcat(tempbuf, hyperobjects[cursor].node);
								myendwin();
								xsystem("clear");
								xsystem(tempbuf);
								doupdate();
								xfree(tempbuf);
							}
						}
					}
			}
			/*==========================================================================*/
			if ((key == keys.left_1) ||(key == keys.left_2))
			{
				if (infocolumn>0) infocolumn--;
				addtopline(Type,infocolumn);
			}
			/*==========================================================================*/
			if ((key == keys.right_1) ||(key == keys.right_2))
			{
				infocolumn++;
				addtopline(Type,infocolumn);
			}
			/*==========================================================================*/
			/**************************** end of keyboard handling **********************/
			/******************************** mouse handler *****************************/
#ifdef CURSES_MOUSE
			if (key == KEY_MOUSE)
			{
				MEVENT mouse;
				int done = 0;

				getmouse(&mouse);
				if (mouse.x<0 || mouse.y<0) /* should never happen, according to curses docs */
					continue;

				/* copy to unsigned vars to avoid all kinds of signed/unsigned comparison unpleasantness below */
				unsigned mouse_x = mouse.x;
				unsigned mouse_y = mouse.x;

				if (mouse.bstate == BUTTON1_CLICKED)
				{
					if ((mouse_y > 0) &&(mouse_y < maxy - 1))
					{
						for (int i = cursor; i > 0; i--)
						{
							if (hyperobjects[i].line == mouse_y + pos - 1)
							{
								if (hyperobjects[i].col <= mouse_x - 1)
								{
									if (hyperobjects[i].col + strlen(hyperobjects[i].node) + strlen(hyperobjects[i].file) >= mouse_x - 1)
									{
										if (hyperobjects[i].type < HIGHLIGHT)
										{
											cursor = i;
											done = 1;
											break;
										}
									}
								}
							}
						}
						if (!done)
							for (unsigned i = cursor; i < hyperobjectcount; i++)
							{
								if (hyperobjects[i].line == mouse_y + pos - 1)
								{
									if (hyperobjects[i].col <= mouse_x - 1)
									{
										if (hyperobjects[i].col + strlen(hyperobjects[i].node) + strlen(hyperobjects[i].file) >= mouse_x - 1)
										{
											if (hyperobjects[i].type < HIGHLIGHT)
											{
												cursor = i;
												done = 1;
												break;
											}
										}
									}
								}
							}
					}		/* end: if (mouse_y not on top/bottom line) */
					else if (mouse_y == 0)
						ungetch(keys.up_1);
					else if (mouse_y == maxy - 1)
						ungetch(keys.down_1);
				}		/* end: button clicked */
				if (mouse.bstate == BUTTON1_DOUBLE_CLICKED)
				{
					if ((mouse_y > 0) &&(mouse_y < maxy - 1))
					{
						for (int i = cursor; i >= 0; i--)
						{
							if (hyperobjects[i].line == mouse_y + pos - 1)
							{
								if (hyperobjects[i].col <= mouse_x - 1)
								{
									if (hyperobjects[i].col + strlen(hyperobjects[i].node) + strlen(hyperobjects[i].file) >= mouse_x - 1)
									{
										if (hyperobjects[i].type < HIGHLIGHT)
										{
											cursor = i;
											done = 1;
											break;
										}
									}
								}
							}
						}
						if (!done)
							for (unsigned i = cursor; i < hyperobjectcount; i++)
							{
								if (hyperobjects[i].line == mouse_y + pos - 1)
								{
									if (hyperobjects[i].col <= mouse_x - 1)
									{
										if (hyperobjects[i].col + strlen(hyperobjects[i].node) + strlen(hyperobjects[i].file) >= mouse_x - 1)
										{
											if (hyperobjects[i].type < HIGHLIGHT)
											{
												cursor = i;
												done = 1;
												break;
											}
										}
									}
								}
							}
						if (done)
							ungetch(keys.followlink_1);
					}		/* end: if (mouse_y not on top/bottom line) */
					else if (mouse_y == 0)
						ungetch(keys.pgup_1);
					else if (mouse_y == maxy - 1)
						ungetch(keys.pgdn_1);
				}		/* end: button doubleclicked */
			}
#endif /* NCURSES_MOUSE */
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
	aftersearch = 0;
	return rval;
}

void
next_infomenu()
{
	if (hyperobjectcount == 0)
	{
		infomenu = -1;
		return;
	}
	for (unsigned long i = infomenu + 1; i < hyperobjectcount; i++)
	{
		if (hyperobjects[i].type <= 1)	/* menu item */
		{
			infomenu = i;
			return;
		}
	}
	infomenu = -1;		/* no menuitem left is found */
}

void
rescan_cursor()
{
	for (unsigned long i = 0; i < hyperobjectcount; i++)
	{
		if ((hyperobjects[i].line >= pos) &&
				(hyperobjects[i].line < pos +(maxy - 2)))
		{
			if (hyperobjects[i].type < HIGHLIGHT)
			{
				cursor = i;
				break;
			}
		}
	}
}

int
getnodeoffset(int tag_table_pos, int *Indstart)	/* count node offset in file */
{
#define indirectstart	(*Indstart)
	int i, fileoffset = 0;
	if (indirect)
	{
		for (i = IndirectEntries; i >= 1; i--)
		{
			if (indirect[i].offset <= tag_table[tag_table_pos].offset)
			{
				fileoffset +=(tag_table[tag_table_pos].offset - indirect[i].offset + FirstNodeOffset);
				indirectstart = i;
				break;
			}
		}
	}
	else
	{
		fileoffset +=(tag_table[tag_table_pos].offset - 2);
	}
	return fileoffset;
#undef indirectstart
}
