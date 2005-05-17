#include "common_includes.h"

RCSID ("$Id$")

#include <ctype.h>
#include <sys/stat.h>

#define HTTPSECTION 100
#define FTPSECTION 101
#define MAILSECTION 102

     int ishyphen (unsigned char ch);	/* check if a char is a hyphen character */
     void loadmanual (FILE * id);	/* load manual */
     int manualwork ();		/* handle keyboard */
     void rescan_selected ();	/* scan for potential link to select on
				   viewed manual page */
     void showmanualscreen ();	/* self explanatory */
     void mvaddstr_manual (int y, int x, char *str);	/* mvaddstr with bold/italic */
     void add_highlights ();	/* adds highlights to a painted screen */
     void strip_manual (char *buf);	/* strips line from formatting characters */
     void man_initializelinks (char *line, int carry);
 /* 
  * Initialize links in a line .
  * Links are entries of form
  * reference (section), and are stored
  * in `manuallinks' var, described 
  * bellow.
  */
     int is_in_manlinks (char *in, char *find);

     void printmanual (char **Message, long Lines);

     char **manual = 0;		/* line by line stored manual */
     int ManualLines = 0;	/* number of lines in manual */
     int selected = -1;		/* number of selected link (offset in 'manuallinks',
				   bellow) */
     int manualpos = 0;		/* number of the first line, which is painted on
				   screen */

     int manualcol = 0;		/* the first displayed column of manpage--
     				   for moving the screen left/right */				   

     int manual_aftersearch = 0;	/* this is set if man page is now after search 
					   operation */
     int manwidthChanged = 0;	/* this flag indicates whether the env variable
				   $MANWIDTH was changed by pinfo */

     typedef struct
       {
	 char name[256];	/* name of a manual */
	 char sect[32];		/* section */
	 int selected;		/* what was last selected on this page */
	 int pos;		/* what was the last manualpos */
       }
manhistory;			/* 
				 * type for the `lastread' history entries, when viewing
				 * man pages.
				 */

     manhistory *manualhistory = 0;	/* manual lastread history */
     int manualhistorylength = 0;	/* length of the above table - 1 */

     typedef struct
       {			/* struct for hypertext references */
	 int line;		/* line of the manpage, where the reference
				   is */
	 int col;		/* column of that line */
	 char *name;		/* name of the reference */
	 char section[32];	/* section of the reference */
	 int section_mark;
	 int carry;		/* determine whether there is a hyphen above */
       }
manuallink;			/*
				 * this structure describes a hyperlink
				 * in manual viewer
				 */

     manuallink *manuallinks = 0;	/* a set of manual references of man page */

     int ManualLinks = 0;	/* number of found manual references in man page */

     int historical = 0;	/* semaphore for checking if it's a history (left
				   arrow) call */


     void
       manual_free_buffers ()	/* free buffers allocated by current man page */
{
  int i;
  if (manual)			/* first free previously allocated memory */
    {				/* for the manual itself... */
      for (i = 0; i <= ManualLines; i++)
	{
	  xfree (manual[i]);
	}
      xfree (manual);
      manual = 0;
      ManualLines = 0;
    }
  if (manuallinks)		/* ...and for the list of manual hypertext */
    {				/* links */
      for (i = 0; i < ManualLinks; i++)
	{
	  xfree (manuallinks[i].name);
	}
      xfree (manuallinks);
      manuallinks = 0;
      ManualLinks = 0;
      selected = -1;
    }
}

void
set_initial_history (char *name)	/* 
					 * initialize history variables for 
					 * manual pages.
					 */
{
  int len = strlen (name), i;
  char *name1 = strdup (name);
  manualhistory = xmalloc (sizeof (manhistory));	/* one object of
							   array */
  while ((len > 1) && (isspace (name1[len - 1])))	/* filter trailing spaces */
    {
      name1[len - 1] = 0;
      len--;
    }
  i = len;
  for (i = len - 1; (i > 0) && (!isspace (name1[i])); i--);	/* find the beginning of the last token */

  if (i > 0)
    i++;			/* if we've found space, then we move to the first nonspace character */

  strcpy (manualhistory[0].name, &name1[i]);	/* filename->name */
  strcpy (manualhistory[0].sect, "");	/* section unknown */
  manualhistory[0].selected = -1;	/* selected unknown */
  manualhistory[0].pos = 0;	/* pos=0 */
  free (name1);
}

void
construct_manualname (char *buf, int which)	/* construct man name; take
						   care about carry */
{
  if (!manuallinks[which].carry)
    {
    /* workaround for names starting with '(' */
      if (manuallinks[which].name[0] == '(') strcpy(buf, manuallinks[which].name + 1);
      else strcpy (buf, manuallinks[which].name);
      return;
    }
  else
    {
      if (manuallinks[which].section_mark < HTTPSECTION)	/* normal manual reference */
	{
	  char *base = xmalloc (1024);
	  char *ptr;
	  int tmppos;
	  strcpy (base, manual[manuallinks[which].line - 1]);
	  strip_manual (base);
	  ptr = base + strlen (base) - 3;
	  while (((isalpha (*ptr)) || (*ptr == '.') || (*ptr == '_')) && (ptr > base))
	    ptr--;
    if (*ptr == '(') ptr++; /* workaround for man pages with leading '(' see svgalib man pages */
	  strcpy (buf, ptr);
	  tmppos = strlen (buf);
	  if (tmppos > 1);
	  buf[tmppos - 2] = 0;
	  strcat (buf, manuallinks[which].name);
	  xfree (base);
	}
      else
	/* url reference */
	{
	  char *base = xmalloc (1024);
	  char *ptr, *eptr;
	  int tmppos;
	  int namelen = strlen (manuallinks[which].name);
	  strcpy (base, manual[manuallinks[which].line + 1]);
	  strip_manual (base);
	  ptr = base;
	  while (isspace (*ptr))
	    ptr++;		/* skip whitespace */
	  eptr = findurlend (ptr);
	  *eptr = 0;
	  strcpy (buf, manuallinks[which].name);
	  buf[namelen - 1] = 0;	/* cut the hyphen */
	  strcat (buf, ptr);
	  xfree (base);
	}
    }
}

