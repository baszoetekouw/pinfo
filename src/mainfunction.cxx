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
#include "printinfo.h"
#include <string>
using std::string;
#include <vector>
using std::vector;

#include <ctype.h>

#ifndef MIN
#define        MIN(a,b)(((a)<(b))?(a):(b))
#endif

void rescan_cursor();	/* set the cursor to 1st item on visible screen */
void next_infomenu();	/* go to the next menu item for sequential reading */
int getnodeoffset(int tag_table_pos,
			typeof(indirect.size())& indirectstart);	/* get node offset in file */

int aftersearch = 0;
/*
 * this flag is turned on when the engine receives a simulated `key.back',
 * caused by the sequential auto-pgdn reading code
 */
int toggled_by_menu = 0;
long pos, infomenu;
long infocolumn=0;

int cursor;



/* Inline support functions formerly in menu_and_note_utils.cxx */

/*
 * Read the `$foo' header entry
 * Eliminates former duplicate code
 */

#define ERRNODE "ERR@!#$$@#!%%^#@!OR"

static inline string
get_foo_node(const char * const foo, string type)
{
	string::size_type start_idx = type.find(foo);
	if (start_idx == string::npos) {
		return string(ERRNODE);
	}

	start_idx += strlen(foo);
	string::size_type end_idx = type.find_first_of(",\n", start_idx);
	if (end_idx != string::npos) {
		return type.substr(start_idx, end_idx - start_idx);
	}
}

/* read the `Next:' header entry */
static inline string
getnextnode(string type)
{
	return get_foo_node("Next: ", type);
}

/* read the `Prev:' header entry */
static inline string
getprevnode(string type)
{
	return get_foo_node("Prev: ", type);
}

/* read the `Up:' header entry */
static inline string
getupnode(string type)
{
	return get_foo_node("Up: ", type);
}

/* read the `Node:' header entry */
static inline string
getnodename(string type)
{
	return get_foo_node("Node: ", type);
}

/* Main work functions */

