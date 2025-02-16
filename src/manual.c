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
#include <sys/stat.h>
#include <stdlib.h>

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
/* strips line from formatting characters */
void strip_manual(char *buf);
/*
 * Initialize links in a line .  Links are entries of form reference(section),
 * and are stored in `manuallinks' var, described bellow.
 */
void man_initializelinks(char *line, int carry);
int is_in_manlinks(char *in, char *find);

void printmanual(char **Message, long Lines);

/* line by line stored manual */
char **manual = 0;
/* number of lines in manual */
unsigned ManualLines = 0;
int selected = -1;		/* number of selected link(offset in 'manuallinks',
						   bellow) */
unsigned manualpos = 0;	/* number of the first line, which is painted on
						   screen */

unsigned manualcol = 0;	/* the first displayed column of manpage--
						   for moving the screen left/right */

int manual_aftersearch = 0;	/* this is set if man page is now after search
							   operation */
int manwidthChanged = 0;	/* this flag indicates whether the env variable
							   $MANWIDTH was changed by pinfo */

typedef struct
{
	/* name of a manual */
	char name[128];
	/* section */
	char sect[32];
	/* what was last selected on this page */
	int selected;
	/* what was the last manualpos */
	int pos;
}
manhistory;			/*
					 * type for the `lastread' history entries, when viewing
					 * man pages.
					 */

/* manual lastread history */
manhistory *manualhistory = 0;
/* length of the above table - 1 */
int manualhistorylength = 0;

/* this structure describes a hyperlink in manual viewer */
typedef struct
{			/* struct for hypertext references */
	unsigned int line;		/* line of the manpage, where the reference is */
	/* column of that line */
	unsigned int col;
	/* name of the reference */
	char *name;
	/* section of the reference */
	char section[32];
	int section_mark;
	/* determine whether there is a hyphen above */
	int carry;
}
manuallink;

/* a set of manual references of man page */
manuallink *manuallinks = 0;

/* number of found manual references in man page */
unsigned ManualLinks = 0;

/* semaphore for checking if it's a history(left arrow) call */
int historical = 0;


void
/* free buffers allocated by current man page */
manual_free_buffers()
{
	unsigned int i;
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
	if (manuallinks)
	{				/* links */
		for (i = 0; i < ManualLinks; i++)
		{
			xfree(manuallinks[i].name);
		}
		xfree(manuallinks);
		manuallinks = 0;
		ManualLinks = 0;
		selected = -1;
	}
}

/* initialize history variables for manual pages.  */
void
set_initial_history(char *name)
{
	int len = strlen(name), i;
	char *name1 = strdup(name);

	/* one object of array */
	manualhistory = xmalloc(sizeof(manhistory));
	/* filter trailing spaces */
	while ((len > 1) &&(isspace(name1[len - 1])))
	{
		name1[len - 1] = 0;
		len--;
	}
	i = len;
	/* find the beginning of the last token */
	for (i = len - 1;(i > 0) &&(!isspace(name1[i])); i--);

	/* if we've found space, then we move to the first nonspace character */
	if (i > 0)
		i++;

	/* filename->name */
	strcpy(manualhistory[0].name, &name1[i]);

	/* It is essential to know the section name, as otherwise links in the
	 * man page to the same name but different section would be ignored -
	 * see man_initializelinks. An example is sleep(1) which has link to
	 * sleep(3). So we try to find the section directly from man. */
	if (manualhistory[0].sect[0] == 0) {
		char buf[1024];
		char *str, *lastSlash, *lastButOneSlash;
		FILE *pathFile;
		snprintf(buf, sizeof(buf), "man -w -W %s %s", ManOptions, name);
		pathFile = popen(buf, "r");
		if (fgets(buf, sizeof(buf), pathFile)==NULL)
		{
			pclose(pathFile);
			/* Try without -W */
			snprintf(buf, sizeof(buf), "man -w %s %s", ManOptions, name);
			pathFile = popen(buf, "r");
			if (fgets(buf, sizeof(buf), pathFile)==NULL)
			{
				fprintf(stderr, "Error executing command '%s'\n", buf);
				exit(1);
			}
		}
		pclose(pathFile);
		/* buf will be of the form "/usr/share/man/man1/sleep.1.gz". We
		 * find the section from the leaf directory "/man1" */
		for (str = buf, lastSlash = str, lastButOneSlash = 0; *str; ++str) {
			if (*str == '/') {
				lastButOneSlash = lastSlash;
				lastSlash = str;
			}
		}
		if (lastButOneSlash) {
			*lastSlash = 0; /* terminate the section */
			lastButOneSlash += 4; /* skip "/man", and land on the section */
			strncpy(manualhistory[0].sect, lastButOneSlash, sizeof(manualhistory[0].sect)-1);
		}
	}

	/* selected unknown */
	manualhistory[0].selected = -1;
	/* pos=0 */
	manualhistory[0].pos = 0;
	free(name1);
}

