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

/*
 * Algorithm: We first print highlights, then we send `\r' to the printer,
 * and we draw the base line. Thus highlights are printed `twice', and
 * are darker than the rest :)
 */
void
printnode(char ***message, unsigned long *lines)
{
#define Message	(*message)
#define Lines	(*lines)

	/* counter, to point at what highlights are already * handled */
	unsigned highlight = 0;
	/* printer fd */
	FILE *prnFD;
	/* temporary buffer */
	char *buf = xmalloc(1024);

	prnFD = popen(printutility, "w");

	/* scan through all lines */
	for (unsigned i = 1; i < Lines; i++)
	{
		/*
		 * this says, where the printer's head is
		 * right now.(offset in cols from the
		 * beginning of line
		 */
		int lineprinted = 0;
		/*
		 * let's handle the highlights, which belong to our(i'th) line.
		 */
		while (hyperobjects[highlight].line <= i)
		{
			/* build a complete highlighted text */
			if (hyperobjects[highlight].file[0] == 0)
				strcpy(buf, hyperobjects[highlight].node);
			else
			{
				strcpy(buf, "(");
				strcat(buf, hyperobjects[highlight].file);
				strcat(buf, ")");
				strcat(buf, hyperobjects[highlight].node);
			}
			/* if it's a contiunuation of last's line highlight */
			if (hyperobjects[highlight].line == i - 1)
			{
				int length = 1;
				if (hyperobjects[highlight].breakpos == -1)
					length = strlen(buf) -
						hyperobjects[highlight].breakpos;
				fprintf(prnFD, "%s", buf + length -
						hyperobjects[highlight].breakpos);
				lineprinted += strlen(buf + length -
						hyperobjects[highlight].breakpos);
			}
			else if (hyperobjects[highlight].line == i)
			{
				for (unsigned j = 0; j < hyperobjects[highlight].col - lineprinted; j++)
					fprintf(prnFD, " ");
				fprintf(prnFD, "%s", buf);
				lineprinted = hyperobjects[highlight].col +
					strlen(buf);
			}
			if (highlight < hyperobjectcount - 1)
				highlight++;
			else
				break;
		}
		fprintf(prnFD, "\r%s", Message[i]);
	}
	pclose(prnFD);
	xfree(buf);
#undef Message
#undef Lines
}
