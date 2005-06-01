#ifndef __KEYBOARD_H
#define __KEYBOARD_H

/* escape or alt key */
#define META_KEY 0x1b

/* adapted from Midnight Commander */

/* macro to get CTRL+key sequence */
#define KEY_CTRL(x) ((x)&31)
/* macro to get ALT+key sequence */
#define KEY_ALT(x) (0x200 | (x))
#define is_enter_key(c) ((c) == '\r' || (c) == '\n' || (c) == KEY_ENTER)

/***********************************/

/* a structure, which holds the keybindings */
extern struct keybindings keys;

#endif
