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

#ifndef __DATATYPES_H
#define __DATATYPES_H

#include <string>
#include <vector>

#define FREE 0
#define LOCKED 1

#define KEEP_HISTORY 1
#define KILL_HISTORY 2

#define SOFT_HYPHEN 0xAD
#define KEY_NOTHING 99999

#define HIGHLIGHT 1000

typedef struct
{
	std::string lastsearch;	/* last searched regexp */
	char type;				/* type of the last search (global/local) */
	int search;				/* if true -- search again */
}
SearchAgain;

typedef struct Indirect
{
	long offset;			/* offset of the node */
	std::string filename;		/* name of file, wherein the given offset is */
}
Indirect;

typedef struct TagTable
{
	long offset;			/* offset of the node */
	std::string nodename;		/* name of the node */
}
TagTable;

typedef struct
{
	int length;
	char **node;	/* array of history of nodes */
	char **file;	/* array of history of files, associated with given nodes */
	int *pos;		/* history of pos offsets in viewed nodes */
	int *cursor;	/* history of cursor offsets in viewed nodes */
	int *menu;		/* history of menu positions (in sequential reading) in viewed nodes */
}
InfoHistory;

typedef struct HyperObject
{
	int line;			/* line number of the place where the link is */
	int col;			/* column number ----||---- */
	int breakpos;		/* col number, where the links breaks to next line */
	int type;			/* type of link: 0 -  * menu::,
						   1 -  * Comment: menu.
						   2 -  *note note::
						   3 -  *note Comment: note.
						   4 -  http url
						   5 -  ftp url
						   6 -  mailto url */
	std::string node;		/* name of the referenced node */
	std::string file;		/* name of the referenced file -- empty=this file */
	int tagtableoffset;	/* offset in tag table */
}
HyperObject;

extern int verbose;

/*
 * Prefix directory of the infopage.  It is used when we view a given set of
 * infopages, eg. bfd* pages. We want all pages to be read from one directory.
 * And this path points to that directory, and openinfo() will try to open the
 * file only in this directory (if this variable is set nonzero)
 */
extern std::string filenameprefix;

/* name of http viewer (i.e. lynx) */
extern std::string httpviewer;
/* name of ftp viewer */
extern std::string ftpviewer;
/* name of maileditor */
extern std::string maileditor;
/* name of the printing utility */
extern std::string printutility;
/* man sections, considered to be highlighted  as links */
extern std::string manlinks;
/* configured paths to infopages */
extern std::string configuredinfopath;
/* groff/troff macros which are removed while preformatting manual page */
extern std::string ignoredmacros;
/* a user specified rc file */
extern std::string rcfile;

/* temporary filename */
extern char *tmpfilename1;
/* second tmp filename--needed by regexp search, etc */
extern char *tmpfilename2;

/* a structure for "search again" feature */
extern SearchAgain searchagain;

/* an array of references for info */
extern std::vector<HyperObject> hyperobjects;
/* an array of indirect entries */
extern std::vector<Indirect> indirect;
/* an array of tag table entries [0 to n - 1] */
extern std::vector<TagTable> tag_table;
/* offset of the first node in info file */
extern long FirstNodeOffset;
/* name of the first node in info file */
extern std::string FirstNodeName;
/* maximum dimensions of screen */
extern int maxx, maxy;
extern InfoHistory infohistory;
/* position to by set when moving via history */
extern int npos;
/* cursor pos to be set when..... as above */
extern int ncursor;
/* sequential reading menu pos..... as above */
extern int nmenu;
/* determines if the apropos should be called if searching for aproprimate
 * document fails */
extern int use_apropos;
/* determines if we want only apropos output to be displayed */
extern int plain_apropos;
/* determines if man handling routines should try to cut off the repeating
 * headers */
extern int CutManHeaders;
/* determines if man loading routines should try to cut out the repeating empty
 * double-newlines */
extern int CutEmptyManLines;
/* Determines if you wish to initialize the tag table automaticaly, or you wish
 * that pinfo does it alone. Some info pages may have corrupt tag table (i.e.
 * some versions of jed pages */
extern int ForceManualTagTable;
/* Causes manual link sections to be treated as long names (i.e. 3x11 instead
 * of 3) */
extern int LongManualLinks;
/* options passed to the `man' program */
extern std::string ManOptions;
/* shell code to redirect stderr output */
extern std::string StderrRedirection;
/* convert 0xb7 values in man pages to 'o'? */
extern int FilterB7;
/* determines if pinfo should ask for quit confirmation */
extern int ConfirmQuit;
/* determines the deafult answer to yes/no dialog, when finishing work with
 * pinfo */
extern int QuitConfirmDefault;
/* determines if pinfo should clear the screen at exit */
extern int ClearScreenAtExit;
/* determines whether when using readline wrapper to call the latest history
 * entry as default prompt or not */
extern int CallReadlineHistory;

/* quote ignored macros when watching page */
extern int quote_ignored;

/* set by SIGWINCH handler */
extern int winchanged;

/* Needed by parse_config.cxx */
extern int use_raw_filename;

/* Needed by parse_config.cxx, in pinfo.cxx */
extern int DontHandleWithoutTagTable;

/* initialize history (see struct above) * variables for `lastread' history */
void inithistory ();
/* adds a history entry to the info file `lastread' history */
void addinfohistory (const char *file, const char *node, int cursor, int menu, int pos);
/* deletes last history entry */
void dellastinfohistory ();

#endif