/* construct man name; take care about carry */
void
construct_manualname(char *buf, int which)
{
	if (!manuallinks[which].carry)
	{
		/* workaround for names starting with '(' */
		if (manuallinks[which].name[0] == '(') strcpy(buf, manuallinks[which].name + 1);
		else strcpy(buf, manuallinks[which].name);
		return;
	}
	else
	{
		/* normal manual reference */
		if (manuallinks[which].section_mark < HTTPSECTION)
		{
			char *base = xmalloc(1024);
			char *ptr;
			int tmppos;
			strncpy(base, manual[manuallinks[which].line - 1],1023);
			strip_manual(base);
			ptr = base + strlen(base) - 3;
			while (((isalpha(*ptr)) ||(*ptr == '.') ||(*ptr == '_')) &&(ptr > base))
				ptr--;
			/* workaround for man pages with leading '(' see svgalib man pages */
			if (*ptr == '(')
				ptr++;
			strcpy(buf, ptr);
			tmppos = strlen(buf);
			if (tmppos > 1)
				buf[tmppos - 2] = 0;
			strcat(buf, manuallinks[which].name);
			xfree(base);
		}
		/* url reference */
		else
		{
			char *base = xmalloc(1024);
			char *ptr, *eptr;
			int namelen = strlen(manuallinks[which].name);
			strcpy(base, manual[manuallinks[which].line + 1]);
			strip_manual(base);
			ptr = base;
			/* skip whitespace */
			while (isspace(*ptr))
				ptr++;
			eptr = findurlend(ptr);
			*eptr = 0;
			strcpy(buf, manuallinks[which].name);
			/* cut the hyphen */
			buf[namelen - 1] = 0;
			strcat(buf, ptr);
			xfree(base);
		}
	}
}

