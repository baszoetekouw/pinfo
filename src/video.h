
#ifndef __VIDEO_H
#define __VIDEO_H
void showscreen (char **message, char *type, long lines, long pos,
		 long cursor, int column);
						   /* the above paints the 
						      screen while viewing 
						      info file */
void mvaddstr_menu (int y, int x, char *line, int linenumber);
						   /* prints unselected menu 
						      option */
void mvaddstr_menu_selected (int y, int x, char *line, int linenumber);
						   /* prints selected menu 
						      option */
void mvaddstr_note (int y, int x, char *line, char *nline, int linenumber);
						   /* prints unselected note
						      option */
void mvaddstr_note_selected (int y, int x, char *line, char *nline, int linenumber);
						   /* prints selected note
						      option */
void addtopline (char *type, int column);	/* adds top line of info 
						   page */
#endif
