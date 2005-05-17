#include "common_includes.h"

RCSID ("$Id$")

     char **rlhistory = 0;
     int rlhistorylen = 0;
     int rlhistorypos = 0;

#define KEY_BCKSPC 8

     char *
       readlinewrapper (char *prompt)
{
  char *buf = xmalloc (1024);	/* initial buffer for the read line */
  int origx, origy;		/* start coords of input line */
  int cursor = 0;		/* cursor position in input string */
  int key = 0, i;		/* key - a variable for getch() */
  buf[0] = 0;			/* initial value of line - "" */
  addstr (prompt);		/* print prompt */
  getyx (stdscr, origy, origx);	/* get origx,origy coordinates */
  noecho ();			/* turn off echoing chars by getch() */
  mvhline (origy, origx, ' ', maxx - origx);	/* create input line bar */

  rlhistorylen++;		/* history entry for this line */
  rlhistorypos = rlhistorylen;	/* move history pos to current entry */
  if (!rlhistory)
    rlhistory = xmalloc (sizeof (char *));	/* alloc memory for 
						   this entry */
  else
    rlhistory = xrealloc (rlhistory, sizeof (char *) * rlhistorylen);
  rlhistory[rlhistorylen - 1] = xmalloc (1024);
  strcpy (rlhistory[rlhistorylen - 1], buf);	/* and copy there the current
						   value of input line */
  if(CallReadlineHistory)
    ungetch(KEY_UP);	/* call history to be present */						   

  while (key != '\n')
    {
      key = getch ();		/* read key */
      switch (key)
	{
	case KEY_LEFT:		/* move cursor left */
	  if (cursor > 0)
	    cursor--;
	  break;
	case KEY_RIGHT:	/* move cursor right */
	  if (cursor < strlen (buf))
	    cursor++;
	  break;
	case KEY_END:
	  cursor = strlen (buf);
	  break;
	case KEY_BCKSPC:	/* handle backspace: copy all */
	case KEY_BACKSPACE:	/* chars starting from curpos */
	  if (cursor > 0)	/* - 1 from buf[n+1] to buf   */
	    {
	      for (i = cursor - 1; buf[i] != 0; i++)
		buf[i] = buf[i + 1];
	      cursor--;
	    }
	  break;
	case KEY_DC:		/* handle delete key. As above */
	  if (cursor <= strlen (buf) - 1)
	    {
	      for (i = cursor; buf[i] != 0; i++)
		buf[i] = buf[i + 1];
	    }
	  break;
	case KEY_UP:		/* backwards-history call */
	  if (rlhistorylen)	/* if there is history */
	    if (rlhistorypos > 1)	/* and we have */
	      {			/* where to move */
		rlhistorypos--;	/* decrement history position */
		if (rlhistorypos == rlhistorylen - 1)	/* 
							 * if the previous 
							 * pos was the input 
							 * line 
							 */
		  strcpy (rlhistory[rlhistorylen - 1], buf);	/* 
								 * save it's 
								 * value to 
								 * history 
								 */
		strcpy (buf, rlhistory[rlhistorypos - 1]);	/* 
								 * recall 
								 * value from 
								 * history to 
								 * input buf 
								 */
	      }
	  if (cursor > strlen (buf))
	    cursor = strlen (buf);
	  break;
	case KEY_DOWN:		/* forwards-history call */
	  if (rlhistorylen)
	    if (rlhistorypos < rlhistorylen)
	      {
		rlhistorypos++;
		strcpy (buf, rlhistory[rlhistorypos - 1]);
	      }
	  if (cursor > strlen (buf))
	    cursor = strlen (buf);
	  break;
	  /* eliminate nonprintable chars */
	case '\n':
	case KEY_PPAGE:
	case KEY_NPAGE:
	case KEY_F (1):
	case KEY_F (2):
	case KEY_F (3):
	case KEY_F (4):
	case KEY_F (5):
	case KEY_F (6):
	case KEY_F (7):
	case KEY_F (8):
	case KEY_F (9):
	case KEY_F (10):
	  break;
	default:
	  if (key >= 32)
	    {
	      if (strlen (buf + cursor))	/* if the cursor is */
		{		/* not at the last pos */
		  char *tmp = 0;
		  tmp = xmalloc (strlen (buf + cursor) + 1);
		  strcpy (tmp, buf + cursor);
		  buf[cursor] = key;
		  buf[cursor + 1] = 0;
		  strcat (&buf[cursor + 1], tmp);
		  xfree (tmp);
		  cursor++;
		}
	      else
		{
		  buf[cursor + 1] = 0;
		  buf[cursor] = key;
		  cursor++;
		}
	    }
	}
      move (origy, origx);
      for (i = origx; i < maxx; i++)
	addch (' ');
      move (origy, origx);
      addstr (buf);
      move (origy, origx + cursor);

    }
  strcpy (rlhistory[rlhistorylen - 1], buf);
  if (strlen (buf))
    {
      rlhistory[rlhistorylen - 1] = xrealloc (rlhistory[rlhistorylen - 1],
				  strlen (rlhistory[rlhistorylen - 1]) + 1);
    }
  else
    {
      xfree (rlhistory[rlhistorylen - 1]);
      rlhistorylen--;
      rlhistorypos = rlhistorylen;
    }
  buf = xrealloc (buf, strlen (buf) + 1);
  return buf;
}
