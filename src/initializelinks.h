#ifndef __INITIALIZELINKS_H
#define __INITIALIZELINKS_H
void freelinks ();		/* frees node-links */
/* initializes node links.  */
void initializelinks (char *line1, char *line2, int line);
/*
 * scans for url end in given url-string.
 * returns a pointer to the found place.
 */
char *findurlend (char *str);
/* scans for the beginning of username. Returns a pointer to it.  */
char *findemailstart (char *str);
/* strcmp, which is insensitive to whitespaces */
int compare_tag_table_string (char *base, char *compared);
/*
 * calculate length of visible part of string ('\t' included) between start and
 * end. Returns length.
 */
int calculate_len (char *start, char *end);
#endif
