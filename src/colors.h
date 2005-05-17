#ifndef __COLORS_H
#define __COLORS_H

/* numbers of color pairs in curses color definitions */

#define NORMAL           1
#define MENUSELECTED     2
#define NOTESELECTED     3
#define MENU             4
#define NOTE             5
#define TOPLINE          6
#define BOTTOMLINE       7
#define MANUALBOLD       8
#define MANUALITALIC     9
#define URL              10
#define URLSELECTED      11
#define INFOHIGHLIGHT    12
#define SEARCHHIGHLIGHT  13

/* those bellow hold color attributes for named screen widgets */

extern int menu;
extern int menuselected;
extern int note;
extern int noteselected;
extern int normal;
extern int topline;
extern int bottomline;
extern int manualbold;
extern int manualitalic;
extern int url;
extern int urlselected;
extern int infohighlight;
extern int searchhighlight;

void initcolors ();		/* 
				 * initialize color values/attributes/etc. 
				 * Either for color and monochrome mode.
				 */

#endif
