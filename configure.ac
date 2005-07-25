#***************************************************************************
#*  Pinfo is a ncurses based lynx style info documentation browser
#*
#*  Copyright (C) 1999  Przemek Borys <pborys@dione.ids.pl>
#*  Copyright (C) 2005  Bas Zoetekouw <bas@debian.org>
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
#*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
#*  USA
#***************************************************************************/
#
#
# Process this file with autoconf to produce a configure script.
# $Id$
#

# init
AC_INIT([pinfo],[0.6.8+svn],[pinfo-devel@lists.alioth.debian.org])
# require a recent autoconf
AC_PREREQ([2.57])
# for identification of derived ./configure scripts
AC_REVISION([$Revision$])

# put the config into config.h
AC_CONFIG_HEADERS([config.h])

# id main dir by src/pinfo.c file
AC_CONFIG_SRCDIR([src/pinfo.c])
# helper scripts are in tools/
AC_CONFIG_AUX_DIR([tools])

# The fairly useless --with-tags option to configure requires that
# C++ and F77 support be invoked; this makes configure horribly
# slow. Obliterating this macro pries the feature out of libtool 1.5; it
# is removed in 1.6 in favour of a different scheme.
m4_define([_LT_AC_TAGCONFIG], [])

# init automake
AM_INIT_AUTOMAKE
# disable automatic rebuilding of ./configure, Makefile.in, etc
AM_MAINTAINER_MODE
# include the m4 stuff in tools/macros
AM_ACLOCAL_INCLUDE(macros)


####################################################
## compiler stuff
####################################################
# find C compiler
AC_PROG_CC

# set correct LIBS for (almost obsolete) INTERACTIVE UNIX
AC_ISC_POSIX

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

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
AC_PROG_LIBTOOL


####################################################
## readline and curses
####################################################
# readline
AC_CHECK_READLINE
AM_CONDITIONAL(HAS_READLINE, test "$has_readline" = true)

# curses
AC_CHECK_CURSES
if  test "$has_curses" = false; then
	AC_MSG_ERROR([Curses not found. You need curses to compile pinfo])
fi
LIBS="$LIBS $CURSES_LIBS"
CPPFLAGS="$CPPFLAGS $CURSES_INCLUDES"

# check for some functions in curses
AC_CHECK_FUNCS(curs_set use_default_colors bkgdset)

# check if our curses lib knows about KEY_END
AC_MSG_CHECKING(for KEY_END in curses)
KEY_END=""
AC_EGREP_CPP(HAVE_KEY_END,	
[
	#if defined(RENAMED_NCURSES) or !defined(USE_NCURSES)
	#include <curses.h>
	#else
	#include <ncurses.h>
	#endif

	#ifdef KEY_END
		HAVE_KEY_END
	#endif
],
[
	AC_DEFINE(HAVE_KEY_END,1,[defined if curses has KEY_END definition]) 
	KEY_END="KEY_END" 
	MAN_KEY_END="End Key"
	AC_MSG_RESULT(yes)
],
[
	AC_MSG_RESULT(no) 
	KEY_END="\'E\'"
	MAN_KEY_END="E"
])

####################################################
## i18n, t10n stuff
####################################################
# the languages for which we have .po files
ALL_LINGUAS="pl cs sv de ru ja"

# Checks for all prerequisites of the intl subdirectory
AM_INTL_SUBDIR

# gettext
AM_GNU_GETTEXT_VERSION([0.14.4])
AM_GNU_GETTEXT([external])

# TVM:
# horrible *temporary* hack to make sure that if we found gettext() in
# -lintl that we add -lintl *back* to $LIBS.
#
## TODO: do we still need this?
##if test X$gt_cv_func_gettext_libintl = Xyes ; then
##    LIBS="-lintl $LIBS"
##fi

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


####################################################
## user arguments
####################################################
# do you want to see cursor when working with pinfo?
## TODO: this should really be a command line/config file option
AC_ARG_ENABLE(showcursor,
	[  --enable-cursor         enable cursor when working with pinfo [default=no]], , )
if test "x$enable_showcursor" != "xyes" ; then
	AC_DEFINE(HIDECURSOR,FALSE,[don't show cursor])
else
	AC_DEFINE(HIDECURSOR,TRUE,[don't show cursor])
fi

# If pinfo is too slow, the below definition will resign from dynamical
# allocation in critical places, and replace it with static buffers which
# should save few instructions to CPU ;)
## TODO: do we still need this with current computing powers?
AC_ARG_ENABLE(use_static,
	[  --disable-static        enable using static buffers ], ,
	enable_use_static=yes) 
if test "x$enable_use_static" = "xyes" ; then
	AC_DEFINE(___USE_STATIC___, 1, [don't use dynamic buffer])
fi

# do you want to use regexp searches?
## TODO: this should really be a command line/config file option
AC_ARG_WITH(regexp_search,
	[  --with-regexp-search    use regular expression search [default=yes]])
if test "x$with_regexp_search" = "xno"; then
        AC_DEFINE(___DONT_USE_REGEXP_SEARCH___,1,
			[Don't use regexp search engine])
fi




####################################################
## destination dirs
####################################################
CONFIGDIR=${sysconfdir}/pinforc
## TODO: have this put in config.h instead of passing it on the command line
CPPFLAGS="$CPPFLAGS -DCONFIGDIR=\\\"${sysconfdir}/pinforc\\\" -DLOCALEDIR=\\\"${LOCALEDIR}\\\""
## TODO: looks evil
MANDIR="`eval MANDIR=$mandir;echo $MANDIR|sed -e \"s#NONE#$ac_default_prefix#\"`"



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
