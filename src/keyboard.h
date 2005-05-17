#ifndef __KEYBOARD_H
#define __KEYBOARD_H

#define META_KEY 0x1b		/* escape or alt key */

/* adapted from Midnight Commander */

#define KEY_CTRL(x) ((x)&31)	/* macro to get CTRL+key sequence */
#define KEY_ALT(x) (0x200 | (x))	/* macro to get ALT+key sequence */
#define is_enter_key(c) ((c) == '\r' || (c) == '\n' || (c) == KEY_ENTER)

/***********************************/

extern struct keybindings keys;	/* a structure, which holds the keybindings */

#endif
