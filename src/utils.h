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

void pinfo_re_comp (char *name);	/* wrappers for re_comp and re_exec */
int pinfo_re_exec (char *name);

int pinfo_getch ();		/* user defined getch, capable of handling
				   ALT keybindings */
void xfree (void *ptr);		/* free() wrapper */
void *xmalloc (size_t size);	/* malloc() wrapper */
void *xrealloc (void *ptr, size_t size);	/* realloc() wrapper */
void initlocale ();		/* initializes GNU locales */
void checkfilename (char *filename);	/* checks if file name does not
					 * cause secuirity problems */
void closeprogram ();		/* closes the program, and removes temporary files */
void init_curses ();		/* initializes curses interface */
char *getstring (char *prompt);	/* an interface to gnu readline */
void mymvhline (int y, int x, char ch, int len);	/* for some reasons mvhline 
							   does not work quite 
							   properly... */
void myclrtoeol ();		/* this one supports color back/foreground */
void myendwin ();		/* takes care of the cursor, which is turned off */
int gettagtablepos (char *node);	/* 
					 * get offset of "node" in
					 * tag_table variable 
					 */

int yesno (char *prompt, int def);	/* 
					 * handle localized `(y/n)' dialog 
					 * box.
					 */
void copy_stripped_from_regexp (char *src, char *dest);		/* 
								 * copies the first part 
								 * of string, which is 
								 * without regexp 
								 */


void waitforgetch ();		/* Block until something's on STDIN */

extern int curses_open;		/* is curses screen open? */

#endif