/* this is something like main() function for the manual viewer code.  */
int
handlemanual(char *name)
{
	int return_value = 0;
	struct stat statbuf;
	FILE *id;

	char manualname[256];
	char cmd[4096];
	char *raw_tempfilename = 0;
	char *apropos_tempfilename = 0;

	if (tmpfilename1)
	{
		unlink(tmpfilename1);
		xfree(tmpfilename1);
	}
	tmpfilename1 = make_tempfile();

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
#ifdef NIETS
	/****************************************************************************
	 *                    Ignore macros part: BEGIN                             *
	 * PS: Siewca: I still expect that you'll isolate it to a single procedure  *
	 * Description(by PB): This code opens a manpage file, and filters it from *
	 * dangerous macros. The output is put into a temporary file, which is then *
	 * used as the `name' filename argument of this(handlemanual) procedure.   *
	 * There is a stored variable raw_tempfilename to allow unlinking this temp *
	 * file after usage							    *
	 ****************************************************************************/
	FILE *source;
	char **ignored_entries;
	char location[256];
	char line[1025];
	char *end, *prev;
	size_t macroline_size;
	int ignored_items = 0, i = 0;
	char zipped = 0;

	/* if the pointer is non-null */
	if (ignoredmacros)
		/* if there are some macros */
		if (*ignoredmacros && strlen(ignoredmacros))
		{				/* that should be ignored   */
			*location = '\0';
			/* we need to know the path */
			snprintf(cmd, 255, "man -W %s %s",
					ManOptions,
					name);
			id = popen(cmd, "r");
			if (!id)
			{
				printf(_("Error: Cannot call man command.\n"));
				return 1;
			}
			fflush(id);
			fgets(location, 255, id);
			pclose(id);

			if (*location == '\0')
			{
				printf(_("Error: No manual page found either.\n"));
				if (use_apropos)
				{
					printf(_("Apropos pages:\n"));
					snprintf(cmd, 255, "apropos %s|cat %s", name, StderrRedirection);
					xsystem(cmd);
				}
				return 1;
			}


			ignored_items++;
			prev = ignoredmacros;
			/* counting items */
			while ((end = strchr(prev, ':')))
			{
				ignored_items++;
				prev = end + 1;
			}

			ignored_entries =(char **) xmalloc(ignored_items * sizeof(char **));
			ignored_entries[0] = ignoredmacros;
			prev = ignoredmacros;
			i = 0;
			/* creating pointers */
			while ((end = strchr(prev, ':')))
			{
				*end = '\0';
				prev = end + 1;
				i++;
				ignored_entries[i] = prev;
			}

			/* removing newline */
			if ((prev = rindex(location, '\n')))
				*prev = '\0';

			/* checking if it's compressed */
			prev = index(location, '\0');
			if ((strlen(location)) > 3
					&&((*(prev - 1) == 'Z' && *(prev - 2) == '.')
						||(*(prev - 1) == 'z' && *(prev - 2) == 'g' && *(prev - 3) == '.')
					   )
			   )
			{
				if (verbose)
					printf("%s %s\n", _("Calling gunzip for"), location);
				snprintf(cmd, 255, "gunzip -c %s", location);
				source = popen(cmd, "r");
				zipped = 1;
				if (!source)
				{
					printf(_("Couldn't call gunzip.\n"));
					return 1;
				}
			}
			else /* from cmd output  */
				source = fopen(location, "r");
			name = make_tempfile();
			raw_tempfilename = name;
			id = fopen(name, "w");

			/* we read until eof */
			while (!feof(source))
			{
				if (fgets(line, 1024, source) == NULL)
					line[0] = '\0';

				/* macro starts with a dot*/
				if (line[0] != '.' ||(strlen(line)) <(size_t) 2)
				{
					fprintf(id, "%s", line);
					continue;
				}
				else
					while (i >= 0)
					{
						macroline_size = strlen(ignored_entries[i]);
						if (strlen(line + 1) < macroline_size)
							macroline_size = strlen(line + 1);
						if ((strncmp(ignored_entries[i], line + 1, macroline_size)) == 0
								&&(*(line + 1 +(int) macroline_size) == ' '
									|| *(line + 1 +(int) macroline_size) == '\n'
									|| *(line + 1 +(int) macroline_size) == '\t'))
						{
							if (quote_ignored)
							{
								if ((prev = rindex(line, '\n')))
									*prev = '\0';
								sprintf(cmd, "\n.br\n.nf\n[ [pinfo] - %s: %.42s", _("IGNORING"), line);
								if ((strlen(line)) >(size_t) 42)
									strcat(cmd, "(...)]\n.fi\n");
								else
									strcat(cmd, " ]\n.fi\n");
							}
							else
							{
								sprintf(cmd, ".\\\" removed macro: %.42s", line);
								if ((strlen(line)) >(size_t) 42)
									strcat(cmd, "(...)");
							}
							strcpy(line, cmd);
							break;
						}
						i--;
					}

				fprintf(id, "%s", line);
				i = ignored_items - 1;
			}			/* while (!feof(source)) */
			if (zipped)
				pclose(source);
			else
				fclose(source);
			fclose(id);
			free(ignored_entries);
		}				/* if (ignored_macros... */
	/****************************************************************************
	 *                    Ignore macros part: END                               *
	 ****************************************************************************/
#endif
	if (!plain_apropos)
	{
		snprintf(cmd, 255, "man %s %s %s > %s",
				ManOptions,
				name,
				StderrRedirection,
				tmpfilename1);
	}

	if (plain_apropos || (system_check(cmd) != 0))
	{
		if (!plain_apropos)
		{
			unlink(tmpfilename1);
			printf(_("Error: No manual page found\n"));
		}
		plain_apropos = 0;
		if (use_apropos)
		{
			printf(_("Calling apropos \n"));
			apropos_tempfilename = make_tempfile();
			snprintf(cmd, 4096, "apropos %s > %s", name, apropos_tempfilename);
			if (system_check(cmd) != 0)
			{
				printf(_("Nothing appropriate\n"));
				unlink(apropos_tempfilename);
				return 1;
			}
			id = fopen(apropos_tempfilename, "r");
		}
		else
			return 1;
	}
	else
		id = fopen(tmpfilename1, "r");
	init_curses();


	set_initial_history(name);
	/* load manual to memory */
	loadmanual(id);
	fclose(id);
	do
	{
		/* manualwork handles all actions when viewing man page */
		return_value = manualwork();
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
			tmpfilename2 = make_tempfile();
			/*
			 * key_back is not pressed; and return_value is an offset to
			 * manuallinks
			 */
			if (return_value != -2)
			{
				construct_manualname(manualname, return_value);
				snprintf(cmd, 4096, "man %s %s %s %s > %s",
						ManOptions,
						manuallinks[return_value].section,
						manualname,
						StderrRedirection,
						tmpfilename2);
			}
			else /* key_back was pressed */
			{
				manualhistorylength--;
				if (manualhistorylength == 0 && apropos_tempfilename)
				{
					id = fopen(apropos_tempfilename, "r");
					loadmanual(id);
					fclose(id);
					continue;
				}
				if (manualhistory[manualhistorylength].sect[0] == 0)
					snprintf(cmd, 255, "man %s %s %s > %s",
							ManOptions,
							manualhistory[manualhistorylength].name,
							StderrRedirection,
							tmpfilename2);
				else
					snprintf(cmd, 255, "man %s %s %s %s > %s",
							ManOptions,
							manualhistory[manualhistorylength].sect,
							manualhistory[manualhistorylength].name,
							StderrRedirection,
							tmpfilename2);
				/*
				 * flag to make sure, that
				 * manualwork will refresh the variables manualpos and selected
				 * when going back to this page
				 */
				historical = 1;
			}
			xsystem(cmd);
			stat(tmpfilename2, &statbuf);
			if (statbuf.st_size > 0)
			{
				snprintf(cmd, 255, "mv %s %s", tmpfilename2, tmpfilename1);
				/* create tmp file containing man page */
				xsystem(cmd);
				/* open man page */
				id = fopen(tmpfilename1, "r");
				if (id != NULL)
				{
					/* now we create history entry for new page */
					if (!historical)
					{
						manualhistorylength++;
						manualhistory = xrealloc(manualhistory,(manualhistorylength + 2) * sizeof(manhistory));
						/*
						 * we can write so since this code applies
						 * only when it's not a history call
						 */
						strcpy(manualhistory[manualhistorylength].name,
								manualname);
						strcpy(manualhistory[manualhistorylength].sect,
								manuallinks[return_value].section);
					}
					/* loading manual page and its defaults... */
					loadmanual(id);
					fclose(id);
					/* continuing with creation of history */
					if (!historical)
					{
						manualhistory[manualhistorylength].pos = manualpos;
						manualhistory[manualhistorylength].selected = selected;
					}
					else
						historical = 0;
				}
				else
					return_value = -1;
			}
		}
	}
	while (return_value != -1);
	if (apropos_tempfilename)
		unlink(apropos_tempfilename);
	/* we were using temporary */
	if (raw_tempfilename)
		unlink(raw_tempfilename);
	/* raw-manpage for scaning */
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
	manual = xmalloc(sizeof(char *));
	manuallinks = xmalloc(sizeof(manuallinks));
	manual[ManualLines] = xmalloc(1024);

	/* we read until eof */
	while (!feof(id))
	{
		char *tmp;
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
			manual[ManualLines] = xrealloc(manual[ManualLines],
					manlinelen + 10);

			/* temporary variable for determining hypertextuality of fields */
			tmp = xmalloc(manlinelen + 10);

			strcpy(tmp, manual[ManualLines]);

			/* remove formatting chars */
			strip_manual(tmp);
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
			manual = xrealloc(manual,(ManualLines + 5) * sizeof(char *));
			manual[ManualLines] = xmalloc(1024);
		}
	}

}

