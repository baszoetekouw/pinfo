#ifndef __UTILS_H
#define __UTILS_H

extern char *safe_user;
extern char *safe_group;

#ifndef HAVE_CURS_SET
void curs_set (int a);
#endif

#ifdef ___DONT_USE_REGEXP_SEARCH___
extern char *pinfo_re_pattern;
#endif

/* wrappers for re_comp and re_exec */
void pinfo_re_comp (char *name);
int pinfo_re_exec (char *name);

/* user defined getch, capable of handling ALT keybindings */
int pinfo_getch ();
/* free() wrapper */
void xfree (void *ptr);
/* malloc() wrapper */
void *xmalloc (size_t size);
/* realloc() wrapper */
void *xrealloc (void *ptr, size_t size);
/* initializes GNU locales */
void initlocale ();
/* checks if file name does not cause secuirity problems */
void checkfilename (char *filename);
/* closes the program, and removes temporary files */
void closeprogram ();
/* initializes curses interface */
void init_curses ();
/* an interface to gnu readline */
char *getstring (char *prompt);
/* for some reasons mvhline does not work quite properly... */
void mymvhline (int y, int x, char ch, int len);
/* this one supports color back/foreground */
void myclrtoeol ();
/* takes care of the cursor, which is turned off */
void myendwin ();
/* get offset of "node" in tag_table variable */
int gettagtablepos (char *node);

/* handle localized `(y/n)' dialog box.  */
int yesno (char *prompt, int def);
/* copies the first part of string, which is without regexp */
void copy_stripped_from_regexp (char *src, char *dest);


/* Block until something's on STDIN */
void waitforgetch ();

/* is curses screen open? */
extern int curses_open;

#endif
