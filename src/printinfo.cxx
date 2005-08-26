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
#include <string>
using std::string;

RCSID("$Id$")

/*
 * Algorithm: We first print highlights, then we send `\r' to the printer,
 * and we draw the base line. Thus highlights are printed `twice', and
 * are darker than the rest :)
 */
void
printnode(char ***message, long *lines)
{
	/* printer fd */
	FILE *prnFD;

	prnFD = popen(printutility, "w");

	/* scan through all lines */
	for (int i = 1; i < (*lines); i++) {
		/*
		 * This says where the printer's head is right now,
		 * offset in columns from the beginning of the line
		 */
		int lineprinted = 0;
		/*
		 * Handle the highlights which belong to our (i'th) line.
		 */
		int highlight = 0; /* counter to track which highlights have been handled */
		while (hyperobjects[highlight].line <= i) {
			string mynode;
			/* build a complete highlighted text */
			if (hyperobjects[highlight].file[0] == 0)
				mynode = hyperobjects[highlight].node;
			else {
				mynode.assign("(");
				mynode.append(hyperobjects[highlight].file);
				mynode.append(")");
				mynode.append(hyperobjects[highlight].node);
			}
			/* if it's a contiunuation of last's line highlight */
			if (hyperobjects[highlight].line == i - 1) {
				int length = 1;
				if (hyperobjects[highlight].breakpos == -1)
					length = mynode.length() - hyperobjects[highlight].breakpos;
				string trimmed;
				trimmed = mynode.substr(length - hyperobjects[highlight].breakpos,
				                        string::npos);
				fputs(trimmed.c_str(), prnFD);
				lineprinted += trimmed.length();
			} else if (hyperobjects[highlight].line == i) {
				for (int j = 0; j < hyperobjects[highlight].col - lineprinted; j++)
					fputc(' ', prnFD);
				fputs(mynode.c_str(), prnFD);
				lineprinted = hyperobjects[highlight].col + mynode.length();
			}
			if (highlight < hyperobjectcount - 1)
				highlight++;
			else
				break;
		}
		/* Carriage return and print the whole line. */
		fputc('\r', prnFD);
		fputs( (*message)[i], prnFD);
	}
	pclose(prnFD);
}
