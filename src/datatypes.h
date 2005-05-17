#ifndef __DATATYPES_H
#define __DATATYPES_H

#define FREE 0
#define LOCKED 1

#define KEEP_HISTORY 1
#define KILL_HISTORY 2

#define SOFT_HYPHEN 0xAD
#define KEY_NOTHING 99999

#define HIGHLIGHT 1000

typedef struct
  {
    char lastsearch[256];	/* last searched regexp */
    char type;			/* type of the last search (global/local) */
    int search;			/* if true -- search again */
  }
SearchAgain;

typedef struct
  {
    char filename[256];		/* name of file, where's the given offset */
    long offset;		/* offset of the node */
  }
Indirect;

typedef struct
  {
    char nodename[256];		/* name of the node */
    long offset;		/* offset of the node */
  }
TagTable;

typedef struct
  {
    int length;
    char **node;		/* array of history of nodes */
    char **file;		/* array of history of files, associated with given nodes */
    int *pos;			/* history of pos offsets in viewed nodes */
    int *cursor;		/* history of cursor offsets in viewed nodes */
    int *menu;			/* history of menu positions (in sequential reading) in viewed nodes */
  }
InfoHistory;

typedef struct
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
    char node[256];		/* name of the referenced node */
    int nodelen;		/* length of string node */
    char file[256];		/* name of the referenced file -- none=this file */
    int filelen;		/* length of string file */
    int tagtableoffset;		/* offset in tag table */
  }
HyperObject;

extern int verbose;

extern char *filenameprefix;	/* Prefix directory of the infopage.
				   It is used when we view a given set of
				   infopages, eg. bfd* pages. We want all
				   pages to be read from one directory. And
				   this path points to that directory, and
				   openinfo() will try to open the file
				   only in this directory (if this variable
				   is set nonzero) */

extern char *httpviewer;	/* name of http viewer (i.e. lynx) */
extern char *ftpviewer;		/* name of ftp viewer */
extern char *maileditor;	/* name of maileditor */
extern char *printutility;	/* name of the printing utility */
extern char *manlinks;		/* man sections, considered to be highlighted 
				 * as links */
extern char *configuredinfopath;	/* configured paths to infopages */
extern char *ignoredmacros;	/* groff/troff macros which are removed while
				   preformatting manual page */
extern char *rcfile;		/* a user specified rc file */

extern char *tmpfilename1;	/* temporary filename */
extern char *tmpfilename2;	/* second tmp filename--needed by regexp 
				 * search, etc */

extern SearchAgain searchagain;	/* a structure for "search again" feature */

extern HyperObject *hyperobjects;	/* an array of references for info */
extern int hyperobjectcount;
extern Indirect *indirect;	/* an array of indirect entries [1 to n] */
extern int IndirectEntries;	/* number of indirect entries */
extern TagTable *tag_table;	/* an array of tag table entries [1 to n] */
extern long FirstNodeOffset;	/* offset of the first node in info file */
extern char FirstNodeName[256];	/* name of the first node in info file */
extern int TagTableEntries;	/* number of tag table entries */
extern int maxx, maxy;		/* maximum dimensions of screen */
extern InfoHistory infohistory;
extern int npos;		/* position to by set when moving via history */
extern int ncursor;		/* cursor pos to be set when..... as above */
extern int nmenu;		/* sequential reading menu pos..... as above */
extern int use_apropos;		/* determines if the apropos should be called
				   if searching for aproprimate document fails */
extern int plain_apropos;	/* determines if we want only apropos output
				   to be displayed */
extern int CutManHeaders;	/* determines if man handling routines should
				   try to cut off the repeating headers */
extern int CutEmptyManLines;	/* determines if man loading routines should
				   try to cut out the repeating empty 
				   double-newlines */
extern int ForceManualTagTable;	/* Determines if you wish to initialize the
				   tag table automaticaly, or you wish
				   that pinfo does it alone. Some info pages
				   may have corrupt tag table (i.e. some
				   versions of jed pages */
extern int LongManualLinks;	/* Causes manual link sections to be treated
				   as long names (i.e. 3x11 instead of 3) */
extern char *ManOptions;	/* options passed to the `man' program */
extern char *StderrRedirection;	/* shell code to redirect stderr output */
extern int FilterB7;		/* convert 0xb7 values in man pages to 'o'? */
extern int ConfirmQuit;		/* determines if pinfo should ask for quit 
				   confirmation */
extern int QuitConfirmDefault;	/* determines the deafult answer to yes/no
				   dialog, when finishing work with pinfo */
extern int ClearScreenAtExit;	/* determines if pinfo should clear the screen
				   at exit */
extern int CallReadlineHistory;	/* determines whether when using readline wrapper
				   to call the latest history entry as default
				   prompt or not */				   
				   
extern int quote_ignored;	/* quote ignored macros when watching page */

extern int winchanged;		/* set by SIGWINCH handler */

void inithistory ();		/* initialize history (see struct above) 
				 * variables for `lastread' history */
void addinfohistory (char *file, char *node, int cursor, int menu, int pos);
				/* adds a history entry to the info file
				 * `lastread' history */
void dellastinfohistory ();	/* deletes last history entry */

void clearfilenameprefix ();	/* clears the default searchpath for openinfo() */

#endif
