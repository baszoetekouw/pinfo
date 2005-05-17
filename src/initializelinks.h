#ifndef __INITIALIZELINKS_H
#define __INITIALIZELINKS_H
void freelinks ();		/* frees node-links */
void initializelinks (char *line1, char *line2, int line);
/* 
 * initializes node links.
 */
char *findurlend (char *str);	/* 
				 * scans for url end in given url-string.
				 * returns a pointer to the found place.
				 */
char *findemailstart (char *str);	/* 
					 * scans for the beginning of 
					 * username. Returns a pointer to it.
					 */
int compare_tag_table_string (char *base, char *compared);
		/* strcmp, which is insensitive to whitespaces */
int calculate_len (char *start, char *end);	/* 
						 * calculate length of visible
						 * part of string ('\t' 
						 * included) between start
						 * and end. Returns length. 
						 */
#endif