int
handlemanual (char *name)	/*
				 * this is something like main() function
				 * for the manual viewer code.
				 */
{
  int return_value = 0;
  struct stat statbuf;
  FILE *id, *source;

  char **ignored_entries;
  char manualname[256];
  char cmd[256];
  char location[256];
  char line[1025];
  char *raw_tempfilename = 0;
  char *apropos_tempfilename = 0;
  char *end, *prev;
  size_t macroline_size;
  int ignored_items = 0, i = 0;
  char zipped = 0;

  if (tmpfilename1)
    {
      unlink (tmpfilename1);
      xfree (tmpfilename1);
    }
  tmpfilename1 = tempnam ("/tmp", NULL);

#ifdef getmaxyx
  init_curses ();
  getmaxyx (stdscr, maxy, maxx);	/* if ncurses, get maxx and maxy */
  myendwin ();
  if ((!getenv ("MANWIDTH")) || (manwidthChanged))
    {
      static char tmp[24];	/* set MANWIDTH environment variable */
      snprintf (tmp, 24, "MANWIDTH=%d", maxx);
      putenv (tmp);
      manwidthChanged = 1;
    }
#else
  maxx = 80;
  maxy = 25;			/* otherwise hardcode 80x25... */
#endif /* getmaxyx */
#ifdef NIETS
/****************************************************************************
 *                    Ignore macros part: BEGIN                             *
 * PS: Siewca: I still expect that you'll isolate it to a single procedure  *
 * Description (by PB): This code opens a manpage file, and filters it from *
 * dangerous macros. The output is put into a temporary file, which is then *
 * used as the `name' filename argument of this (handlemanual) procedure.   *
 * There is a stored variable raw_tempfilename to allow unlinking this temp *
 * file after usage							    *
 ****************************************************************************/
  if (ignoredmacros)		/* if the pointer is non-null */
    if (*ignoredmacros && strlen (ignoredmacros))	/* if there are some macros */
      {				/* that should be ignored   */
	*location = '\0';
	snprintf (cmd, 255, "man -W %s %s",	/* we need to know the path */
		  ManOptions,
		  name);
	id = popen (cmd, "r");
	if (!id)
	  {
	    printf (_ ("Error: Cannot call man command.\n"));
	    return 1;
	  }
	fflush (id);
	fgets (location, 255, id);
	pclose (id);

	if (*location == '\0')
	  {
	    printf (_ ("Error: No manual page found either.\n"));
	    if (use_apropos)
	      {
		printf (_ ("Appropriate pages:\n"));
		snprintf (cmd, 255, "apropos %s|cat %s", name, StderrRedirection);
		system (cmd);
	      }
	    return 1;
	  }


	ignored_items++;
	prev = ignoredmacros;
	while ((end = strchr (prev, ':')))	/* counting items */
	  {
	    ignored_items++;
	    prev = end + 1;
	  }

	ignored_entries = (char **) xmalloc (ignored_items * sizeof (char **));
	ignored_entries[0] = ignoredmacros;
	prev = ignoredmacros;
	i = 0;
	while ((end = strchr (prev, ':')))	/* creating pointers */
	  {
	    *end = '\0';
	    prev = end + 1;
	    i++;
	    ignored_entries[i] = prev;
	  }

	if ((prev = rindex (location, '\n')))
	  *prev = '\0';		/* removing newline */

	prev = index (location, '\0');	/* checking if it's comressed */
	if ((strlen (location)) > 3
	    && ((*(prev - 1) == 'Z' && *(prev - 2) == '.')
	 || (*(prev - 1) == 'z' && *(prev - 2) == 'g' && *(prev - 3) == '.')
	    )
	  )
	  {
	    if (verbose)
	      printf ("%s %s\n", _ ("Calling gunzip for"), location);
	    snprintf (cmd, 255, "gunzip -c %s", location);
	    source = popen (cmd, "r");
	    zipped = 1;
	    if (!source)
	      {
		printf (_ ("Couldn't call gunzip.\n"));
		return 1;
	      }
	  }
	else
	  source = fopen (location, "r");	/* from cmd output  */
	name = tempnam ("/tmp", NULL);
	raw_tempfilename = name;
	id = fopen (name, "w");

	while (!feof (source))	/* we read until eof */
	  {
	    if (fgets (line, 1024, source) == NULL)
	      line[0] = '\0';

	    if (line[0] != '.' || (strlen (line)) < (size_t) 2)		/* macro starts */
	      {			/* with da DOT  */
		fprintf (id, "%s", line);
		continue;
	      }
	    else
	      while (i >= 0)
		{
		  macroline_size = strlen (ignored_entries[i]);
		  if (strlen (line + 1) < macroline_size)
		    macroline_size = strlen (line + 1);
		  if ((strncmp (ignored_entries[i], line + 1, macroline_size)) == 0
		      && (*(line + 1 + (int) macroline_size) == ' '
			  || *(line + 1 + (int) macroline_size) == '\n'
			  || *(line + 1 + (int) macroline_size) == '\t'))
		    {
		      if (quote_ignored)
			{
			  if ((prev = rindex (line, '\n')))
			    *prev = '\0';
			  sprintf (cmd, "\n.br\n.nf\n[ [pinfo] - %s: %.42s", _ ("IGNORING"), line);
			  if ((strlen (line)) > (size_t) 42)
			    strcat (cmd, " (...)]\n.fi\n");
			  else
			    strcat (cmd, " ]\n.fi\n");
			}
		      else
			{
			  sprintf (cmd, ".\\\" removed macro: %.42s", line);
			  if ((strlen (line)) > (size_t) 42)
			    strcat (cmd, " (...)");
			}
		      strcpy (line, cmd);
		      break;
		    }
		  i--;
		}

	    fprintf (id, "%s", line);
	    i = ignored_items - 1;
	  }			/* while (!feof (source)) */
	if (zipped)
	  pclose (source);
	else
	  fclose (source);
	fclose (id);
	free (ignored_entries);
      }				/* if (ignored_macros... */
/****************************************************************************
 *                    Ignore macros part: END                               *
 ****************************************************************************/
#endif
  if (!plain_apropos)
    snprintf (cmd, 255, "man %s %s %s > %s",
	      ManOptions,
	      name,
	      StderrRedirection,
	      tmpfilename1);
  if ((plain_apropos) || (system (cmd) != 0))
    {
      if (!plain_apropos)
	{
	  unlink (tmpfilename1);
	  printf (_ ("Error: No manual page found\n"));
	}
      plain_apropos = 0;
      if (use_apropos)
	{
	  printf (_ ("Calling apropos \n"));
	  apropos_tempfilename = tempnam ("/tmp", NULL);
	  snprintf (cmd, 255, "apropos %s > %s", name, apropos_tempfilename);
	  if (system (cmd) != 0)
	    {
	      printf (_ ("Nothing apropiate\n"));
	      unlink (apropos_tempfilename);
	      return 1;
	    }
	  id = fopen (apropos_tempfilename, "r");
	}
      else
	return 1;
    }
  else
    id = fopen (tmpfilename1, "r");
  init_curses ();


  set_initial_history (name);
  loadmanual (id);		/* load manual to memory */
  fclose (id);
  do
    {
      return_value = manualwork ();	/* manualwork handles all
					   actions when viewing man
					   page */
#ifdef getmaxyx
      getmaxyx (stdscr, maxy, maxx);	/* if ncurses, get maxx and maxy */
      if ((!getenv ("MANWIDTH")) || (manwidthChanged))
	{
	  static char tmp[24];	/* set MANWIDTH environment variable */
	  snprintf (tmp, 24, "MANWIDTH=%d", maxx);
	  putenv (tmp);
	  manwidthChanged = 1;
	}
#endif
      manual_aftersearch = 0;
      if (return_value != -1)	/* -1 is quit key */
	{
	  if (tmpfilename2)
	    {
	      unlink (tmpfilename2);
	      xfree (tmpfilename2);
	    }
	  tmpfilename2 = tempnam ("/tmp", NULL);
	  if (return_value != -2)	/* key_back is not pressed;
					   and return_value is an
					   offset to manuallinks */
	    {
	      construct_manualname (manualname, return_value);
	      snprintf (cmd, 255, "man %s %s %s %s > %s",
			ManOptions,
			manuallinks[return_value].section,
			manualname,
			StderrRedirection,
			tmpfilename2);
	    }
	  else
	    /* key_back was pressed */
	    {
	      manualhistorylength--;
	      if (manualhistorylength == 0 && apropos_tempfilename)
		{
		  id = fopen (apropos_tempfilename, "r");
		  loadmanual (id);
		  fclose (id);
		  continue;
		}
	      if (manualhistory[manualhistorylength].sect[0] == 0)
		snprintf (cmd, 255, "man %s %s %s > %s",
			  ManOptions,
			  manualhistory[manualhistorylength].name,
			  StderrRedirection,
			  tmpfilename2);
	      else
		snprintf (cmd, 255, "man %s %s %s %s > %s",
			  ManOptions,
			  manualhistory[manualhistorylength].sect,
			  manualhistory[manualhistorylength].name,
			  StderrRedirection,
			  tmpfilename2);
	      historical = 1;	/* flag to make sure, that
				   manualwork will refresh
				   the variables manualpos
				   and selected when going
				   back to this page */
	    }
	  system (cmd);
	  stat (tmpfilename2, &statbuf);
	  if (statbuf.st_size > 0)
	    {
	      snprintf (cmd, 255, "mv %s %s",
			tmpfilename2,
			tmpfilename1);
	      system (cmd);	/* create tmp file 
				   containing man page */
	      id = fopen (tmpfilename1, "r");	/* open man page */
	      if (id != NULL)
		{
		  if (!historical)	/* now we create history entry for new page */
		    {
		      manualhistorylength++;
		      manualhistory = xrealloc (manualhistory, (manualhistorylength + 2) * sizeof (manhistory));
		      strcpy (manualhistory[manualhistorylength].name, manualname);	/* we can write so since this code applies only when it's not a history call */
		      strcpy (manualhistory[manualhistorylength].sect, manuallinks[return_value].section);
		    }
		  loadmanual (id);	/* loading manual page and its defaults... */
		  fclose (id);
		  if (!historical)	/* continuing with creation of history */
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
    unlink (apropos_tempfilename);
  if (raw_tempfilename)
    unlink (raw_tempfilename);	/* we were using temporary */
  return 0;			/* raw-manpage for scaning */
}

void
loadmanual (FILE * id)		/* loads manual from given filedescriptor */
{
  char prevlinechar = 0;
  int cutheader = 0;		/* tmp variable, set after reading first
				   nonempty line of input */
  int carryflag = 0;
  manualpos = 0;
  manual_free_buffers ();
  manual = xmalloc (sizeof (char *));
  manuallinks = xmalloc (sizeof (manuallinks));
  manual[ManualLines] = xmalloc (1024);

  while (!feof (id))		/* we read until eof */
    {
      char *tmp;
      if (fgets (manual[ManualLines], 1024, id) == NULL)	/* it happens 
								   sometimes, 
								   that the last 
								   line is 
								   weird */
	manual[ManualLines][0] = 0;	/* and causes 
					   sigsegvs by 
					   not entering 
					   anything to 
					   buffer, what 
					   confuses 
					   strlen */


      if (cutheader)
	{
	  if (strcmp (manual[cutheader], manual[ManualLines]) == 0)
	    {
	      manual[ManualLines][0] = '\n';
	      manual[ManualLines][1] = 0;
	    }
	}
      if (FilterB7)
	{
	  char *filter_pos = index (manual[ManualLines], 0xb7);
	  if (filter_pos)
	    *filter_pos = 'o';
	}
      if (CutManHeaders)
	if (!cutheader)
	  {
	    if (strlen (manual[ManualLines]) > 1)
	      {
		cutheader = ManualLines;
	      }
	  }
      if ((CutEmptyManLines) && ((manual[ManualLines][0]) == '\n') &&
	  (prevlinechar == '\n'))
	{
	  ;			/* do nothing :)) */
	}
      else
	{
	  int manlinelen = strlen (manual[ManualLines]);
	  manual[ManualLines] = xrealloc (manual[ManualLines],
					  manlinelen + 10);

	  tmp = xmalloc (manlinelen + 10);	/* temporary 
						   variable for 
						   determining 
						   hypertextuality
						   of fields */

	  strcpy (tmp, manual[ManualLines]);

	  strip_manual (tmp);	/* remove formatting chars */
	  man_initializelinks (tmp, carryflag);
	  carryflag = 0;
	  if (manlinelen > 1)
	    if (ishyphen (manual[ManualLines][manlinelen - 2]))
	      carryflag = 1;
	  xfree (tmp);		/* free temporary buffer */
	  prevlinechar = manual[ManualLines][0];
	  ManualLines++;	/* increase the number of man lines */
	  manual = xrealloc (manual, (ManualLines + 5) * sizeof (char *));
	  /* and realloc manual to add an empty
	     space for next entry of manual line */
	  manual[ManualLines] = xmalloc (1024);
	}
    }

}

int
compare_manuallink (const void *a, const void *b)
{
  return ((manuallink *) a)->col - ((manuallink *) b)->col;
}

void
sort_manuallinks_from_current_line (long startlink, long endlink)
{
  qsort (manuallinks + startlink, endlink - startlink, sizeof (manuallink), compare_manuallink);
}


void
man_initializelinks (char *tmp, int carry)	/* initializes hyperlinks in manual */
{
  int tmpcnt = strlen (tmp) + 1;	/* set tmpcnt to the trailing zero of tmp */
  char *link = tmp;
  char *urlstart, *urlend;
  long initialManualLinks = ManualLinks;
  int i, b;
/******************************************************************************
 * handle url refrences                                                       *
 *****************************************************************************/
  urlend = tmp;
  while ((urlstart = strstr (urlend, "http://")) != NULL)
    {
      urlend = findurlend (urlstart);	/* always successfull */
      manuallinks = xrealloc (manuallinks, sizeof (manuallink) * (ManualLinks + 3));
      manuallinks[ManualLinks].line = ManualLines;
      manuallinks[ManualLinks].col = urlstart - tmp;
      strcpy (manuallinks[ManualLinks].section, "HTTPSECTION");
      manuallinks[ManualLinks].section_mark = HTTPSECTION;
      manuallinks[ManualLinks].name = xmalloc (urlend - urlstart + 10);
      strncpy (manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
      manuallinks[ManualLinks].name[urlend - urlstart] = 0;
      if (ishyphen (manuallinks[ManualLinks].name[urlend - urlstart - 1]))
	manuallinks[ManualLinks].carry = 1;
      else
	manuallinks[ManualLinks].carry = 0;
      ManualLinks++;
    }
  urlend = tmp;
  while ((urlstart = strstr (urlend, "ftp://")) != NULL)
    {
      urlend = findurlend (urlstart);	/* always successfull */
      manuallinks = xrealloc (manuallinks, sizeof (manuallink) * (ManualLinks + 3));
      manuallinks[ManualLinks].line = ManualLines;
      manuallinks[ManualLinks].col = urlstart - tmp;
      strcpy (manuallinks[ManualLinks].section, "FTPSECTION");
      manuallinks[ManualLinks].section_mark = FTPSECTION;
      manuallinks[ManualLinks].name = xmalloc (urlend - urlstart + 10);
      strncpy (manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
      manuallinks[ManualLinks].name[urlend - urlstart] = 0;
      if (ishyphen (manuallinks[ManualLinks].name[urlend - urlstart - 1]))
	manuallinks[ManualLinks].carry = 1;
      else
	manuallinks[ManualLinks].carry = 0;
      ManualLinks++;
    }
  urlend = tmp;
  while ((urlstart = findemailstart (urlend)) != NULL)
    {
      urlend = findurlend (urlstart);	/* always successfull */
      manuallinks = xrealloc (manuallinks, sizeof (manuallink) * (ManualLinks + 3));
      manuallinks[ManualLinks].line = ManualLines;
      manuallinks[ManualLinks].col = urlstart - tmp;
      strcpy (manuallinks[ManualLinks].section, "MAILSECTION");
      manuallinks[ManualLinks].section_mark = MAILSECTION;
      manuallinks[ManualLinks].name = xmalloc (urlend - urlstart + 10);
      strncpy (manuallinks[ManualLinks].name, urlstart, urlend - urlstart);
      manuallinks[ManualLinks].name[urlend - urlstart] = 0;
      if (ishyphen (manuallinks[ManualLinks].name[urlend - urlstart - 1]))
	manuallinks[ManualLinks].carry = 1;
      else
	manuallinks[ManualLinks].carry = 0;
      if (strchr (manuallinks[ManualLinks].name, '.') != NULL)	/* there should be a 
								   dot in e-mail domain */
	ManualLinks++;
    }
/******************************************************************************
* handle normal manual refrences -- reference (section)                       *
******************************************************************************/
  do
    {
      link = strchr (link, '(');	/* we look for '(', since manual link */
      if (link != NULL)		/* has form of  'blah (x)' */
	{
	  char *temp;
	  if ((temp = strchr (link, ')')))	/* look for the closing bracket */
	    {
	      char *p_t1, *p_t;
	      p_t = p_t1 = xmalloc ((strlen (link) + 10) * sizeof (char));
	      for (++link; link != temp; *p_t++ = *link++);
	      *p_t = '\0';
	      link -= (strlen (p_t1) + sizeof (char));

	      if ((!strchr (p_t1, '(')) && (!is_in_manlinks (manlinks, p_t1)))
		{
		  char tempchar;
		  int breakpos;
		  i = link - tmp - 1;
		  if (i < 0)
		    i++;
		  for (; i > 0; --i)
		    {
		      if (!isspace (tmp[i]))
			/* ignore spaces between linkname and '(x)' */
			break;
		    }
		  breakpos = i + 1;	/* we'll put zero on the last non-textual character of link */
		  tempchar = tmp[breakpos];	/* but remember the cleared char for the future */
		  tmp[breakpos] = 0;
		  for (i = breakpos; i > 0; --i)	/* scan to the first space sign or to 0 -- that means go to the beginning of the scanned token */
		    {
		      if (isspace (tmp[i]))
			{
			  i++;
			  break;
			}
		    }
		  /* now we have needed string in i..breakpos. We need now to realloc the
		     manuallinks table to make free space for new entry */

		  if (!((use_apropos) && (manualhistorylength == 0)))	/* a small check */
		    {
		      if ((!strcasecmp (&tmp[i], manualhistory[manualhistorylength].name))
			  && ((!strcasecmp (p_t1, manualhistory[manualhistorylength].sect))
			|| (manualhistory[manualhistorylength].sect[0] == 0)
			      || (!strcmp (manualhistory[manualhistorylength].sect, " "))))

			/* In English: if the name of the link is the name of the current page
			   and the section of the link is the current section or if we don't
			   know the current section, then... */
			break;
		    }
		  manuallinks = xrealloc (manuallinks, sizeof (manuallink) * (ManualLinks + 3));
		  manuallinks[ManualLinks].line = ManualLines;
		  manuallinks[ManualLinks].col = i;
		  if (LongManualLinks)
		    {
		      for (b = 1; link[b] != ')'; b++)
			manuallinks[ManualLinks].section[b - 1] = tolower (link[b]);
		      manuallinks[ManualLinks].section[b - 1] = 0;
		    }
		  else
		    {
		      manuallinks[ManualLinks].section[0] = link[1];
		      manuallinks[ManualLinks].section[1] = 0;
		    }
		  manuallinks[ManualLinks].section_mark = 0;
		  manuallinks[ManualLinks].name = xmalloc ((breakpos - i) + 10);
		  strcpy (manuallinks[ManualLinks].name, tmp + i);
		  tmp[breakpos] = tempchar;
		  for (b = i - 1; b >= 0; b--)	/* check whether this is a carry'ed
						 * entry (i.e. in the previous line
						 * there was `-' at end, and this is
						 * the first word of this line */
		    {
		      if (b > 0)
			if (!isspace (tmp[b]))
			  break;
		    }
		  if (b >= 0)
		    manuallinks[ManualLinks].carry = 0;
		  else
		    manuallinks[ManualLinks].carry = carry;
		  ManualLinks++;	/* increase the number of entries */
		}		/*... if(in man links) */
	      xfree ((void *) p_t1);
	    }
	}
      if (link)
	link++;
      if (link > (tmp + tmpcnt))
	{
	  break;
	}
    }
  while (link != NULL);		/* do this line until strchr() won't
				   find a '(' in string */
  if (initialManualLinks != ManualLinks)
    sort_manuallinks_from_current_line (initialManualLinks, ManualLinks);
}

int
manualwork ()			/* viewer function. Handles keyboard actions--main event loop */
{
  FILE *pipe;			/* for user's shell commands */
  char *token;			/* a temporary buffer */
  char *tmp;			/* again the same */
  int key = 0;			/* key, which contains the value entered by user */
  int i, selectedchanged;	/* tmp values */
  int statusline = FREE;
#ifdef getmaxyx
  getmaxyx (stdscr, maxy, maxx);	/* if ncurses, get maxx and maxy */
  if ((!getenv ("MANWIDTH")) || (manwidthChanged))
    {
      static char tmp[24];	/* set MANWIDTH environment variable */
      snprintf (tmp, 24, "MANWIDTH=%d", maxx);
      putenv (tmp);
      manwidthChanged = 1;
    }
#else
  maxx = 80;
  maxy = 25;			/* otherwise hardcode 80x25... */
#endif /* getmaxyx */
  manualpos = manualhistory[manualhistorylength].pos;	/* get manualpos
							   from history.
							   it is set in
							   handlemanual() */
  if (manualhistory[manualhistorylength].selected != -1)	/* if there
								   was a valid
								   selected entry, 
								   apply it */
    selected = manualhistory[manualhistorylength].selected;
  else
    rescan_selected ();		/* otherwise scan for
				   selected on currently
				   viewed page */
  erase ();			/* clean screen */
  while (1)			/* user events loop. finish when
				   key_quit */
    {
      nodelay (stdscr, TRUE);	/* make getch not wait for user */
      key = pinfo_getch ();	/* action -- return ERR */
      if (key == ERR)		/* if there was nothing in buffer */
	{
	  if (statusline == FREE)
	    showmanualscreen ();	/* then show screen */
	  wrefresh (stdscr);
	  waitforgetch ();
	  key = pinfo_getch ();
	}
      nodelay (stdscr, FALSE);
      statusline = FREE;
      if (winchanged)
	{
	  handlewinch ();
	  winchanged = 0;
	  key = pinfo_getch ();
	}
/************************ keyboard handling **********************************/
      if (key != 0)
	{
	  if ((key == keys.print_1) ||
	      (key == keys.print_2))
	    {
	      if (yesno (_ ("Are you sure to print?"), 0))
		printmanual (manual, ManualLines);
	    }
/*==========================================================================*/
	  if ((key == keys.goto_1) ||
	      (key == keys.goto_2))
	    {
	      manuallinks = xrealloc (manuallinks, (ManualLinks + 1) * (sizeof (manuallink) + 3));

	      attrset (bottomline);	/* get user's value */
	      move (maxy - 1, 0);
	      echo ();
	      curs_set (1);
	      manuallinks[ManualLinks].name = getstring (_ ("Enter manual name: "));
	      curs_set (0);
	      noecho ();
	      move (maxy - 1, 0);
#ifdef HAVE_BKGDSET
	      bkgdset (' ' | bottomline);
	      clrtoeol ();
	      bkgdset (0);
#else
	      myclrtoeol ();
#endif
	      attrset (normal);

	      manuallinks[ManualLinks].carry = 0;
	      manuallinks[ManualLinks].section_mark = 0;
	      strcpy (manuallinks[ManualLinks].section, " ");
	      manuallinks[ManualLinks].line = -1;
	      manuallinks[ManualLinks].col = -1;
	      ManualLinks++;
	      return ManualLinks - 1;
	    }
/*==========================================================================*/
	  if ((key == keys.goline_1) ||
	      (key == keys.goline_2))
	    {
	      long newpos;
	      attrset (bottomline);	/* get user's value */
	      move (maxy - 1, 0);
	      echo ();
	      curs_set (1);
	      token = getstring (_ ("Enter line: "));
	      curs_set (0);
	      noecho ();
	      move (maxy - 1, 0);
#ifdef HAVE_BKGDSET
	      bkgdset (' ' | bottomline);
	      clrtoeol ();
	      bkgdset (0);
#else
	      myclrtoeol ();
#endif
	      attrset (normal);
	      if (token)	/*
				 * convert string to long.
				 * careful with nondigit strings.
				 */
		{
		  int digit_val = 1;
		  for (i = 0; token[i] != 0; i++)
		    {
		      if (!isdigit (token[i]))
			digit_val = 0;
		    }
		  if (digit_val)	/* move cursor position */
		    {
		      newpos = atol (token);
		      newpos -= (maxy - 1);
		      if ((newpos >= 0) && (newpos < ManualLines - (maxy - 2)))
			manualpos = newpos;
		      else if (newpos > 0)
			manualpos = ManualLines - (maxy - 2);
		      else
			manualpos = 0;
		    }
		  xfree (token);
		  token = 0;
		}
	    }
/*===========================================================================*/
	  if ((key == keys.shellfeed_1) ||
	      (key == keys.shellfeed_2))
	    {
	      /* get command name */
	      curs_set (1);
	      attrset (bottomline);
	      move (maxy - 1, 0);
	      echo ();
	      token = getstring (_ ("Enter command: "));	/* get users cmd */
	      noecho ();
	      move (maxy - 1, 0);
#ifdef HAVE_BKGDSET
	      bkgdset (' ' | bottomline);
	      clrtoeol ();
	      bkgdset (0);
#else
	      myclrtoeol ();
#endif
	      attrset (normal);

	      myendwin ();
	      system ("clear");
	      pipe = popen (token, "w");	/* open pipe */
	      if (pipe != NULL)
		{
		  for (i = 0; i < ManualLines; i++)	/* and flush the msg to stdin */
		    fprintf (pipe, "%s", manual[i]);
		  pclose (pipe);
		}
	      getchar ();
	      doupdate ();
	      curs_set (0);
	    }
/*===========================================================================*/
	  if ((key == keys.refresh_1) ||
	      (key == keys.refresh_2))
	    {
	      myendwin ();
	      doupdate ();
	      refresh ();
	      curs_set (0);
	    }
/*===========================================================================*/
	  if ((key == keys.search_1) ||		/* search in current node */
	      (key == keys.search_2))
	    {
	      int success = 0;
	      move (maxy - 1, 0);	/* procedure of getting regexp string */
	      attrset (bottomline);
	      echo ();
	      curs_set (1);
	      if (!searchagain.search)	/* 
					 * searchagain handler. see
					 * keys.totalsearch at mainfunction.c
					 * for comments
					 */
		{
		  token = getstring (_ ("Enter regexp: "));
		  strcpy (searchagain.lastsearch, token);
		  searchagain.type = key;
		}
	      else
		{
		  token = xmalloc (strlen (searchagain.lastsearch) + 1);
		  strcpy (token, searchagain.lastsearch);
		  searchagain.search = 0;
		}		/* end of searchagain handler */
	      if (strlen (token) == 0)
		{
		  xfree (token);
		  goto skip_search;
		}
	      curs_set (0);
	      noecho ();
	      move (maxy - 1, 0);
#ifdef HAVE_BKGDSET
	      bkgdset (' ' | bottomline);
	      clrtoeol ();
	      bkgdset (0);
#else
	      myclrtoeol ();
#endif
	      attrset (normal);
	      pinfo_re_comp (token);	/* compile regexp expression */
	      for (i = manualpos + 1; i < ManualLines - 1; i++)
		/* and search for it in all subsequential lines */
		{
		  tmp = xmalloc (strlen (manual[i]) + strlen (manual[i + 1]) + 10);
		  strcpy (tmp, manual[i]);	/*
						 * glue two following lines
						 * together, to find expres-
						 * sions split up into two 
						 * lines
						 */
		  strcat (tmp, manual[i + 1]);
		  strip_manual (tmp);

		  if (pinfo_re_exec (tmp))	/* execute search */
		    {		/* if found, enter here... */
		      success = 1;
		      strcpy (tmp, manual[i + 1]);
		      strip_manual (tmp);
		      if (pinfo_re_exec (tmp))	/* 
						 * if it was found in the second line 
						 * of the glued expression.
						 */
			manualpos = i + 1;
		      else
			manualpos = i;
		      xfree (tmp);
		      break;
		    }
		  xfree (tmp);
		}
	      xfree (token);
	      rescan_selected ();
	      if (!success)
		{
		  attrset (bottomline);
		  mvaddstr (maxy - 1, 0, _ ("Search string not found..."));
		  statusline = LOCKED;
		}

	      manual_aftersearch = 1;
	    }
/*===========================================================================*/
	  if ((key == keys.search_again_1) ||	/* search again */
	      (key == keys.search_again_2))	/* see mainfunction.c for comments */
	    {
	      if (searchagain.type != 0)
		{
		  searchagain.search = 1;
		  ungetch (searchagain.type);
		}
	    }
	skip_search:
/*===========================================================================*/
	  if ((key == keys.twoup_1) ||
	      (key == keys.twoup_2))
	    {
	      ungetch (keys.up_1);
	      ungetch (keys.up_1);
	    }
/*===========================================================================*/
	  if ((key == keys.up_1) ||
	      (key == keys.up_2))
	    {
	      selectedchanged = 0;
	      if (selected != -1)	/* if there are links at all */
		{
		  if (selected > 0)	/* if one is selected */
		    for (i = selected - 1; i >= 0; i--)		/* 
								 * scan for a next
								 * visible one, which
								 * is above the current.
								 */
		      {
			if ((manuallinks[i].line >= manualpos) &&
			    (manuallinks[i].line < manualpos + (maxy - 1)))
			  {
			    selected = i;
			    selectedchanged = 1;
			    break;
			  }
		      }
		}
	      if (!selectedchanged)	/* if new link not found */
		{
		  if (manualpos >= 1)	/* move one position up */
		    manualpos--;
		  for (i = 0; i < ManualLinks; i++)	/* 
							 * and scan for selected
							 * again :)
							 */
		    {
		      if (manuallinks[i].line == manualpos)
			{
			  selected = i;
			  break;
			}
		    }
		}
	    }
/*===========================================================================*/
	  if ((key == keys.end_1) ||
	      (key == keys.end_2))
	    {
	      manualpos = ManualLines - (maxy - 1);
	      if (manualpos < 0)
		manualpos = 0;
	      selected = ManualLinks - 1;
	    }
/*===========================================================================*/
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
/*===========================================================================*/
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
/*===========================================================================*/
	  if ((key == keys.pgdn_1) ||
	      (key == keys.pgdn_2))
	    {
	      if (manualpos + (maxy - 2) < ManualLines - (maxy - 1))
		{
		  manualpos += (maxy - 2);
		  rescan_selected ();
		}
	      else if (ManualLines - (maxy - 1) >= 1)
		{
		  manualpos = ManualLines - (maxy - 1);
		  selected = ManualLinks - 1;
		}
	      else
		{
		  manualpos = 0;
		  selected = ManualLinks - 1;
		}
	    }
/*===========================================================================*/
	  if ((key == keys.home_1) ||
	      (key == keys.home_2))
	    {
	      manualpos = 0;
	      rescan_selected ();
	    }
/*===========================================================================*/
	  if ((key == keys.pgup_1) |
	      (key == keys.pgup_2))
	    {
	      if (manualpos > (maxy - 1))
		manualpos -= (maxy - 1);
	      else
		manualpos = 0;
	      rescan_selected ();
	    }
/*===========================================================================*/
	  if ((key == keys.twodown_1) ||
	      (key == keys.twodown_2))	/* top+bottom line \|/ */
	    {			/* see keys.up for comments */
	      ungetch (keys.down_1);
	      ungetch (keys.down_1);
	    }
/*===========================================================================*/
	  if ((key == keys.down_1) ||
	      (key == keys.down_2))	/* top+bottom line \|/ */
	    {			/* see keys.up for comments */
	      selectedchanged = 0;
	      if (selected < ManualLinks)
		for (i = selected + 1; i < ManualLinks; i++)
		  {
		    if ((manuallinks[i].line >= manualpos) &&
			(manuallinks[i].line < manualpos + (maxy - 2)))
		      {
			selected = i;
			selectedchanged = 1;
			break;
		      }
		  }
	      if (!selectedchanged)
		{
		  if (manualpos < ManualLines - (maxy - 1))
		    manualpos++;
		  if (selected < ManualLinks)
		    for (i = selected + 1; i < ManualLinks; i++)
		      {
			if ((manuallinks[i].line >= manualpos) &&
			    (manuallinks[i].line < manualpos + (maxy - 2)))
			  {
			    selected = i;
			    selectedchanged = 1;
			    break;
			  }
		      }
		}
	    }
/*===========================================================================*/
	  if ((key == keys.back_1) ||
	      (key == keys.back_2))
	    {
	      if (manualhistorylength)
		return -2;
	    }
/*===========================================================================*/
	  if ((key == keys.followlink_1) ||
	      (key == keys.followlink_2))
	    {
	      manualhistory[manualhistorylength].pos = manualpos;
	      manualhistory[manualhistorylength].selected = selected;
	      if (selected >= 0)
		if ((manuallinks[selected].line >= manualpos) &&
		    (manuallinks[selected].line < manualpos + (maxy - 1)))
		  {
		    if (!strncmp (manuallinks[selected].section, "HTTPSECTION", 11))
		      {
			int buflen;
			char *tempbuf = xmalloc (1024);
			strcpy (tempbuf, httpviewer);
			strcat (tempbuf, " ");
			buflen = strlen (tempbuf);
			construct_manualname (tempbuf + buflen, selected);
			myendwin ();
			system (tempbuf);
			doupdate ();
			xfree (tempbuf);
		      }
		    else if (!strncmp (manuallinks[selected].section, "FTPSECTION", 10))
		      {
			int buflen;
			char *tempbuf = xmalloc (1024);
			strcpy (tempbuf, ftpviewer);
			strcat (tempbuf, " ");
			buflen = strlen (tempbuf);
			construct_manualname (tempbuf + buflen, selected);
			myendwin ();
			system (tempbuf);
			doupdate ();
			xfree (tempbuf);
		      }
		    else if (!strncmp (manuallinks[selected].section, "MAILSECTION", 11))
		      {
			int buflen;
			char *tempbuf = xmalloc (1024);
			strcpy (tempbuf, maileditor);
			strcat (tempbuf, " ");
			buflen = strlen (tempbuf);
			construct_manualname (tempbuf + buflen, selected);
			myendwin ();
			system (tempbuf);
			doupdate ();
			xfree (tempbuf);
		      }
		    else
		      {
			return selected;
		      }
		  }
	    }
/*===========================================================================*/
	  if((key==keys.left_1)||(key==keys.left_2))
	    if(manualcol>0) manualcol--;
	  if((key==keys.right_1)||(key==keys.right_2))
	    manualcol++;
/*===========================================================================*/
/*********************** end of keyboard handling ***************************/
/********************************* mouse handler *****************************/
#ifdef NCURSES_MOUSE_VERSION
	  if (key == KEY_MOUSE)
	    {
	      MEVENT mouse;
	      int done = 0;
	      getmouse (&mouse);
	      if (mouse.bstate == BUTTON1_CLICKED)
		{
		  if ((mouse.y > 0) && (mouse.y < maxy - 1))
		    {
		      for (i = selected; i > 0; i--)
			{
			  if (manuallinks[i].line == mouse.y + manualpos - 1)
			    {
			      if (manuallinks[i].col <= mouse.x - 1)
				{
				  if (manuallinks[i].col + strlen (manuallinks[i].name) >= mouse.x - 1)
				    {
				      selected = i;
				      done = 1;
				      break;
				    }
				}
			    }
			}
		      if (!done)
			for (i = selected; i < ManualLinks; i++)
			  {
			    if (manuallinks[i].line == mouse.y + manualpos - 1)
			      {
				if (manuallinks[i].col <= mouse.x - 1)
				  {
				    if (manuallinks[i].col + strlen (manuallinks[i].name) >= mouse.x - 1)
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
		    ungetch (keys.up_1);
		  if (mouse.y == maxy - 1)
		    ungetch (keys.down_1);
		}		/* end: button_clicked */
	      if (mouse.bstate == BUTTON1_DOUBLE_CLICKED)
		{
		  if ((mouse.y > 0) && (mouse.y < maxy - 1))
		    {
		      for (i = selected; i > 0; i--)
			{
			  if (manuallinks[i].line == mouse.y + manualpos - 1)
			    {
			      if (manuallinks[i].col <= mouse.x - 1)
				{
				  if (manuallinks[i].col + strlen (manuallinks[i].name) >= mouse.x - 1)
				    {
				      selected = i;
				      done = 1;
				      break;
				    }
				}
			    }
			}
		      if (!done)
			for (i = selected; i < ManualLinks; i++)
			  {
			    if (manuallinks[i].line == mouse.y + manualpos - 1)
			      {
				if (manuallinks[i].col <= mouse.x - 1)
				  {
				    if (manuallinks[i].col + strlen (manuallinks[i].name) >= mouse.x - 1)
				      {
					selected = i;
					done = 1;
					break;
				      }
				  }
			      }
			  }
		      if (done)
			ungetch (keys.followlink_1);
		    }		/* end: mouse not at top/bottom line */
		  if (mouse.y == 0)
		    ungetch (keys.pgup_1);
		  if (mouse.y == maxy - 1)
		    ungetch (keys.pgdn_1);
		}		/* end: button doubleclicked */
	    }
#endif
/*****************************************************************************/
	}
      if ((key == keys.quit_2) || (key == keys.quit_1))
	{
	  if (!ConfirmQuit)
	    break;
	  else
	    {
	      if (yesno (_ ("Are you sure to quit?"), QuitConfirmDefault))
		break;
	    }
	}
    }
  closeprogram ();
  return -1;
}

void
rescan_selected ()		/* scan for some hyperlink, available on current screen */
{
  int i;
  for (i = 0; i < ManualLinks; i++)
    {
      if ((manuallinks[i].line >= manualpos) &&
	  (manuallinks[i].line < manualpos + (maxy - 1)))
	{
	  selected = i;
	  break;
	}
    }
}

char *getmancolumn(char *man, int mancol)	/* calculate from which to
						   start displaying of manual
						   line *man. Skip `mancol'
						   columns. But remember, that
						   *man contains also nonprinteble
						   characters for boldface etc. */
{
  if(mancol==0) return man;
  while(mancol>0)
    { if(*(man+1) == 8) man+=3; else man++; mancol--; }
  return man;
}

void
showmanualscreen ()		/* show the currently visible part of manpage */
{
  int i;
#ifdef getmaxyx
  getmaxyx (stdscr, maxy, maxx);	/* refresh maxy, maxx values */
#endif
  attrset (normal);
  for (i = manualpos; (i < manualpos + (maxy - 2)) && (i < ManualLines); i++)
    /* print all visible text lines */
    {
      int len = strlen (manual[i]);
      if (len)
	manual[i][len - 1] = ' ';
      if(len>manualcol)	/* if we have something to display */
        mvaddstr_manual ((i - manualpos) + 1, 0, getmancolumn(manual[i],manualcol));
      else		/* otherwise, just clear the line to eol */
        {
          move((i - manualpos) + 1, 0);
          bkgdset (' ' | normal);
          clrtoeol();
        }
      if (len)
	manual[i][len - 1] = '\n';
    }
#ifdef HAVE_BKGDSET
  bkgdset (' ' | normal);
#endif
  clrtobot ();			/* and clear to bottom */
#ifdef HAVE_BKGDSET
  bkgdset (0);
#endif
  attrset (normal);
  add_highlights ();		/* add highlights */
  attrset (bottomline);		/* draw bottomline with user informations */
  mymvhline (0, 0, ' ', maxx);
  mymvhline (maxy - 1, 0, ' ', maxx);
  move (maxy - 1, 0);
  if (((manualpos + maxy) < ManualLines) && (ManualLines > maxy - 2))
    printw (_ ("Viewing line %d/%d, %d%%"), (manualpos - 1 + maxy), ManualLines, ((manualpos - 1 + maxy) * 100) / ManualLines);
  else
    printw (_ ("Viewing line %d/%d, 100%%"), ManualLines, ManualLines);
  move (maxy - 1, 0);
  attrset (normal);
}
void
mvaddstr_manual (int y, int x, char *str)	/* print a manual line */
{
  int i, j, len = strlen (str);
  static char strippedline[1024];
  if ((h_regexp_num) || (manual_aftersearch))
    {
      memcpy (strippedline, str, len + 1);
      strip_manual (strippedline);
    }
  move (y, x);
  for (i = 0; i < len; i++)
    {
      if ((i > 0) && (i < len - 1))
	{
	  if ((str[i] == 8) && (str[i - 1] == '_'))	/* handle bold highlight */
	    {
	      attrset (manualbold);
	      addch (str[i] & 0xff);
	      addch (str[i + 1] & 0xff);
	      attrset (normal);
	      i++;
	      goto label_skip_other;
	    }
	  else if (i < len - 3)	/* 
				 * if it wasn't bold, check italic, before 
				 * default, unhighlighted line will be painted.
				 * We can do it only if i<len-3.
				 */
	    goto label_check_italic;
	  else
	    /* unhighlighted */
	    {
	      addch (str[i] & 0xff);
	      goto label_skip_other;
	    }
	}
      if (i < len - 3)		/* italic highlight */
	{
	label_check_italic:
	  if ((str[i + 1] == 8) && (str[i + 2] == str[i]))
	    {
	      attrset (manualitalic);
	      addch (str[i] & 0xff);
	      i += 2;
	      attrset (normal);
	    }
	  else
	    {
	      addch (str[i] & 0xff);
	    }
	}
    label_skip_other:;
    }
#ifdef HAVE_BKGDSET
  bkgdset (' ' | normal);
  clrtoeol ();
  bkgdset (0);
#else
  myclrtoeol ();
#endif
  attrset (normal);
#ifndef ___DONT_USE_REGEXP_SEARCH___
  if ((h_regexp_num) || (manual_aftersearch))
    {
      regmatch_t pmatch[1];
      int maxregexp = manual_aftersearch ? h_regexp_num + 1 : h_regexp_num;
      /* if it is after search, then we have user defined regexps+
         a searched regexp to highlight */
      for (j = 0; j < maxregexp; j++)
	{
	  char *tmpstr = strippedline;
	  while (!regexec (&h_regexp[j], tmpstr, 1, pmatch, 0))
	    {
	      int n = pmatch[0].rm_eo - pmatch[0].rm_so, k;
	      int rx = pmatch[0].rm_so + tmpstr - strippedline;
	      int curY, curX;
	      char tmpchr;
	      getyx (stdscr, curY, curX);
	      tmpchr = strippedline[rx + n];
	      strippedline[rx + n] = 0;
	      attrset (searchhighlight);
	      mvaddstr (y, rx, strippedline + rx);
	      attrset (normal);
	      strippedline[rx + n] = tmpchr;
	      tmpstr = tmpstr + pmatch[0].rm_eo;
	      move (curY, curX);
	    }
	}
    }
#endif
}
void
add_highlights ()		/* add hyperobject highlights */
{
  int i;
  for (i = 0; i < ManualLinks; i++)	/* scan through the visible objects */
    {
      if ((manuallinks[i].line >= manualpos) &&	/* if the object is on the current screen */
	  (manuallinks[i].line < manualpos + (maxy - 2)))
	{
	  if (manuallinks[i].section_mark < HTTPSECTION)	/* if it's a simple man link */
	    {
	      if (i == selected)
		attrset (noteselected);
	      else
		attrset (note);
	      if (manuallinks[i].carry == 1)	/* if it's a link split into two lines */
		{
		  int x, y, ltline = manuallinks[i].line - 1;
		  char *tmpstr = strdup (manual[ltline]);	/* find the line, where starts the split link */
		  int ltlinelen;
		  char *newlinemark;
		  strip_manual (tmpstr);	/* remove boldfaces&italics */
		  ltlinelen = strlen (tmpstr);  /* calculate the length of this line */
		  newlinemark = tmpstr + ltlinelen - 1;	/* set this var to the last character of this line (to an '\n')*/
		  getyx (stdscr, y, x);
		  if (y > 2)
		    {
#define TestCh tmpstr[ltlinelen]
		      if (ltlinelen > 2)	/* skip \n, -, and the at least one char... */
			ltlinelen -= 3;
		      while ((isalpha (TestCh)) || (TestCh == '.') || (TestCh == '_'))	/* positon ltlinelen to the beginning of the link to be highlighted */
			ltlinelen--;

		      *newlinemark = 0;
		      if(ltlinelen>manualcol)	/* OK, link horizontally fits into screen */
		        mvaddstr (manuallinks[i].line - manualpos + 1 - 1, ltlinelen-manualcol,
			  	  &tmpstr[ltlinelen]);
		      else if(ltlinelen+strlen(&tmpstr[ltlinelen])>manualcol)	/* we cut here a part of the link, and draw only what's visible on screen */
		        mvaddstr (manuallinks[i].line - manualpos + 1 - 1, ltlinelen-manualcol,
			  	  &tmpstr[manualcol]);
		      
		      *newlinemark = '\n';
#undef TestCh
		    }
		  xfree (tmpstr);
		  move (y, x);
		}
	    }
	  else
	    {
	      if (i == selected)
		attrset (urlselected);
	      else
		attrset (url);
	      if (manuallinks[i].carry == 1)
		{
		  int x, y, ltline = manuallinks[i].line + 1;
		  char *tmpstr = strdup (manual[ltline]);	/* the split part to find is lying down to the line defined in manlinks (line+1) */
		  char *wsk = tmpstr, *wskend;
		  strip_manual (tmpstr);
		  while (isspace (*wsk))
		    wsk++;	/* skip spaces */
		  wskend = findurlend (wsk);	/* find the end of url */
		  *wskend = 0;	/* add end of string, and print */
		  if(wsk-tmpstr<manualcol)
		    mvaddstr (manuallinks[i].line - manualpos + 2, wsk - tmpstr - manualcol, wsk);
		  else if(wskend-tmpstr<manualcol)
		    mvaddstr (manuallinks[i].line - manualpos + 2, 0, wsk+manualcol);
		}
	    }
	  if(manuallinks[i].col>manualcol)
	    mvaddstr (1 + manuallinks[i].line - manualpos, manuallinks[i].col - manualcol, manuallinks[i].name);
	  else if(manuallinks[i].col+strlen(manuallinks[i].name)>manualcol)
	    mvaddstr (1 + manuallinks[i].line - manualpos, 0, manuallinks[i].name+(manualcol-manuallinks[i].col));
	  attrset (normal);
	}
    }
}
void
strip_manual (char *buf)	/* all variables passed here must have, say 10 bytes of overrun buffer */
{
  int i, tmpcnt = 0;
  for (i = 0; buf[i] != 0; i++)	/* in general, tmp buffer will hold a line stripped from highlight marks */
    {
      if ((buf[i] == '_') && (buf[i + 1] == 8))		/* so we strip the line from "'_',0x8" -- bold marks */
	{
	  buf[tmpcnt++] = buf[i + 2];
	  i += 2;
	}
      else if ((buf[i + 1] == 8) && (buf[i + 2] == buf[i]))	/* and 0x8 -- italic marks */
	{
	  buf[tmpcnt++] = buf[i];
	  i += 2;
	}
      else
	buf[tmpcnt++] = buf[i];	/* else we don't do anything */
    }
  buf[tmpcnt] = 0;
}

int
is_in_manlinks (char *in, char *find)	/* checks if a construction, which
					 * looks like hyperlink, belongs
					 * to the allowed manual sections.*/
{
  char *copy, *token;
  const char delimiters[] = ":";

  copy = strdup (in);
  if ((strcmp (find, (token = strtok (copy, delimiters))) != 0))
    {
      while ((token = strtok (NULL, delimiters)))
	{
#ifdef HAVE_STRCASECMP
	  if (!strcasecmp (token, find))
#else
	  if (!strcmp (token, find))
#endif
	    {
	      xfree ((void *) copy);
	      return 0;
	    }
	}
      xfree ((void *) copy);
      return 1;
    }
  else
    {
      xfree ((void *) copy);
      return 0;
    }
}

void
printmanual (char **Message, long Lines)
{
  FILE *prnFD;			/* printer fd */
  int i;

  prnFD = popen (printutility, "w");

  for (i = 0; i < Lines; i++)	/* scan through all lines */
    {
      fprintf (prnFD, "\r%s", Message[i]);
    }
  pclose (prnFD);
}

int
ishyphen (unsigned char ch)
{
  if ((ch == '-') || (ch == SOFT_HYPHEN))
    return 1;
  return 0;
}