WorkRVal
work(const vector<string> my_message, char **type, FILE * id, int tag_table_pos)
{
#define Type	(*type)
	static WorkRVal rval;
	FILE *pipe;
	int fileoffset;
	typeof(indirect.size()) indirectstart = -1;
	int cursorchanged = 0;
	int key = 0;
	int return_value;
	int statusline = FREE;
	char *token, *tmp;
	/* if the static variable was allocated, free it */
	rval.file = "";
	rval.node = "";
	rval.keep_going = false; /* Important */

	pos = 1, cursor = 0, infomenu = -1;	/* default position, and selected number */

#ifdef getmaxyx
	getmaxyx(stdscr, maxy, maxx);	/* initialize maxx, maxy */
#else
	maxx = 80;
	maxy = 25;
#endif /*  getmaxyx */
	/* Clear old hyperlink info */
	hyperobjects.clear();
	/* initialize node-links for every line */
	for (int i = 0; i < my_message.size() - 1; i++)
	{
		/* Horrible conversion to 1-based index here. FIXME. */
		initializelinks(my_message[i].c_str(), my_message[i + 1].c_str(), i + 1);
	}
	/* Horrible conversion to 1-based index here. FIXME. */
	initializelinks(my_message[my_message.size() - 1].c_str(),"",
	                my_message.size());

	/* infomenu will remain -1 if it's the last pos, or if there's no menu item */
	next_infomenu();

	if (npos != -1)
		pos = npos;			/* set eventual history pos */

	/* if we're in a node found using 's'earch function. */
	if (aftersearch)
	{
		pos = aftersearch;	/* set pos to the found position */
		/*  aftersearch=0;  * don't reset this--we want to know if we mus highlight something */
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
	string type_str = Type;
	addtopline(type_str,infocolumn);
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
			if (statusline == FREE) {
				showscreen(my_message, pos, cursor, infocolumn);
			}
			waitforgetch();
			key = pinfo_getch();
		}
		nodelay(stdscr, FALSE);
		statusline = FREE;
		if (winchanged)		/* SIGWINCH */
		{
			handlewinch();
			winchanged = 0;
			string type_str = Type;
			addtopline(type_str,infocolumn);
			key = pinfo_getch();
		}
		/***************************** keyboard handling ****************************/
		if (key != 0)
		{
			if ((key == keys.print_1) ||
					(key == keys.print_2))
			{
				if (yesno(_("Are you sure you want to print?"), 0) == 1) {
					printnode(my_message);
				}
			}
			/*==========================================================================*/
			if ((key == keys.pgdn_auto_1) ||
					(key == keys.pgdn_auto_2) ||
					(toggled_by_menu))
			{
				int wastoggled = toggled_by_menu;
				toggled_by_menu = 0;
				/* if hyperobject type <= 1, then we have a menu */
				if ((pos >= my_message.size() -(maxy - 2)) ||(wastoggled))
				{
					if ((infomenu != -1) &&(!wastoggled))
					{
						cursor = infomenu;
						key = keys.followlink_1;	/* the handler for keys.followlink must be bellow this statement! */
					}
					else
						/* we shouldn't select a menu item if this node is called via `up:' from bottom, or if there is no menu */
					{
						string type_str = getnextnode(Type);
						if (type_str != ERRNODE) {
							key = keys.nextnode_1;
						}	else {
							type_str = getnodename(Type);
							if (FirstNodeName != type_str)	/* if it's not end of all menus */
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
				long newpos;
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
					for (int i = 0; token[i] != 0; i++)
					{
						if (!isdigit(token[i]))
							digit_val = 0;
					}
					if (digit_val)	/* go to specified line */
					{
						newpos = atol(token);
						newpos -=(maxy - 1);
						if ((newpos > 0) &&(newpos < my_message.size() -(maxy - 2)))
							pos = newpos;
						else if ((newpos > 0) &&((my_message.size() -(maxy - 2)) > 0))
							pos = my_message.size() -(maxy - 2);
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
				system("clear");
				pipe = popen(token, "w");	/* open pipe */
				if (pipe != NULL)
				{
					/* and flush the msg to stdin */
					for (int i = 0; i < my_message.size(); i++)	
						fprintf(pipe, "%s", my_message[i].c_str());
					pclose(pipe);
					getchar();
				}
				doupdate();
				curs_set(0);
				if (pipe == NULL)
					mvaddstr(maxy - 1, 0, _("Operation failed..."));
				xfree(token);
				token = 0;
			}
			/*==========================================================================*/
			if ((key == keys.dirpage_1) ||
					(key == keys.dirpage_2))
			{
				rval.file = "dir";
				rval.node = "";
				rval.keep_going = true;
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
					token = getstring(_("Enter regexp: "));	/* get the token */
					searchagain.lastsearch = token;	/* and save it to searchagain buffer */
					/*
					 * give a hint, which key to ungetch to call this procedure
					 * by searchagain
					 */
					searchagain.type = key;
				}
				else /* it IS searchagain */
				{
					token = (char*)xmalloc(searchagain.lastsearch.length() + 1);
					/* allocate space for token */
					strcpy(token, searchagain.lastsearch.c_str());
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
				for (int i = 0; i < pos + 1; i++)	/* count the length of curnode */
					fileoffset += my_message[i].length();
				fileoffset += strlen(Type);	/* add also header length */

				fileoffset += getnodeoffset(tag_table_pos, indirectstart);	/* also load the variable indirectstart */

				/* Searching part...  */
				aftersearch = 0;

				/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
				return_value = -1;
				/* the info is of indirect type; we'll search through several files */
				if (!indirect.empty())
				{
					FILE *fd;
					long tokenpos;
					long starttokenpos;
					long filelen;
					/* Signed/unsigned issues. */
					for (signed int j = indirectstart;
					     j < indirect.size(); j++)
					{
						fd = openinfo(indirect[j].filename, 1);	/* get file length. */
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

						tmp = (char*)xmalloc(filelen - starttokenpos + 10);	/* read data */
						fread(tmp, 1, filelen - starttokenpos, fd);
						tmp[filelen - starttokenpos + 1] = 0;

						tokenpos = regexp_search(token, tmp);	/* search */

						if (tokenpos != -1)	/* if something was found */
						{
							/*
							 * add the offset of the part of file, which wasn't
							 * read to the memory
							 */
							tokenpos += starttokenpos;
							{	/* local scope for tmpvar, matched */
								int tmpvar = -1, matched = 0;
								for (int i = tag_table.size() - 1; i >= 0; i--)
								{
									if ((tag_table[i].offset > tag_table[tmpvar].offset) &&
											((tag_table[i].offset - indirect[j].offset + FirstNodeOffset) <= tokenpos))
									{
										return_value = i;
										tmpvar = i;
										matched = 1;
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
					tmp = (char*)xmalloc(filelen - starttokenpos + 10);
					fread(tmp, 1, filelen - starttokenpos, id);
					tmp[filelen - starttokenpos + 1] = 0;

					/* search */
					tokenpos = regexp_search(token, tmp);

					if (tokenpos != -1)	/* if we've found something */
					{
						/*
						 * add offset of the start of search area to this token
						 * position.
						 */
						tokenpos += starttokenpos;
						{		/* local scope for tmpvar, matched */
							int tmpvar = -1, matched = 0;
							for (int i = tag_table.size() - 1; i >= 0; i--)
							{
								if ((tag_table[i].offset > tag_table[tmpvar].offset) &&
										(tag_table[i].offset <= tokenpos))
								{
									return_value = i;
									tmpvar = i;
									matched = 1;
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
					mvaddstr(maxy - 1, 0, _("Search string not found..."));
					statusline = LOCKED;
				}

				if (!aftersearch)
					aftersearch = tmpaftersearch;

				if (return_value != -1)
				{
					infohistory[infohistory.size() - 1].pos = pos;
					infohistory[infohistory.size() - 1].cursor = cursor;
					infohistory[infohistory.size() - 1].menu = infomenu;
					rval.node = tag_table[return_value].nodename;
					rval.file = "";
					rval.keep_going = true;
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
					token = getstring(_("Enter regexp: "));
					searchagain.lastsearch = token;
					searchagain.type = key;
				}
				else
				{
					token = (char*)xmalloc(searchagain.lastsearch.length() + 1);
					strcpy(token, searchagain.lastsearch.c_str());
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
				/* Note that pos is still 1-based */
				for (int i = pos; i < my_message.size() - 1; i++)
				{
					/*
					 * glue two following lines into one -- to find matches
					 * split up into two lines.
					 */
					string tmpstr = my_message[i];
					tmpstr += my_message[i + 1];
					tmp = strdup(tmpstr.c_str());
					if (pinfo_re_exec(tmp))	{ /* execute the search command */
						/* if found, enter here */
						success = 1;
						char* tmp2 = strdup(my_message[i + 1].c_str());
						if (pinfo_re_exec(tmp2)) {
						/* if token was found in the second line, make pos=i+1.  */
							pos = i + 1;
						}	else {
							/* otherwise, pos=i. This happens when we have a split expression. */
							pos = i;
						}
						free(tmp2);
						free(tmp);
						tmp = 0;
						aftersearch = 1;
						break;
					} else { /* nothing found */
						xfree(tmp);	/* free tmp buffer */
						tmp = 0;
					}
				}
				if (!success)
				{
					attrset(bottomline);
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
				token = getstring(_("Enter node name: "));	/* read user's wish */
				curs_set(0);
				noecho();
				attrset(normal);
				for (typeof(tag_table.size()) i = 0; i < tag_table.size(); i++)
				{
					/* if the name was found in the tag table */
					if (tag_table[i].nodename == token)
					{
						return_value = i;
						break;
					}
				}
				if (return_value != -1)	/* if the name was in tag table */
				{
					xfree(token);
					token = 0;

					infohistory[infohistory.size() - 1].pos = pos;
					infohistory[infohistory.size() - 1].cursor = cursor;
					infohistory[infohistory.size() - 1].menu = infomenu;
					rval.node = tag_table[return_value].nodename;
					rval.file = "";
					rval.keep_going = true;
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
							char* tmp_ick = (char*)xmalloc(gotoendptr - gotostartptr + 1);
							strncpy(tmp_ick, gotostartptr + 1, gotoendptr - gotostartptr - 1);
							tmp_ick[gotoendptr - gotostartptr - 1] = 0;
							rval.file = tmp_ick;
							xfree(tmp_ick);
							gotoendptr++;
							while (gotoendptr)	/* skip whitespaces until nodename */
							{
								if (*gotoendptr != ' ')
									break;
								gotoendptr++;
							}	/* skip spaces */
							rval.node = gotoendptr; /* Needs cleanup.  Eeeew. */
							rval.keep_going = true;
							xfree(token);
							token = 0;
							aftersearch = 0;
							return rval;
						}
					}
					/* handle the `file.info' format of crossinfo goto. */
					else if (strstr(token, ".info"))
					{
						rval.file = token;
						xfree(token);
						token = 0;
						rval.node = "";
						aftersearch = 0;
						rval.keep_going = true;
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
				string token_str = getprevnode(Type);
				return_value = gettagtablepos(token_str);
				if (return_value != -1)
				{
					infohistory[infohistory.size() - 1].pos = pos;
					infohistory[infohistory.size() - 1].cursor = cursor;
					infohistory[infohistory.size() - 1].menu = infomenu;
					rval.node = tag_table[return_value].nodename;
					rval.file = "";
					rval.keep_going = true;
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.nextnode_1) ||	/* goto next node */
					(key == keys.nextnode_2))
			{
				string token_str;
				token_str = getnextnode(Type);
				return_value = gettagtablepos(token_str);
				if (return_value != -1)
				{
					infohistory[infohistory.size() - 1].pos = pos;
					infohistory[infohistory.size() - 1].cursor = cursor;
					infohistory[infohistory.size() - 1].menu = infomenu;
					rval.node = tag_table[return_value].nodename;
					rval.file = "";
					rval.keep_going = true;
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.upnode_1) ||		/* goto up node */
					(key == keys.upnode_2))
			{
				string token_str = getupnode(Type);
				if (token_str.compare(0, 5, "(dir)") == 0)
				{
					ungetch(keys.dirpage_1);
				}
				return_value = gettagtablepos(token_str);
				if (return_value != -1)
				{
					if (toggled_by_menu == KEEP_HISTORY)
					{
						infohistory[infohistory.size() - 1].pos = pos;
						infohistory[infohistory.size() - 1].cursor = cursor;
						infohistory[infohistory.size() - 1].menu = infomenu;
					}
					rval.node = tag_table[return_value].nodename;
					rval.file = "";
					rval.keep_going = true;
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
				if (cursor != (typeof(hyperobjects.size()))-1)	{
					/* if we must handle cursor... */
					if ((cursor > 0) &&(hyperobjects.size()))
						/* if we really must handle it ;) */
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
					if (pos > 2)	/* lower the nodepos */
						pos--;
					/* and scan for a hyperlink in the new line */
					for (typeof(hyperobjects.size()) i = 0;
					     i < hyperobjects.size(); i++) {
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
				pos = my_message.size() -(maxy - 2);
				if (pos < 1)
					pos = 1;
				cursor = hyperobjects.size() - 1;
			}
			/*==========================================================================*/
			if ((key == keys.pgdn_1) ||
					(key == keys.pgdn_2))
			{
				if (pos +(maxy - 2) < my_message.size() -(maxy - 2))
				{
					pos +=(maxy - 2);
					rescan_cursor();
				}
				else if (my_message.size() -(maxy - 2) >= 1)
				{
					pos = my_message.size() -(maxy - 2);
					cursor = hyperobjects.size() - 1;
				}
				else
				{
					pos = 1;
					cursor = hyperobjects.size() - 1;
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
				if (cursor < hyperobjects.size())
					for (typeof(hyperobjects.size()) i = cursor + 1;
					     i < hyperobjects.size(); i++)
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
				if (!cursorchanged)
				{
					if (pos <= my_message.size() -(maxy - 2))
						pos++;
					for (typeof(hyperobjects.size()) i = cursor + 1;
					     i < hyperobjects.size(); i++)
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
				infohistory[infohistory.size() - 1].pos = pos;
				infohistory[infohistory.size() - 1].cursor = cursor;
				infohistory[infohistory.size() - 1].menu = infomenu;
				rval.node = FirstNodeName;
				rval.file = "";
				rval.keep_going = true;
				aftersearch = 0;
				return rval;
			}
			/*==========================================================================*/
			if ((key == keys.back_1) ||
					(key == keys.back_2))
			{
				if (infohistory.size() > 1)
				{
					dellastinfohistory();	/* remove history entry for this node */
					/* now we deal with the previous node history entry */

					rval.node = infohistory[infohistory.size() - 1].node;
					rval.file = infohistory[infohistory.size() - 1].file;
					rval.keep_going = true;

					npos = infohistory[infohistory.size() - 1].pos;
					ncursor = infohistory[infohistory.size() - 1].cursor;
					nmenu = infohistory[infohistory.size() - 1].menu;
					dellastinfohistory();	/* remove history entry for previous node */
					aftersearch = 0;
					return rval;
				}
			}
			/*==========================================================================*/
			if ((key == keys.followlink_1) ||
					(key == keys.followlink_2))
			{
				infohistory[infohistory.size() - 1].pos = pos;
				infohistory[infohistory.size() - 1].cursor = cursor;
				infohistory[infohistory.size() - 1].menu = infomenu;
				if (!toggled_by_menu)
					infohistory[infohistory.size() - 1].menu = cursor;
				if ((cursor >= 0) && (cursor < hyperobjects.size()))
					if ((hyperobjects[cursor].line >= pos) &&
							(hyperobjects[cursor].line < pos +(maxy - 2)) ||
							(toggled_by_menu))
					{
						toggled_by_menu = 0;
						if (hyperobjects[cursor].type < 4)	/* normal info link */
						{
							rval.node = hyperobjects[cursor].node;
							rval.file = hyperobjects[cursor].file;
							rval.keep_going = true;
							aftersearch = 0;
							return rval;
						}
						else if (hyperobjects[cursor].type < HIGHLIGHT)	/* we deal with an url */
						{
							if (hyperobjects[cursor].type == 4)	/* http */
							{
								string tempbuf = httpviewer;
								tempbuf += " ";
								tempbuf += hyperobjects[cursor].node;
								myendwin();
								system(tempbuf.c_str());
								doupdate();
							}
							else if (hyperobjects[cursor].type == 5)	/* ftp */
							{
								string tempbuf = ftpviewer;
								tempbuf += " ";
								tempbuf += hyperobjects[cursor].node;
								myendwin();
								system(tempbuf.c_str());
								doupdate();
							}
							else if (hyperobjects[cursor].type == 6)	/* mail */
							{
								string tempbuf = maileditor;
								tempbuf += " ";
								tempbuf += hyperobjects[cursor].node;
								myendwin();
								system("clear");
								system(tempbuf.c_str());
								doupdate();
							}
						}
					}
			}
			/*==========================================================================*/
			if ((key == keys.left_1) ||(key == keys.left_2))
			{
				if (infocolumn>0)
					infocolumn--;
				string typestr = Type;
				addtopline(typestr,infocolumn);
			}
			/*==========================================================================*/
			if ((key == keys.right_1) ||(key == keys.right_2))
			{
				infocolumn++;
				string typestr = Type;
				addtopline(typestr,infocolumn);
			}
			/*==========================================================================*/
			/**************************** end of keyboard handling **********************/
			/******************************** mouse handler *****************************/
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
						for (typeof(hyperobjects.size()) i = cursor; i > 0; i--)
						{
							if (hyperobjects[i].line == mouse.y + pos - 1)
							{
								if (hyperobjects[i].col <= mouse.x - 1)
								{
									if (hyperobjects[i].col + hyperobjects[i].node.length() + hyperobjects[i].file.length() >= mouse.x - 1)
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
							for (typeof(hyperobjects.size()) i = cursor;
							     i < hyperobjects.size(); i++)
							{
								if (hyperobjects[i].line == mouse.y + pos - 1)
								{
									if (hyperobjects[i].col <= mouse.x - 1)
									{
										if (hyperobjects[i].col + hyperobjects[i].node.length() + hyperobjects[i].file.length() >= mouse.x - 1)
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
					}		/* end: if (mouse.y not on top/bottom line) */
					else if (mouse.y == 0)
						ungetch(keys.up_1);
					else if (mouse.y == maxy - 1)
						ungetch(keys.down_1);
				}		/* end: button clicked */
				if (mouse.bstate == BUTTON1_DOUBLE_CLICKED)
				{
					if ((mouse.y > 0) &&(mouse.y < maxy - 1))
					{
						/* signed/unsigned.  Use iterators.  FIXME. */
						for (int i = cursor; i >= 0; i--)
						{
							if (hyperobjects[i].line == mouse.y + pos - 1)
							{
								if (hyperobjects[i].col <= mouse.x - 1)
								{
									if (hyperobjects[i].col + hyperobjects[i].node.length() + hyperobjects[i].file.length() >= mouse.x - 1)
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
							for (typeof(hyperobjects.size()) i = cursor;
							     i < hyperobjects.size(); i++)
							{
								if (hyperobjects[i].line == mouse.y + pos - 1)
								{
									if (hyperobjects[i].col <= mouse.x - 1)
									{
										if (hyperobjects[i].col + hyperobjects[i].node.length() + hyperobjects[i].file.length() >= mouse.x - 1)
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
					}		/* end: if (mouse.y not on top/bottom line) */
					else if (mouse.y == 0)
						ungetch(keys.pgup_1);
					else if (mouse.y == maxy - 1)
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
				if (yesno(_("Are you sure to quit?"), QuitConfirmDefault))
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
	if (hyperobjects.size() == 0) {
		infomenu = -1;
		return;
	}
	for (typeof(hyperobjects.size()) i = infomenu + 1;
	     i < hyperobjects.size(); i++) {
		if (hyperobjects[i].type <= 1) { /* menu item */
			infomenu = i;
			return;
		}
	}
	infomenu = -1;		/* no more menuitems found */
}

void
rescan_cursor()
{
	for (typeof(hyperobjects.size()) i = 0; i < hyperobjects.size(); i++)
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
getnodeoffset(int tag_table_pos,
              typeof(indirect.size())& indirectstart)
							/* count node offset in file */
{
	int fileoffset = 0;
	if (!indirect.empty())
	{
		/* signed/unsigned.  Use iterators. FIXME */
		for (int i = indirect.size() - 1; i >= 0; i--)
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
}
