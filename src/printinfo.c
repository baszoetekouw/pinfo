

#include "common_includes.h"

RCSID ("$Id$")

/*
 * Algorithm: We first print highlights, then we send `\r' to the printer,
 * and we draw the base line. Thus highlights are printed `twice', and
 * are darker than the rest :)
 */

     void
       printnode (char ***message, long *lines)
{
#define Message (*message)
#define Lines (*lines)

  int highlight = 0;		/* counter, to point at what highlights are already
				 * handled */
  int i, j;
  FILE *prnFD;			/* printer fd */
  char *buf = xmalloc (1024);	/* temporary buffer */

  prnFD = popen (printutility, "w");

  for (i = 1; i < Lines; i++)	/* scan through all lines */
    {
      int lineprinted = 0;	/* this says, where the printer's head is
				 * right now. (offset in cols from the
				 * beginning of line */
      while (hyperobjects[highlight].line <= i)		/* let's handle the highlights,
							 * which belong to our (i'th)
							 * line. */
	{
	  if (hyperobjects[highlight].file[0] == 0)	/* build a complete 
							 * highlighted text */
	    strcpy (buf, hyperobjects[highlight].node);
	  else
	    {
	      strcpy (buf, "(");
	      strcat (buf, hyperobjects[highlight].file);
	      strcat (buf, ")");
	      strcat (buf, hyperobjects[highlight].node);
	    }
	  if (hyperobjects[highlight].line == i - 1)	/* if it's a 
							 * contiunuation of
							 * last's line
							 * highlight */
	    {
	      int length = 1;
	      if (hyperobjects[highlight].breakpos == -1)
		length = strlen (buf) -
		  hyperobjects[highlight].breakpos;
	      fprintf (prnFD, "%s", buf + length -
		       hyperobjects[highlight].breakpos);
	      lineprinted += strlen (buf + length -
				     hyperobjects[highlight].breakpos);
	    }
	  else if (hyperobjects[highlight].line == i)	/* else */
	    {
	      for (j = 0; j < hyperobjects[highlight].col - lineprinted; j++)
		fprintf (prnFD, " ");
	      fprintf (prnFD, "%s", buf);
	      lineprinted = hyperobjects[highlight].col +
		strlen (buf);
	    }
	  if (highlight < hyperobjectcount - 1)
	    highlight++;
	  else
	    break;
	}
      fprintf (prnFD, "\r%s", Message[i]);
    }
  pclose (prnFD);
  xfree (buf);
#undef Message
#undef Lines
}
