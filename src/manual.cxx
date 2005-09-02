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

RCSID("$Id$")

#include <ctype.h>
#include <sys/stat.h>
#include <string>
using std::string;
#include <vector>
using std::vector;

#define HTTPSECTION 100
#define FTPSECTION 101
#define MAILSECTION 102

/* check if a char is a hyphen character */
int ishyphen(unsigned char ch);
/* load manual */
void loadmanual(FILE * id);
/* handle keyboard */
int manualwork();
void rescan_selected();	/* scan for potential link to select on
							   viewed manual page */
/* self explanatory */
void showmanualscreen();
/* mvaddstr with bold/italic */
void mvaddstr_manual(int y, int x, char *str);
/* adds highlights to a painted screen */
void add_highlights();
/* strips line of formatting characters */
void strip_manual(string& buf);
/*
 * Initialize links in a line .  Links are entries of form reference(section),
 * and are stored in `manuallinks' var, described bellow.
 */
void man_initializelinks(char *line, int carry);
int is_in_manlinks(string in, char *find);

void printmanual(char **Message, long Lines);

/* line by line stored manual */
char **manual = 0;
/* number of lines in manual */
int ManualLines = 0;
int selected = -1;		/* number of selected link(offset in 'manuallinks',
						   bellow) */
int manualpos = 0;		/* number of the first line, which is painted on
						   screen */

int manualcol = 0;		/* the first displayed column of manpage--
						   for moving the screen left/right */

int manual_aftersearch = 0;	/* this is set if man page is now after search
							   operation */
int manwidthChanged = 0;	/* this flag indicates whether the env variable
							   $MANWIDTH was changed by pinfo */

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

/* semaphore for checking if it's a history(left arrow) call */
int historical = 0;

/* Debugging routine */
void
dumplink(manuallink a) {
	printf("LINK x%sx (x%sx %d) at %d %d (%d)\n\r", (a.name).c_str(),
		(a.section).c_str(), a.section_mark,
		a.line, a.col, a.carry);
}

void
/* free buffers allocated by current man page */
manual_free_buffers()
{
	int i;
	/* first free previously allocated memory */
	/* for the manual itself... */
	if (manual)
	{
		for (i = 0; i <= ManualLines; i++)
		{
			xfree(manual[i]);
		}
		xfree(manual);
		manual = 0;
		ManualLines = 0;
	}
	/* ...and for the list of manual hypertext */
	if (manuallinks.size() > 0)
	{
		manuallinks.clear();
		selected = -1;
	}
}

/* initialize history variables for manual pages.  */
void
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
void
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

