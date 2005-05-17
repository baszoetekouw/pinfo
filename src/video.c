
#include "common_includes.h"

RCSID ("$Id$")

     void info_add_highlights (int pos, int cursor, long lines, int column, char **message);

     void
       substitutestr (char *src, char *dest, char *from, char *to)
/*
 * Utility for substituting strings in given string.
 * Used for internationalization of info headers.
 */
{
  char *start = strstr (src, from);
  char tmp;
  if (!start)
    strcpy (dest, src);
  else
    {
      tmp = *start;
      *start = 0;
      strcpy (dest, src);
      strcat (dest, to);
      *start = tmp;
      start += strlen (from);
      strcat (dest, start);
    }
}

void
addtopline (char *type, int column)
{
  char *buf1 = xmalloc (strlen (type) + 50);
  char *buf2 = xmalloc (strlen (type) + 50);
  int buf2len;
  strcpy (buf1, type);

  substitutestr (buf1, buf2, "File:", _ ("File:"));
  substitutestr (buf2, buf1, "Node:", _ ("Node:"));
  substitutestr (buf1, buf2, "Next:", _ ("Next:"));
  substitutestr (buf2, buf1, "Prev:", _ ("Prev:"));
  substitutestr (buf1, buf2, "Up:", _ ("Up:"));
  attrset (topline);
  mymvhline (0, 0, ' ', maxx);	/* pads line with spaces -- estetic */
  buf2len=strlen(buf2);
  if (buf2len)
    buf2[buf2len - 1] = '\0';
  if(buf2len>column)
  mvaddstr (0, 0, buf2+column);
  attrset (normal);
  xfree (buf1);
  xfree (buf2);
}

void
showscreen (char **message, char *type, long lines, long pos, long cursor, int column)
{
  long i;
#ifdef getmaxyx
  getmaxyx (stdscr, maxy, maxx);
#endif
#ifdef HAVE_BKGDSET
  bkgdset (' ' | normal);
#endif
  attrset (normal);
  for (i = pos; (i < lines) && (i < pos + maxy - 2); i++)
    {
      int tmp = strlen (message[i]) - 1;
      message[i][tmp] = 0;
      if(tmp>column)
        mvaddstr (i + 1 - pos, 0, message[i]+column);
      else
        move(i + 1 - pos,0);
#ifdef HAVE_BKGDSET
      clrtoeol ();
#else
      myclrtoeol ();
#endif
      message[i][tmp] = '\n';
    }
  clrtobot ();
#ifdef HAVE_BKGDSET
  bkgdset (0);
#endif
  attrset (bottomline);
  mymvhline (maxy - 1, 0, ' ', maxx);
  move (maxy - 1, 0);
  if ((pos < lines - 1) && (lines > pos + maxy - 2))
    printw (_ ("Viewing line %d/%d, %d%%"), pos + maxy - 2, lines, ((pos + maxy - 2) * 100) / lines);
  else
    printw (_ ("Viewing line %d/%d, 100%%"), lines, lines);
  info_add_highlights (pos, cursor, lines, column, message);
  attrset (normal);
  move (0, 0);
  refresh ();
}

void info_addstr(int y, int x, char *txt, int column, int txtlen)
	/* prints a line, taking care for the horizontal scrolling.
	   if the string fits in the window, it is drawn. If not,
	   it is either cut, or completely ommited. */
{
  if(x>column)
    mvaddstr(y,x-column,txt);
  else if(x+txtlen>column)
    mvaddstr(y,0,txt+(column-x));
}

