#ifndef __FILEHANDLING_FUNCTIONS_H
#define __FILEHANDLING_FUNCTIONS_H

#include <dirent.h>

#define INFO_TAG 0x1f
#define INDIRECT_TAG 0x7f

void initpaths ();
void addrawpath (char *filename);

void seeknode (int tag_table_pos, FILE ** Id);	/* 
						 * seek to a node in certain
						 * info file 
						 */

void freeitem (char **type, char ***buf, long *lines);
  /*
   * free allocated memory, hold by buf (node** content, stored line by line),
   * and type (a char* pointer, which stores the node header).
   */

void read_item (FILE * id, char **type, char ***buf, long *lines);
/*  
 * reads a node from 'id' to 'buf', and the header of node to 'type'. It sets
 * the numer of read lines to *lines. Warning! First line of 'buf' is left
 * empty.
 */
int seek_indirect (FILE * id);	/* searches for indirect entry of info file */
int seek_tag_table (FILE * id,int quiet);	/* as above, but with tag table entry */
void load_indirect (char **message, long lines);
 /* 
  * loads indirect table (from a special node, stored in message, of lines 
  * length)
  */
void load_tag_table (char **message, long lines);
 /* 
  * loads tag table (as above) 
  */
FILE *openinfo (char *filename, int number);	/* opens info file */
FILE *opendirfile (int number);	/* opens dir info file */

void create_tag_table (FILE * id);	/* creates tag table for info file */
void create_indirect_tag_table ();	/* creates tag table for indirect info */

FILE *
  dirpage_lookup (char **type, char ***message, long *lines,
		  char *filename, char **first_node);
/*              
 * look up a name, which was specified by the user in cmd line, in dir 
 * entries. If found, return filedescriptor of the info file, which holds 
 * needed entry. Also set `first node' to the name of node, which describes 
 * the problem. Arguments:
 * type: a pointer to char*, which will hold the header line of dir entry
 * message: a pointer to char** buffer, which will hold the dir page line by 
 * line
 * lines: pointer to long, which holds the number of lines in dir entry
 */

void strip_compression_suffix (char *file);	/* removes trailing .gz, .bz2, etc. */

#endif