/* this is something like main() function for the manual viewer code.  */
int
handlemanual(string name)
{
	int return_value;
	struct stat statbuf;
	FILE *id;

	string manualname_string; /* Filled by construct_manualname */
	char *raw_tempfilename = 0;
	char *apropos_tempfilename = 0;

	if (tmpfilename1)
	{
		unlink(tmpfilename1);
		xfree(tmpfilename1);
	}
	tmpfilename1 = tempnam("/tmp", NULL);

#ifdef getmaxyx
	init_curses();
	/* if ncurses, get maxx and maxy */
	getmaxyx(stdscr, maxy, maxx);
	myendwin();
	if ((!getenv("MANWIDTH")) ||(manwidthChanged))
	{
		/* set MANWIDTH environment variable */
		static char tmp[24];
		snprintf(tmp, 24, "MANWIDTH=%d", maxx);
		putenv(tmp);
		manwidthChanged = 1;
	}
#else
	/* otherwise hardcode 80x25... */
	maxx = 80;
	maxy = 25;
#endif /* getmaxyx */

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
			unlink(tmpfilename1);
			printf(_("Error: No manual page found\n"));
			plain_apropos = 1; /* Fallback */
		} else {
			id = fopen(tmpfilename1, "r");
		}
	}
	if (plain_apropos) {
		plain_apropos = 0;
		if (!use_apropos) {
			return 1;
		}
		printf(_("Calling apropos \n"));
		apropos_tempfilename = tempnam("/tmp", NULL);
		string cmd_string = "apropos ";
		cmd_string += name;
		cmd_string += " > ";
		cmd_string += apropos_tempfilename;
		if (system(cmd_string.c_str()) != 0) {
			printf(_("Nothing appropriate\n"));
			unlink(apropos_tempfilename);
			return 1;
		} else {
			id = fopen(apropos_tempfilename, "r");
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
#ifdef getmaxyx
		/* if ncurses, get maxx and maxy */
		getmaxyx(stdscr, maxy, maxx);
		if ((!getenv("MANWIDTH")) ||(manwidthChanged))
		{
			/* set MANWIDTH environment variable */
			static char tmp[24];
			snprintf(tmp, 24, "MANWIDTH=%d", maxx);
			putenv(tmp);
			manwidthChanged = 1;
		}
#endif
		manual_aftersearch = 0;
		/* -1 is quit key */
		if (return_value != -1)
		{
			if (tmpfilename2)
			{
				unlink(tmpfilename2);
				xfree(tmpfilename2);
			}
			tmpfilename2 = tempnam("/tmp", NULL);
			/*
			 * key_back is not pressed; and return_value is an offset to
			 * manuallinks
			 */
			string cmd_string = "man ";
			cmd_string += ManOptions;
			cmd_string += " ";
			if (return_value != -2)
			{
				construct_manualname(manualname_string, return_value);
				cmd_string += manuallinks[return_value].section;
				cmd_string += " ";
				cmd_string += manualname_string;
			}
			else /* key_back was pressed */
			{
				if ( (manualhistory.size() - 2) == 0 && apropos_tempfilename)
				{
					id = fopen(apropos_tempfilename, "r");
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
				/*
				 * flag to make sure, that
				 * manualwork will refresh the variables manualpos and selected
				 * when going back to this page
				 */
				historical = 1;
				manualhistory.pop_back();
			}
			cmd_string += " ";
			cmd_string += StderrRedirection;
			cmd_string += " > ";
			cmd_string += tmpfilename2;
			system(cmd_string.c_str());
			stat(tmpfilename2, &statbuf);
			if (statbuf.st_size > 0)
			{
				string cmd_string = "mv ";
				cmd_string += tmpfilename2;
				cmd_string += " ";
				cmd_string += tmpfilename1;
				/* create tmp file containing man page */
				system(cmd_string.c_str());
				/* open man page */
				id = fopen(tmpfilename1, "r");
				if (id != NULL)
				{
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
					}
					/* loading manual page and its defaults... */
					loadmanual(id);
					fclose(id);
					/* continuing with creation of history */
					if (!historical)
					{
						my_hist.pos = manualpos;
						my_hist.selected = selected;
						manualhistory.push_back(my_hist);
					}
					else
						historical = 0;
				}
				else
					return_value = -1;
			}
		}
	} while (return_value != -1);
	if (apropos_tempfilename)
		unlink(apropos_tempfilename);
	/* we were using temporary */
	if (raw_tempfilename)
		unlink(raw_tempfilename);
	/* raw-manpage for scanning */
	return 0;
}