void
info_add_highlights (int pos, int cursor, long lines, int column, char **message)
{
  int i, j;
  for (i = 0; i < hyperobjectcount; i++)
    {
      if ((hyperobjects[i].line >= pos) &&
	  (hyperobjects[i].line < pos + (maxy - 2)))
	{
	  /* first part of if's sets the required attributes */
	  if (hyperobjects[i].type < 2)		/* menu */
	    {
	      if (i == cursor)
		attrset (menuselected);
	      else
		attrset (menu);
	    }
	  else if (hyperobjects[i].type < 4)	/* note */
	    {
	      if (i == cursor)
		attrset (noteselected);
	      else
		attrset (note);
	    }
	  else if (hyperobjects[i].type < HIGHLIGHT)	/* url */
	    {
	      if (i == cursor)
		attrset (urlselected);
	      else
		attrset (url);
	    }
	  else
	    /* quoted text -- highlight it */
	    {
	      attrset (infohighlight);
	    }
	  if (hyperobjects[i].file[0] == 0)	/* now we start actual drawing */
	    {
	      if (hyperobjects[i].breakpos == -1)
		{
		  info_addstr (1 + hyperobjects[i].line - pos,
			    hyperobjects[i].col,
			    hyperobjects[i].node,
			    column,
			    hyperobjects[i].nodelen);
	          
		}
	      else
		{
		  char tmp = hyperobjects[i].node[hyperobjects[i].breakpos];
		  hyperobjects[i].node[hyperobjects[i].breakpos] = 0;
		  info_addstr (1 + hyperobjects[i].line - pos,
			    hyperobjects[i].col,
			    hyperobjects[i].node,
			    column,
			    hyperobjects[i].breakpos);
		  hyperobjects[i].node[hyperobjects[i].breakpos] = tmp;
		  j = hyperobjects[i].breakpos;
		  while (hyperobjects[i].node[j] == ' ')
		    j++;	/* skip leading spaces after newline */
		  if (hyperobjects[i].line - pos + 3 < maxy)
		    info_addstr (1 + hyperobjects[i].line - pos + 1,
			      j - hyperobjects[i].breakpos,
			      hyperobjects[i].node + j,
			      column,
			      hyperobjects[i].nodelen-j);
		}
	    }
	  else
	    {
	      if (hyperobjects[i].breakpos == -1)
		{
		  char *buf=xmalloc(hyperobjects[i].filelen+hyperobjects[i].nodelen+3);
		  snprintf (buf,hyperobjects[i].filelen+hyperobjects[i].nodelen+3,
		            "(%s)%s",hyperobjects[i].file,hyperobjects[i].node);
		  info_addstr (1 + hyperobjects[i].line - pos,
			hyperobjects[i].col,
			buf,
			column,
			hyperobjects[i].filelen+hyperobjects[i].nodelen+2);
		  xfree(buf);
		}
	      else
		{
		  static char buf[1024];
		  char tmp;
		  strcpy (buf, "(");
		  strcat (buf, hyperobjects[i].file);
		  strcat (buf, ")");
		  strcat (buf, hyperobjects[i].node);
		  tmp = buf[hyperobjects[i].breakpos];
		  buf[hyperobjects[i].breakpos] = 0;
		  info_addstr (1 + hyperobjects[i].line - pos,
			    hyperobjects[i].col,
			    buf,
			    column,
			    hyperobjects[i].breakpos+2);
		  buf[hyperobjects[i].breakpos] = tmp;
		  j = hyperobjects[i].breakpos;
		  while (buf[j] == ' ')
		    j++;	/* skip leading spaces after newline */
		  if (hyperobjects[i].line - pos + 3 < maxy)
		    info_addstr (1 + hyperobjects[i].line - pos + 1,
			      j - hyperobjects[i].breakpos,
			      buf + j,
			      column,
			      hyperobjects[i].filelen+hyperobjects[i].nodelen+2-j);
		}
	    }
	  attrset (normal);
	}
    }
#ifndef ___DONT_USE_REGEXP_SEARCH___
  if ((h_regexp_num) || (aftersearch))
    {
      regmatch_t pmatch[1];
      long maxpos = pos + (maxy - 2);
      if (maxpos > lines)
	maxpos = lines;
      for (i = pos; i < maxpos; i++)
	{
	  int maxregexp = aftersearch ? h_regexp_num + 1 : h_regexp_num;
	  /* if it is after search, then we have user defined regexps+
	     a searched regexp to highlight */
	  for (j = 0; j < maxregexp; j++)
	    {
	      char *str = message[i];
	      while (!regexec (&h_regexp[j], str, 1, pmatch, 0))
		{
		  int n = pmatch[0].rm_eo - pmatch[0].rm_so, k;
		  int y = i - pos + 1, x = calculate_len (message[i], pmatch[0].rm_so + str);
		  int txtoffset = pmatch[0].rm_so + str - message[i];
		  char tmp;
		  tmp = message[i][x + n];
		  message[i][x + n] = 0;
		  attrset (searchhighlight);
		  mvaddstr (y, x, message[i] + txtoffset);
		  attrset (normal);
		  message[i][x + n] = tmp;
		  str = str + pmatch[0].rm_eo;
		}
	    }
	}
    }
#endif
}