int
compare_manuallink(const void *a, const void *b)
{
	return ((manuallink *) a)->col -((manuallink *) b)->col;
}

void
sort_manuallinks_from_current_line(long startlink, long endlink)
{
	qsort(manuallinks + startlink, endlink - startlink, sizeof(manuallink), compare_manuallink);
}


/* initializes hyperlinks in manual */
void
man_initializelinks(char *tmp, int carry)
{
	/* set tmpcnt to the trailing zero of tmp */
	int tmpcnt = strlen(tmp) + 1;
	char *mylink = tmp;
	char *urlstart, *urlend;
	unsigned initialManualLinks = ManualLinks;
	int i, b;
	/******************************************************************************
	 * handle url refrences                                                       *
	 *****************************************************************************/
	urlend = tmp;
	while ((urlstart = strstr(urlend, "http://")) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallinks = xrealloc(manuallinks, sizeof(manuallink) *(ManualLinks + 3));
		manuallinks[ManualLinks].line = ManualLines;
		manuallinks[ManualLinks].col = width_of_string(tmp, urlstart - tmp);
		strcpy(manuallinks[ManualLinks].section, "HTTPSECTION");
		manuallinks[ManualLinks].section_mark = HTTPSECTION;
		manuallinks[ManualLinks].name = xmalloc(urlend - urlstart + 10);
		strncpy(manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
		manuallinks[ManualLinks].name[urlend - urlstart] = 0;
		if (ishyphen(manuallinks[ManualLinks].name[urlend - urlstart - 1]))
			manuallinks[ManualLinks].carry = 1;
		else
			manuallinks[ManualLinks].carry = 0;
		ManualLinks++;
	}
	urlend = tmp;
	while ((urlstart = strstr(urlend, "ftp://")) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallinks = xrealloc(manuallinks, sizeof(manuallink) *(ManualLinks + 3));
		manuallinks[ManualLinks].line = ManualLines;
		manuallinks[ManualLinks].col = width_of_string(tmp, urlstart - tmp);
		strcpy(manuallinks[ManualLinks].section, "FTPSECTION");
		manuallinks[ManualLinks].section_mark = FTPSECTION;
		manuallinks[ManualLinks].name = xmalloc(urlend - urlstart + 10);
		strncpy(manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
		manuallinks[ManualLinks].name[urlend - urlstart] = 0;
		if (ishyphen(manuallinks[ManualLinks].name[urlend - urlstart - 1]))
			manuallinks[ManualLinks].carry = 1;
		else
			manuallinks[ManualLinks].carry = 0;
		ManualLinks++;
	}
	urlend = tmp;
	while ((urlstart = findemailstart(urlend)) != NULL)
	{
		/* always successfull */
		urlend = findurlend(urlstart);
		manuallinks = xrealloc(manuallinks, sizeof(manuallink) *(ManualLinks + 3));
		manuallinks[ManualLinks].line = ManualLines;
		manuallinks[ManualLinks].col = width_of_string(tmp, urlstart - tmp);
		strcpy(manuallinks[ManualLinks].section, "MAILSECTION");
		manuallinks[ManualLinks].section_mark = MAILSECTION;
		manuallinks[ManualLinks].name = xmalloc(urlend - urlstart + 10);
		strncpy(manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
		manuallinks[ManualLinks].name[urlend - urlstart] = 0;
		if (ishyphen(manuallinks[ManualLinks].name[urlend - urlstart - 1]))
			manuallinks[ManualLinks].carry = 1;
		else
			manuallinks[ManualLinks].carry = 0;

		/* there should be a dot in e-mail domain */
		if (strchr(manuallinks[ManualLinks].name, '.') != NULL)
			ManualLinks++;
	}
	/******************************************************************************
	 * handle normal manual refrences -- reference(section)                       *
	 ******************************************************************************/
	do
	{
		/* we look for '(', since manual link */
		mylink = strchr(mylink, '(');
		/* has form of  'blah(x)' */
		if (mylink != NULL)
		{
			char *temp;
			/* look for the closing bracket */
			if ((temp = strchr(mylink, ')')))
			{
				char *p_t1, *p_t;
				p_t = p_t1 = xmalloc((strlen(mylink) + 10) * sizeof(char));
				for (++mylink; mylink != temp; *p_t++ = *mylink++);
				*p_t = '\0';
				mylink -=(strlen(p_t1) + sizeof(char));

				if ((!strchr(p_t1, '(')) &&(!is_in_manlinks(manlinks, p_t1)))
				{
					char tempchar;
					int breakpos, cols_before_link;
					i = mylink - tmp - 1;
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

					/* calculate the number of columns in front of the link */
					cols_before_link = width_of_string(tmp, i-1);

					/* a small check */
					if (!((use_apropos) &&(manualhistorylength == 0)))
					{
						/*
						 * In English: if the name of the link is the name of
						 * the current page and the section of the link is the
						 * current section or if we don't know the current
						 * section, then...
						 */
						if ((!strcasecmp(&tmp[i], manualhistory[manualhistorylength].name))
								&&((!strcasecmp(p_t1, manualhistory[manualhistorylength].sect))
									||(manualhistory[manualhistorylength].sect[0] == 0)
									||(!strcmp(manualhistory[manualhistorylength].sect, " "))))

							break;
					}
					manuallinks = xrealloc(manuallinks, sizeof(manuallink) *(ManualLinks + 3));
					manuallinks[ManualLinks].line = ManualLines;
					manuallinks[ManualLinks].col = cols_before_link + 1;
					if (LongManualLinks)
					{
						for (b = 1; mylink[b] != ')'; b++)
							manuallinks[ManualLinks].section[b - 1] = tolower(mylink[b]);
						manuallinks[ManualLinks].section[b - 1] = 0;
					}
					else
					{
						manuallinks[ManualLinks].section[0] = mylink[1];
						manuallinks[ManualLinks].section[1] = 0;
					}
					manuallinks[ManualLinks].section_mark = 0;
					manuallinks[ManualLinks].name = xmalloc((breakpos - i) + 10);
					strcpy(manuallinks[ManualLinks].name, tmp + i);
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
						manuallinks[ManualLinks].carry = 0;
					else
						manuallinks[ManualLinks].carry = carry;
					/* increase the number of entries */
					ManualLinks++;
				}		/*... if (in man links) */
				xfree((void *) p_t1);
			}
		}
		if (mylink)
			mylink++;
		if (mylink >(tmp + tmpcnt))
		{
			break;
		}
	}
	/* do this line until strchr() won't find a '(' in string */
	while (mylink != NULL);
	if (initialManualLinks != ManualLinks)
		sort_manuallinks_from_current_line(initialManualLinks, ManualLinks);
}

/* viewer function. Handles keyboard actions--main event loop */
int
manualwork()
{
	/* for user's shell commands */
	FILE *mypipe;
	/* a temporary buffer */
	char *token;
	/* key, which contains the value entered by user */
	int key = 0;
	/* tmp values */
	int selectedchanged;
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
	manualpos = manualhistory[manualhistorylength].pos;
	/* if there was a valid selected entry, apply it */
	if (manualhistory[manualhistorylength].selected != -1)
		selected = manualhistory[manualhistorylength].selected;
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
		if (key > 0)
		{
			if ((key == keys.print_1) ||
					(key == keys.print_2))
			{
				if (yesno(_("Are you sure you want to print?"), 0))
					printmanual(manual, ManualLines);
			}
			/*====================================================*/
			if ((key == keys.goto_1) ||
					(key == keys.goto_2))
			{
				manuallinks = xrealloc(manuallinks,(ManualLinks + 1) *(sizeof(manuallink) + 3));

				/* get user's value */
				attrset(bottomline);
				move(maxy - 1, 0);
				echo();
				curs_set(1);
				manuallinks[ManualLinks].name = getstring(_("Enter manual name: "));
				curs_set(0);
				noecho();
				move(maxy - 1, 0);
#ifdef HAVE_DECL_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);

				manuallinks[ManualLinks].carry = 0;
				manuallinks[ManualLinks].section_mark = 0;
				strcpy(manuallinks[ManualLinks].section, " ");
				manuallinks[ManualLinks].line = -1;
				manuallinks[ManualLinks].col = -1;
				ManualLinks++;
				return ManualLinks - 1;
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
#ifdef HAVE_DECL_BKGDSET
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
					for (unsigned i = 0; token[i] != 0; i++)
					{
						if (!isdigit(token[i]))
							digit_val = 0;
					}
					/* move cursor position */
					if (digit_val)
					{
						newpos = atol(token);
						newpos -=(maxy - 1);
						if ((newpos >= 0) && ((unsigned long) newpos < ManualLines - (maxy - 2)))
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
#ifdef HAVE_DECL_BKGDSET
				bkgdset(' ' | bottomline);
				clrtoeol();
				bkgdset(0);
#else
				myclrtoeol();
#endif
				attrset(normal);

				myendwin();
				xsystem("clear");
				/* open mypipe */
				mypipe = popen(token, "w");
				if (mypipe != NULL)
				{
					/* and flush the msg to stdin */
					for (unsigned i = 0; i < ManualLines; i++)
						fprintf(mypipe, "%s", manual[i]);
					pclose(mypipe);
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
				move(maxy - 1, 0);
#ifdef HAVE_DECL_BKGDSET
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
					/* We're not in a search! */
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
				/* and search for it in all subsequential lines */
				for (unsigned i = manualpos + 1; i < ManualLines - 1; i++)
				{
					char *tmp;
					tmp = xmalloc(strlen(manual[i]) + strlen(manual[i + 1]) + 10);
					/*
					 * glue two following lines together, to find expres- sions
					 * split up into two lines
					 */
					strcpy(tmp, manual[i]);
					strcat(tmp, manual[i + 1]);
					strip_manual(tmp);

					/* execute search */
					if (pinfo_re_exec(tmp))
					{		/* if found, enter here... */
						success = 1;
						strcpy(tmp, manual[i + 1]);
						strip_manual(tmp);
						/*
						 * if it was found in the second line of the glued
						 * expression.
						 */
						if (pinfo_re_exec(tmp))
							manualpos = i + 1;
						else
							manualpos = i;
						xfree(tmp);
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
						for (int i = selected - 1; i >= 0; i--)
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
					for (unsigned i = 0; i < ManualLinks; i++)
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
				if (ManualLines < maxy - 1)
					manualpos = 0;
				else
					manualpos = ManualLines -(maxy - 1);

				selected = ManualLinks - 1;
			}
			/*=====================================================*/
			if ((key == keys.nextnode_1) ||
					(key == keys.nextnode_2))
			{
				for (unsigned i = manualpos + 1; i < ManualLines; i++)
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
				for (unsigned i = manualpos - 1; i > 0; i--)
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
					selected = ManualLinks - 1;
				}
				else
				{
					manualpos = 0;
					selected = ManualLinks - 1;
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
				for (unsigned i = selected + 1; i < ManualLinks; i++)
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
					for (unsigned i = selected + 1; i < ManualLinks; i++)
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
				if (manualhistorylength)
					return -2;
			}
			/*=====================================================*/
			if ((key == keys.followlink_1) ||
					(key == keys.followlink_2))
			{
				manualhistory[manualhistorylength].pos = manualpos;
				manualhistory[manualhistorylength].selected = selected;
				if (selected >= 0)
					if ((manuallinks[selected].line >= manualpos) &&
							(manuallinks[selected].line < manualpos +(maxy - 1)))
					{
						if (!strncmp(manuallinks[selected].section, "HTTPSECTION", 11))
						{
							int buflen;
							char *tempbuf = xmalloc(1024);
							strcpy(tempbuf, httpviewer);
							strcat(tempbuf, " ");
							buflen = strlen(tempbuf);
							construct_manualname(tempbuf + buflen, selected);
							myendwin();
							xsystem(tempbuf);
							doupdate();
							xfree(tempbuf);
						}
						else if (!strncmp(manuallinks[selected].section, "FTPSECTION", 10))
						{
							int buflen;
							char *tempbuf = xmalloc(1024);
							strcpy(tempbuf, ftpviewer);
							strcat(tempbuf, " ");
							buflen = strlen(tempbuf);
							construct_manualname(tempbuf + buflen, selected);
							myendwin();
							xsystem(tempbuf);
							doupdate();
							xfree(tempbuf);
						}
						else if (!strncmp(manuallinks[selected].section, "MAILSECTION", 11))
						{
							int buflen;
							char *tempbuf = xmalloc(1024);
							strcpy(tempbuf, maileditor);
							strcat(tempbuf, " ");
							buflen = strlen(tempbuf);
							construct_manualname(tempbuf + buflen, selected);
							myendwin();
							xsystem(tempbuf);
							doupdate();
							xfree(tempbuf);
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
						for (int i = selected; i >= 0; i--)
						{
							if (manuallinks[i].line == mouse_y + manualpos - 1)
							{
								if (manuallinks[i].col <= mouse_x - 1)
								{
									if (manuallinks[i].col + strlen(manuallinks[i].name) >= mouse_x - 1)
									{
										selected = i;
										done = 1;
										break;
									}
								}
							}
						}
						if (!done)
							for (unsigned i = selected; i < ManualLinks; i++)
							{
								if (manuallinks[i].line == mouse_y + manualpos - 1)
								{
									if (manuallinks[i].col <= mouse_x - 1)
									{
										if (manuallinks[i].col + strlen(manuallinks[i].name) >= mouse_x - 1)
										{
											selected = i;
											done = 1;
											break;
										}
									}
								}
							}
					}		/* end: mouse not on top/bottom line */
					if (mouse_y == 0)
						ungetch(keys.up_1);
					if (mouse_y == maxy - 1)
						ungetch(keys.down_1);
				}		/* end: button_clicked */
				if (mouse.bstate == BUTTON1_DOUBLE_CLICKED)
				{
					if ((mouse_y > 0) &&(mouse_y < maxy - 1))
					{
						for (int i = selected; i >= 0; i--)
						{
							if (manuallinks[i].line == mouse_y + manualpos - 1)
							{
								if (manuallinks[i].col <= mouse_x - 1)
								{
									if (manuallinks[i].col + strlen(manuallinks[i].name) >= mouse_x - 1)
									{
										selected = i;
										done = 1;
										break;
									}
								}
							}
						}
						if (!done)
							for (unsigned i = selected; i < ManualLinks; i++)
							{
								if (manuallinks[i].line == mouse_y + manualpos - 1)
								{
									if (manuallinks[i].col <= mouse_x - 1)
									{
										if (manuallinks[i].col + strlen(manuallinks[i].name) >= mouse_x - 1)
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
					if (mouse_y == 0)
						ungetch(keys.pgup_1);
					if (mouse_y == maxy - 1)
						ungetch(keys.pgdn_1);
				}		/* end: button doubleclicked */
			}
#endif /* CURSES_MOUSE */
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

void
/* scan for some hyperlink, available on current screen */
rescan_selected()
{
	for (unsigned i = 0; i < ManualLinks; i++)
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
#ifdef getmaxyx
	/* refresh maxy, maxx values */
	getmaxyx(stdscr, maxy, maxx);
#endif
	attrset(normal);
	/* print all visible text lines */
	for (unsigned i = manualpos;(i < manualpos +(maxy - 2)) &&(i < ManualLines); i++)
	{
		size_t len = strlen(manual[i]);
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
#ifdef HAVE_DECL_BKGDSET
	bkgdset(' ' | normal);
#endif
	/* and clear to bottom */
	clrtobot();
#ifdef HAVE_DECL_BKGDSET
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
	int i, j, len = strlen(str);
	static char strippedline[1024];
	if ((h_regexp_num) ||(manual_aftersearch))
	{
		memcpy(strippedline, str, len + 1);
		strip_manual(strippedline);
	}
	move(y, x);
	for (i = 0; i < len; i++)
	{
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
#ifdef HAVE_DECL_BKGDSET
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
		for (j = 0; j < maxregexp; j++)
		{
			char *tmpstr = strippedline;
			while (!regexec(&h_regexp[j], tmpstr, 1, pmatch, 0))
			{
				int n = pmatch[0].rm_eo - pmatch[0].rm_so;
				int rx = pmatch[0].rm_so + tmpstr - strippedline;
				int curY, curX;
				char tmpchr;
				getyx(stdscr, curY, curX);
				tmpchr = strippedline[rx + n];
				strippedline[rx + n] = 0;
				attrset(searchhighlight);
				mvaddstr(y, rx, strippedline + rx);
				attrset(normal);
				strippedline[rx + n] = tmpchr;
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
	for (i = 0; (unsigned) i < ManualLinks; i++)
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
					char *tmpstr = strdup(manual[ltline]);
					size_t ltlinelen;
					char *newlinemark;
					/* remove boldfaces&italics */
					strip_manual(tmpstr);
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
					char *tmpstr = strdup(manual[ltline]);
					unsigned long k = 0;
					char *wskend;
					strip_manual(tmpstr);
					/* skip spaces */
					while (isspace(tmpstr[k]))
						k++;
					/* find the end of url */
					wskend = findurlend(tmpstr+k);
					if (wskend<tmpstr) abort(); /* TODO: crude check, but this should never occur anyway */

					/* add end of string, and print */
					*wskend = 0;
					if (k<manualcol)
						mvaddstr(manuallinks[i].line - manualpos + 2, k - manualcol, tmpstr+k);
					else if ((uint64_t) (wskend-tmpstr)<manualcol) /* should be safe see check above */
						mvaddstr(manuallinks[i].line - manualpos + 2, 0, tmpstr+k+manualcol);
				}
			}
			if (manuallinks[i].col>manualcol)
				mvaddstr(1 + manuallinks[i].line - manualpos,
						manuallinks[i].col - manualcol, manuallinks[i].name);
			else if (manuallinks[i].col+strlen(manuallinks[i].name)>manualcol)
				mvaddstr(1 + manuallinks[i].line - manualpos, 0,
						manuallinks[i].name+(manualcol-manuallinks[i].col));
			attrset(normal);
		}
	}
}

/* all variables passed here must have, say 10 bytes of overrun buffer */
void
strip_manual(char *buf)
{
	int i, tmpcnt = 0;
	/* in general, tmp buffer will hold a line stripped from highlight marks */
	for (i = 0; buf[i] != 0; i++)
	{
		/* so we strip the line from "'_',0x8" -- bold marks */
		if ((buf[i] == '_') &&(buf[i + 1] == 8))
		{
			buf[tmpcnt++] = buf[i + 2];
			i += 2;
		}
		/* and 0x8 -- italic marks */
		else if ((buf[i + 1] == 8) &&(buf[i + 2] == buf[i]))
		{
			buf[tmpcnt++] = buf[i];
			i += 2;
		}
		else /* else we don't do anything */
			buf[tmpcnt++] = buf[i];
	}
	buf[tmpcnt] = 0;
}

/*
 * checks if a construction, which looks like hyperlink, belongs to the allowed
 * manual sections.
 */
int
is_in_manlinks(char *in, char *find)
{
	char *copy, *token;
	const char delimiters[] = ":";

	copy = strdup(in);
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

	prnFD = popen(printutility, "w");

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