void
/* loads manual from given filedescriptor */
loadmanual(FILE * id)
{
	char prevlinechar = 0;
	/* tmp variable, set after reading first nonempty line of input */
	int cutheader = 0;
	int carryflag = 0;
	manualpos = 0;
	manual_free_buffers();
	manual = (char**)xmalloc(sizeof(char *));
	manuallinks.clear();
	manual[ManualLines] = (char*)xmalloc(1024);

	/* we read until eof */
	while (!feof(id))
	{
		/*
		 * it happens sometimes, that the last line is weird
		 * and causes sigsegvs by not entering anything to buffer, what
		 * confuses strlen
		 */
		if (fgets(manual[ManualLines], 1024, id) == NULL)
			manual[ManualLines][0] = 0;

		if (cutheader)
		{
			if (strcmp(manual[cutheader], manual[ManualLines]) == 0)
			{
				manual[ManualLines][0] = '\n';
				manual[ManualLines][1] = 0;
			}
		}
		if (FilterB7)
		{
			char *filter_pos = index(manual[ManualLines], 0xb7);
			if (filter_pos)
				*filter_pos = 'o';
		}
		if (CutManHeaders)
			if (!cutheader)
			{
				if (strlen(manual[ManualLines]) > 1)
				{
					cutheader = ManualLines;
				}
			}
		if ((CutEmptyManLines) &&((manual[ManualLines][0]) == '\n') &&
				(prevlinechar == '\n'))
			;			/* do nothing :)) */
		else
		{
			int manlinelen = strlen(manual[ManualLines]);
			manual[ManualLines] = (char*)xrealloc(manual[ManualLines],
					manlinelen + 10);

			/* temporary variable for determining hypertextuality of fields */
			string tmpstr;
			tmpstr = manual[ManualLines];
			strip_manual(tmpstr);

			char* tmp;
			tmp = strdup(tmpstr.c_str());
			man_initializelinks(tmp, carryflag);
			carryflag = 0;
			if (manlinelen > 1)
				if (ishyphen(manual[ManualLines][manlinelen - 2]))
					carryflag = 1;
			/* free temporary buffer */
			xfree(tmp);
			prevlinechar = manual[ManualLines][0];
			/* increase the number of man lines */
			ManualLines++;
			/*
			 * and realloc manual to add an empty space for
			 * next entry of manual line
			 */
			manual = (char**)xrealloc(manual,(ManualLines + 5) * sizeof(char *));
			manual[ManualLines] = (char*)xmalloc(1024);
		}
	}

}

bool
compare_manuallink(manuallink a, manuallink b)
{
	/* Should a sort before b? */
  return (a.col < b.col);
}

void
sort_manuallinks_from_current_line(
	vector<manuallink>::iterator startlink,
	vector<manuallink>::iterator endlink)
{
	std::sort(startlink, endlink, compare_manuallink);
}

