#ifndef __MENU_AND_NOTE_UTILS_H
#define __MENU_AND_NOTE_UTILS_H

#define ERRNODE "ERR@!#$$@#!%%^#@!OR"

/* checks whether a line contains menu */
int ismenu (const char *line);
/* checks whether a line contains note */
int isnote (char *line, char *nline);
/* reads menu token from line */
int getmenutoken (char *line);
/* reads note token from line */
int getnotetoken (char *line, char *nline);
/* gets nextnode token from top line */
void getnextnode (char *type, char *node);
/* gets prevnode token from top line */
void getprevnode (char *type, char *node);
/* gets the up node token from top line */
void getupnode (char *type, char *node);
/* reads the nodename from top line */
void getnodename (char *type, char *node);
void freeindirect ();
void freetagtable ();
#endif
