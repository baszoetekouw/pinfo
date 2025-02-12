#***************************************************************************
#*  Pinfo is a ncurses based lynx style info documentation browser
#*
#*  Copyright (C) 1999-2005  Przemek Borys <pborys@dione.ids.pl>
#*  Copyright (C) 2005,2006  Bas Zoetekouw <bas@debian.org>
#*
#*  This program is free software; you can redistribute it and/or modify
#*  it under the terms of version 2 of the GNU General Public License as
#*  published by the Free Software Foundation.
#*
#*  This program is distributed in the hope that it will be useful, but
#*  WITHOUT ANY WARRANTY; without even the implied warranty of
#*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#*  General Public License for more details.
#*
#*  You should have received a copy of the GNU General Public License
#*  along with this program; if not, write to the Free Software
#*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
#*  USA
#***************************************************************************/
#
#
# Process this file with autoconf to produce a configure script.
# $Id$
#

# init
AC_INIT([pinfo],[0.6.13],[https://github.com/baszoetekouw/pinfo])
# require a recent autoconf
AC_PREREQ([2.69])
# for identification of derived ./configure scripts
AC_REVISION([$Revision$])

# put the config into config.h
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([macros])

# id main dir by src/pinfo.c file
AC_CONFIG_SRCDIR([src/pinfo.c])
# helper scripts are in tools/
AC_CONFIG_AUX_DIR([tools])


# init automake
AM_INIT_AUTOMAKE
# include the m4 stuff in tools/macros
AM_ACLOCAL_INCLUDE(macros)


####################################################
## compiler stuff
####################################################
# find C compiler
AC_PROG_CC
AC_PROG_CC_C99

# set correct LIBS for (almost obsolete) INTERACTIVE UNIX
AC_SEARCH_LIBS([strerror],[cposix])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

WFLAGS="-Wall -Wextra -Wshadow -Werror"
AX_CHECK_COMPILE_FLAG($WFLAGS,
	[AC_SUBST(WFLAGS)],
	[AC_SUBST(WFLAGS,"")]
)

# Checks for library functions.
AC_CHECK_FUNCS(strdup strstr strsep)
AC_CHECK_FUNCS(getopt_long snprintf)
AM_CONDITIONAL(HAVE_SNPRINTF,test "x$ac_cv_func_snprintf" = "xyes")

AC_CHECK_FUNC(sigblock,,AC_CHECK_LIB(bsd, sigblock))
if test "x$ac_cv_lib_bsd_sigblock" = "xyes" -o \
	"x$ac_cv_func_sigblock" = "xyes" ; then
	AC_DEFINE(HAVE_SIGBLOCK,1,[Define if have sigblock function])
fi
AM_CONDITIONAL(HAVE_SIGBLOCK,test "x$ac_cv_lib_bsd_sigblock" = "xyes" -o \
        "x$ac_cv_func_sigblock" = "xyes" )

AM_PROG_CC_C_O

####################################################
## some additional utilities we need
####################################################
# find install
AC_PROG_INSTALL

# find ln -s
AC_PROG_LN_S

# set the MAKE variable for make in subdirs
AC_PROG_MAKE_SET

# find libtool
LT_INIT


####################################################
## readline and curses
####################################################
# readline
AC_CHECK_READLINE

# if we have readline, it needs to be version 5
if test "x$has_readline" = "xtrue"
then
	if test $readline_version -lt 4
	then
		AC_MSG_WARN([Version 4 of readline is required to compile pinfo.
			Yours is only version $readline_version.
			Readline support will be disabled.])
		has_readline=false
	fi
fi
AM_CONDITIONAL(HAS_READLINE, test "$has_readline" = true)


# curses
AX_WITH_CURSES
if test "x$ax_cv_curses" != xyes; then
	AC_MSG_ERROR([requires a (n)curses library])
fi
# check for some functions in curses
AC_CHECK_DECLS([curs_set, bkgdset, use_default_colors], [], [], [
#if HAVE_NCURSESW_H           /* if <ncursesw.h> should be used              */
#  include <ncursesw.h>
#elif HAVE_NCURSESW_CURSES_H  /* if <ncursesw/curses.h> should be used       */
#  include <ncursesw/curses.h>
#elif HAVE_NCURSES_H          /* if <ncurses.h> should be used               */
#  include <ncurses.h>
#elif HAVE_NCURSES_CURSES_H   /* if <ncurses/curses.h> should be used        */
#  include <ncurses/curses.h>
#elif HAVE_CURSES_H           /* if <curses.h> is present and should be used */
#  include <curses.h>
#else
#  error "No valid curses headers detected"
#endif
])

####################################################
## wchar stuff
####################################################
AC_CHECK_WCHAR
if test "x$USE_WCHAR" = "xtrue"
then
	CFLAGS="$CFLAGS $WCHAR_FLAGS"
fi


####################################################
## locales
####################################################
# the languages for which we have .po files
ALL_LINGUAS="cs de eu ja nl pl pt_BR ro ru sv uk vi"

# Checks for all prerequisites of the intl subdirectory
AM_INTL_SUBDIR

# gettext
AM_GNU_GETTEXT_VERSION([0.14.4])
AM_GNU_GETTEXT([external])

# Specify locale stuff destination
AC_ARG_WITH(localedir,
	[  --with-localedir=PATH      specify where the locale stuff should go ])
if test "x$LOCALEDIR" = "x"; then
	if test "x$with_localedir" != "x"; then
		LOCALEDIR=$with_localedir
	else
		LOCALEDIR='$(prefix)/share/locale'
	fi
fi
AC_SUBST(LOCALEDIR)


####################################################
## user arguments
####################################################
# do you want to see cursor when working with pinfo?
## TODO: this should really be a command line/config file option
AC_ARG_ENABLE(showcursor,
	[  --enable-cursor         enable cursor when working with pinfo [default=no]], , )
if test "x$enable_showcursor" != "xyes" ; then
	AC_DEFINE(HIDECURSOR,FALSE,[don't show cursor]) #'
else
	AC_DEFINE(HIDECURSOR,TRUE,[don't show cursor]) #'
fi

# If pinfo is too slow, the below definition will resign from dynamical
# allocation in critical places, and replace it with static buffers which
# should save few instructions to CPU ;)
## TODO: do we still need this with current computing powers?
AC_ARG_ENABLE(use_static,
	[  --disable-static        enable using static buffers ], ,
	enable_use_static=yes)
if test "x$enable_use_static" = "xyes" ; then
	AC_DEFINE(___USE_STATIC___, 1, [don't use dynamic buffer]) #'
fi

# do you want to use regexp searches?
## TODO: this should really be a command line/config file option
AC_ARG_WITH(regexp_search,
	[  --with-regexp-search    use regular expression search [default=yes]])
if test "x$with_regexp_search" = "xno"; then
        AC_DEFINE(___DONT_USE_REGEXP_SEARCH___,1,
			[Don't use regexp search engine]) #'
fi




####################################################
## destination dirs
####################################################

##TODO: replace these by PACKAGE_VERSION and PACKAGE_NAME
AC_DEFINE_UNQUOTED(VERSION,"$VERSION",[Package version])
AC_DEFINE_UNQUOTED(PACKAGE,"$PACKAGE",[Package name])

# export variables
AC_SUBST(KEY_END)
AC_SUBST(MAN_KEY_END)
AC_SUBST(EXTRA_SOURCES)
AC_SUBST(MANDIR)

# generate these files depending on the current configuration
AC_CONFIG_FILES([
	Makefile
	doc/Makefile
	macros/Makefile
	src/Makefile
	pinfo.spec
	po/Makefile.in
	src/pinforc
	doc/pinfo.1
])

# the end.  Output config.status and launch it.
AC_OUTPUT

# vim:ts=4