/* initializes hyperlinks in manual */
void
man_initializelinks(char *tmp, int carry)
{
	/******************************************************************************
	 * handle url refrences                                                       *
	 *****************************************************************************/
	char *urlstart, *urlend;
	urlend = tmp;

	vector<manuallink>::size_type initialManualLinks = manuallinks.size();

	char* crap = tmp;
	while ((urlstart = strstr(urlend, "http://")) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallink my_link;
		my_link.line = ManualLines;
		my_link.col = urlstart - tmp;
		my_link.section = "HTTPSECTION";
		my_link.section_mark = HTTPSECTION;
		my_link.name.assign(urlstart, urlend - urlstart - 1);
		if (ishyphen(my_link.name[urlend - urlstart - 1]))
			my_link.carry = 1;
		else
			my_link.carry = 0;
		manuallinks.push_back(my_link);
	}
	urlend = tmp;
	while ((urlstart = strstr(urlend, "ftp://")) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallink my_link;
		my_link.line = ManualLines;
		my_link.col = urlstart - tmp;
		my_link.section = "FTPSECTION";
		my_link.section_mark = FTPSECTION;
		my_link.name.assign(urlstart, urlend - urlstart - 1);
		if (ishyphen(my_link.name[urlend - urlstart - 1]))
			my_link.carry = 1;
		else
			my_link.carry = 0;
		manuallinks.push_back(my_link);
	}
	urlend = tmp;
	while ((urlstart = findemailstart(urlend)) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallink my_link;
		my_link.line = ManualLines;
		my_link.col = urlstart - tmp;
		my_link.section = "MAILSECTION";
		my_link.section_mark = MAILSECTION;
		my_link.name.assign(urlstart, urlend - urlstart - 1);
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
	/* set tmpcnt to the trailing zero of tmp */
	int tmpcnt = strlen(tmp) + 1;
	char *link = tmp;
	int i, b;
	do {
		/* we look for '(', since manual link */
		link = strchr(link, '(');
		/* has form of  'blah(x)' */
		if (link != NULL)
		{
			char *temp;
			/* look for the closing bracket */
			if ((temp = strchr(link, ')')))
			{
				char *p_t1, *p_t;
				p_t = p_t1 = (char*)xmalloc((strlen(link) + 10) * sizeof(char));
				for (++link; link != temp; *p_t++ = *link++);
				*p_t = '\0';
				link -=(strlen(p_t1) + sizeof(char));

				if ((!strchr(p_t1, '(')) &&(!is_in_manlinks(manlinks, p_t1)))
				{
					char tempchar;
					int breakpos;
					i = link - tmp - 1;
					if (i < 0)
						i++;
					for (; i > 0; --i)
					{
						if (!isspace(tmp[i]))
							/* ignore spaces between linkname and '(x)' */
							break;
					}
					/* we'll put zero on the last non-textual character of link */
					breakpos = i + 1;
					/* but remember the cleared char for the future */
					tempchar = tmp[breakpos];
					tmp[breakpos] = 0;
					/*
					 * scan to the first space sign or to 0 -- that means go to
					 * the beginning of the scanned token
					 */
					for (i = breakpos; i > 0; --i)
					{
						if (isspace(tmp[i]))
						{
							i++;
							break;
						}
					}
					/* now we have needed string in i..breakpos. We need now to
					 * realloc the
					 * manuallinks table to make free space for new entry
					 */

					/* a small check */
					if (!((use_apropos) && (manualhistory.size() - 1 == 0)))
					{
						/*
						 * In English: if the name of the link is the name of
						 * the current page and the section of the link is the
						 * current section or if we don't know the current
						 * section, then...
						 */
						if (    (!strcasecmp(&tmp[i], manualhistory[manualhistory.size() - 1].name.c_str()))
								 && (    (!strcasecmp(p_t1, manualhistory[manualhistory.size() - 1].sect.c_str()))
									    || (manualhistory[manualhistory.size() - 1].sect == "")
									    || (manualhistory[manualhistory.size() - 1].sect == " ")
						        )
						   ) {
							break;
						}
					}
					manuallink my_link;
					my_link.line = ManualLines;
					my_link.col = i;
					if (LongManualLinks)
					{
						for (b = 1; link[b] != ')'; b++)
							my_link.section[b - 1] = tolower(link[b]);
						my_link.section.resize(b - 1);
					}
					else
					{
						/* Short manual links */
						my_link.section = link[1];
					}
					my_link.section_mark = 0;
					my_link.name = (tmp + i);
					tmp[breakpos] = tempchar;

					/* check whether this is a carry'ed entry(i.e. in the
					 * previous line there was `-' at end, and this is the
					 * first word of this line */
					for (b = i - 1; b >= 0; b--)
					{
						if (b > 0)
							if (!isspace(tmp[b]))
								break;
					}
					if (b >= 0)
						my_link.carry = 0;
					else
						my_link.carry = carry;
					/* increase the number of entries */
					manuallinks.push_back(my_link);
				}		/*... if (in man links) */
				xfree((void *) p_t1);
			}
		}
		if (link)
			link++;
		if (link > (tmp + tmpcnt))
		{
			break;
		}
	} while (link != NULL);
	/* do this loop until strchr() won't find a '(' in string */

	if (manuallinks.size() > initialManualLinks) {
		vector<manuallink>::iterator first_new_link
			= manuallinks.end() - (manuallinks.size() - initialManualLinks); 
		sort_manuallinks_from_current_line(first_new_link, manuallinks.end());
	}
}

/* viewer function. Handles keyboard actions--main event loop */
int
manualwork()
{
	/* for user's shell commands */
	FILE *pipe;
	/* a temporary buffer */
	char *token;
	/* key, which contains the value entered by user */
	int key = 0;
	/* tmp values */
	int i, selectedchanged;
	int statusline = FREE;
#ifdef getmaxyx
	/* if ncurses, get maxx and maxy */
	getmaxyx(stdscr, maxy, maxx);
	if ((!getenv("MANWIDTH")) ||(manwidthChanged))
	{
		/* set MANWIDTH environment variable */
		static char tmp[24];
		snprintf(tmp, 24, "MANWIDTH=%d", maxx);
		putenv(tmp);
		manwidthChanged = 1;
	}
#else
	maxx = 80;
	/* otherwise hardcode 80x25... */
	maxy = 25;
#endif /* getmaxyx */

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
			if (statusline == FREE)
				showmanualscreen();
			wrefresh(stdscr);
			waitforgetch();
			key = pinfo_getch();
		}
		nodelay(stdscr, FALSE);
		statusline = FREE;
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
				if (yesno(_("Are you sure to print?"), 0))
					printmanual(manual, ManualLines);
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
				token = getstring(_("Enter line: "));
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
				if (token)
				{
					int digit_val = 1;
					for (i = 0; token[i] != 0; i++)
					{
						if (!isdigit(token[i]))
							digit_val = 0;
					}
					/* move cursor position */
					if (digit_val)
					{
						newpos = atol(token);
						newpos -=(maxy - 1);
						if ((newpos >= 0) &&(newpos < ManualLines -(maxy - 2)))
							manualpos = newpos;
						else if (newpos > 0)
							manualpos = ManualLines -(maxy - 2);
						else
							manualpos = 0;
					}
					xfree(token);
					token = 0;
				}
			}
			/*=====================================================*/
			if ((key == keys.shellfeed_1) ||
					(key == keys.shellfeed_2))
			{
				/* get command name */
				curs_set(1);
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				/* get users cmd */
				token = getstring(_("Enter command: "));
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
				pipe = popen(token, "w");
				if (pipe != NULL)
				{
					/* and flush the msg to stdin */
					for (i = 0; i < ManualLines; i++)
						fprintf(pipe, "%s", manual[i]);
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
				/*
				 * searchagain handler. see keys.totalsearch at mainfunction.c
				 * for comments
				 */
				if (!searchagain.search)
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
				/* and search for it in all subsequential lines */
				for (i = manualpos + 1; i < ManualLines - 1; i++)
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
					char* tmp = strdup(tmpstr.c_str());
					if (pinfo_re_exec(tmp))
					{		/* if found, enter here... */
						success = 1;
						xfree(tmp);
						string newtmpstr = manual[i + 1];
						strip_manual(newtmpstr);
						/*
						 * if it was found in the second line of the glued
						 * expression.
						 */
						char* newtmp = strdup(newtmpstr.c_str());
						if (pinfo_re_exec(newtmp))
							manualpos = i + 1;
						else
							manualpos = i;
						xfree(newtmp);
						break;
					}
					xfree(tmp);
				}
				xfree(token);
				rescan_selected();
				if (!success)
				{
					attrset(bottomline);
					mvaddstr(maxy - 1, 0, _("Search string not found..."));
					statusline = LOCKED;
				}

				manual_aftersearch = 1;
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
				selectedchanged = 0;
				/* if there are links at all */
				if (selected != -1)
				{
					/* if one is selected */
					if (selected > 0)
						/*
						 * scan for a next visible one, which is above the
						 * current.
						 */
						for (i = selected - 1; i >= 0; i--)
						{
							if ((manuallinks[i].line >= manualpos) &&
									(manuallinks[i].line < manualpos +(maxy - 1)))
							{
								selected = i;
								selectedchanged = 1;
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
					for (i = 0; i < manuallinks.size(); i++)
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
				manualpos = ManualLines -(maxy - 1);
				if (manualpos < 0)
					manualpos = 0;
				selected = manuallinks.size() - 1;
			}
			/*=====================================================*/
			if ((key == keys.nextnode_1) ||
					(key == keys.nextnode_2))
			{
				for (i = manualpos + 1; i < ManualLines; i++)
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
				for (i = manualpos - 1; i > 0; i--)
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
				if (manualpos +(maxy - 2) < ManualLines -(maxy - 1))
				{
					manualpos +=(maxy - 2);
					rescan_selected();
				}
				else if (ManualLines -(maxy - 1) >= 1)
				{
					manualpos = ManualLines -(maxy - 1);
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
				selectedchanged = 0;
				if (selected < manuallinks.size())
					for (i = selected + 1; i < manuallinks.size(); i++)
					{
						if ((manuallinks[i].line >= manualpos) &&
								(manuallinks[i].line < manualpos +(maxy - 2)))
						{
							selected = i;
							selectedchanged = 1;
							break;
						}
					}
				if (!selectedchanged)
				{
					if (manualpos < ManualLines -(maxy - 1))
						manualpos++;
					if (selected < manuallinks.size())
						for (i = selected + 1; i < manuallinks.size(); i++)
						{
							if ((manuallinks[i].line >= manualpos) &&
									(manuallinks[i].line < manualpos +(maxy - 2)))
							{
								selected = i;
								selectedchanged = 1;
								break;
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
						for (i = selected; i > 0; i--)
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
							for (i = selected; i < manuallinks.size(); i++)
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
						for (i = selected; i > 0; i--)
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
							for (i = selected; i < manuallinks.size(); i++)
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
				if (yesno(_("Are you sure to quit?"), QuitConfirmDefault))
					break;
			}
		}
	}
	closeprogram();
	return -1;
}

void
/* scan for some hyperlink, available on current screen */
rescan_selected()
{
	int i;
	for (i = 0; i < manuallinks.size(); i++)
	{
		if ((manuallinks[i].line >= manualpos) &&
				(manuallinks[i].line < manualpos +(maxy - 1)))
		{
			selected = i;
			break;
		}
	}
}

/*
 * calculate from which to start displaying of manual line *man. Skip `mancol'
 * columns. But remember, that *man contains also nonprinteble characters for
 * boldface etc.
 */
char *getmancolumn(char *man, int mancol)
{
	if (mancol==0) return man;
	while (mancol>0)
	{ if (*(man+1) == 8) man+=3; else man++; mancol--; }
	return man;
}

/* show the currently visible part of manpage */
void
showmanualscreen()
{
	int i;
#ifdef getmaxyx
	/* refresh maxy, maxx values */
	getmaxyx(stdscr, maxy, maxx);
#endif
	attrset(normal);
	/* print all visible text lines */
	for (i = manualpos;(i < manualpos +(maxy - 2)) &&(i < ManualLines); i++)
	{
		int len = strlen(manual[i]);
		if (len)
			manual[i][len - 1] = ' ';
		/* if we have something to display */
		if (len>manualcol)
			mvaddstr_manual((i - manualpos) + 1, 0, getmancolumn(manual[i],manualcol));
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
	if (((manualpos + maxy) < ManualLines) &&(ManualLines > maxy - 2))
		printw(_("Viewing line %d/%d, %d%%"),(manualpos - 1 + maxy), ManualLines,((manualpos - 1 + maxy) * 100) / ManualLines);
	else
		printw(_("Viewing line %d/%d, 100%%"), ManualLines, ManualLines);
	move(maxy - 1, 0);
	attrset(normal);
}

void
/* print a manual line */
mvaddstr_manual(int y, int x, char *str)
{
	int len = strlen(str);
	static string strippedline_string;
	if ((h_regexp_num) ||(manual_aftersearch))
	{
		strippedline_string = str;
		strip_manual(strippedline_string);
	}
	move(y, x);
	for (int i = 0; i < len; i++) {
		if ((i > 0) &&(i < len - 1))
		{
			/* handle bold highlight */
			if ((str[i] == 8) &&(str[i - 1] == '_'))
			{
				attrset(manualbold);
				addch(str[i] & 0xff);
				addch(str[i + 1] & 0xff);
				attrset(normal);
				i++;
				goto label_skip_other;
			}
			/*
			 * if it wasn't bold, check italic, before default, unhighlighted
			 * line will be painted.  We can do it only if i<len-3.
			 */
			else if (i < len - 3)
				goto label_check_italic;
			else /* unhighlighted */
			{
				addch(str[i] & 0xff);
				goto label_skip_other;
			}
		}
		/* italic highlight */
		if (i < len - 3)
		{
label_check_italic:
			if ((str[i + 1] == 8) &&(str[i + 2] == str[i]))
			{
				attrset(manualitalic);
				addch(str[i] & 0xff);
				i += 2;
				attrset(normal);
			}
			else
			{
				addch(str[i] & 0xff);
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
	if ((h_regexp_num) ||(manual_aftersearch))
	{
		regmatch_t pmatch[1];
		int maxregexp = manual_aftersearch ? h_regexp_num + 1 : h_regexp_num;

		/* if it is after search, then we have user defined regexps+
		   a searched regexp to highlight */
		for (int j = 0; j < maxregexp; j++)
		{
			const char* strippedline = strippedline_string.c_str();
			const char* tmpstr = strippedline;
			while (!regexec(&h_regexp[j], tmpstr, 1, pmatch, 0))
			{
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
#endif
}

/* add hyperobject highlights */
void
add_highlights()
{
	int i;
	/* scan through the visible objects */
	for (i = 0; i < manuallinks.size(); i++)
	{
		/* if the object is on the current screen */
		if ((manuallinks[i].line >= manualpos) &&
				(manuallinks[i].line < manualpos +(maxy - 2)))
		{
			/* if it's a simple man link */
			if (manuallinks[i].section_mark < HTTPSECTION)
			{
				if (i == selected)
					attrset(noteselected);
				else
					attrset(note);

				/* if it's a link split into two lines */
				if (manuallinks[i].carry == 1)
				{
					int x, y, ltline = manuallinks[i].line - 1;
					/* find the line, where starts the split link */
					int ltlinelen;
					char *newlinemark;
					string tmp_string = manual[ltline];
					strip_manual(tmp_string);
					char *tmpstr = strdup(tmp_string.c_str());
					/* calculate the length of this line */
					ltlinelen = strlen(tmpstr);
					/* set this var to the last character of this line(to an '\n')*/
					newlinemark = tmpstr + ltlinelen - 1;
					getyx(stdscr, y, x);
					if (y > 2)
					{
#define TestCh tmpstr[ltlinelen]
						/* skip \n, -, and the at least one char... */
						if (ltlinelen > 2)
							ltlinelen -= 3;

						/*
						 * positon ltlinelen to the beginning of the link to be
						 * highlighted
						 */
						while ((isalpha(TestCh)) ||(TestCh == '.') ||(TestCh == '_'))
							ltlinelen--;

						*newlinemark = 0;
						/* OK, link horizontally fits into screen */
						if (ltlinelen>manualcol)
							mvaddstr(manuallinks[i].line - manualpos + 1 - 1,
									ltlinelen-manualcol, &tmpstr[ltlinelen]);
						/*
						 * we cut here a part of the link, and draw only what's
						 * visible on screen
						 */
						else if (ltlinelen+strlen(&tmpstr[ltlinelen])>manualcol)
							mvaddstr(manuallinks[i].line - manualpos + 1 - 1,
									ltlinelen-manualcol, &tmpstr[manualcol]);

						*newlinemark = '\n';
#undef TestCh
					}
					xfree(tmpstr);
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
					char *tmpstr = strdup(tmp_string.c_str());
					char *wsk = tmpstr;
					char *wskend;
					/* skip spaces */
					while (isspace(*wsk))
						wsk++;
					/* find the end of url */
					wskend = findurlend(wsk);
					/* add end of string, and print */
					*wskend = 0;
					if (wsk-tmpstr<manualcol)
						mvaddstr(manuallinks[i].line - manualpos + 2, wsk - tmpstr - manualcol, wsk);
					else if (wskend-tmpstr<manualcol)
						mvaddstr(manuallinks[i].line - manualpos + 2, 0, wsk+manualcol);
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

/* Cleanse a line of backspaces; overwrites argument. */
/* Should probably be rewritten to not overwrite argument. */
void
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
int
is_in_manlinks(string in_str, char *find)
{
	char *copy, *token;
	const char delimiters[] = ":";

	copy = strdup(in_str.c_str());
	if ((strcmp(find,(token = strtok(copy, delimiters))) != 0))
	{
		while ((token = strtok(NULL, delimiters)))
		{
#ifdef HAVE_STRCASECMP
			if (!strcasecmp(token, find))
#else
				if (!strcmp(token, find))
#endif
				{
					xfree((void *) copy);
					return 0;
				}
		}
		xfree((void *) copy);
		return 1;
	}
	else
	{
		xfree((void *) copy);
		return 0;
	}
}

void
printmanual(char **Message, long Lines)
{
	/* printer fd */
	FILE *prnFD;
	int i;

	prnFD = popen(printutility.c_str(), "w");

	/* scan through all lines */
	for (i = 0; i < Lines; i++)
	{
		fprintf(prnFD, "\r%s", Message[i]);
	}
	pclose(prnFD);
}

int
ishyphen(unsigned char ch)
{
	if ((ch == '-') ||(ch == SOFT_HYPHEN))
		return 1;
	return 0;
}
