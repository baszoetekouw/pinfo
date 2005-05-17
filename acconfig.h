/*=== Curses version detection defines ===*/
/* Found some version of curses that we're going to use */
#undef HAS_CURSES

/* Use SunOS SysV curses? */
#undef USE_SUNOS_CURSES

/* Use old BSD curses - not used right now */
#undef USE_BSD_CURSES

/* Use SystemV curses? */
#undef USE_SYSV_CURSES

/* Use Ncurses? */
#undef USE_NCURSES

/* If you Curses does not have color define this one */
#undef NO_COLOR_CURSES

/* Define if you want to turn on SCO-specific code */
#undef SCO_FLAVOR

/* Set to reflect version of ncurses *
 *   0 = version 1.*
 *   1 = version 1.9.9g
 *   2 = version 4.0/4.1/4.2 */
#undef NCURSES_970530

/* Defined if found readline */
#undef HAS_READLINE

/* for GNU gettext */
#undef ENABLE_NLS
#undef HAVE_CATGETS
#undef HAVE_GETTEXT
#undef HAVE_LC_MESSAGES
#undef HAVE_STPCPY
/* where will be installed localized messages */
/* #undef LOCALEDIR */

/* Define if have sigblock function */
#undef HAVE_SIGBLOCK

/* don't show cursor */
#undef HIDECURSOR

/* don't use dynamic buffor */
#undef ___USE_STATIC___

/* Don't use regexp search engine */
#undef ___DONT_USE_REGEXP_SEARCH___

/* Package version */
#undef VERSION
/* Package name */
#undef PACKAGE

/* defined if curses has KEY_END definition */
#undef HAVE_KEY_END
