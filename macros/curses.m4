dnl Curses detection: 
dnl Copyright (c) 2005 by Bas Zoetekouw <bas@debian.org>

dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of version 2 of the GNU General Public License as
dnl  published by the Free Software Foundation.
dnl
dnl  This program is distributed in the hope that it will be useful, but
dnl  WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  General Public License for more details.
dnl 
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program; if not, write to the Free Software
dnl  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
dnl  
dnl 
dnl  How to use this?
dnl  (-) put a call to AC_CHECK_CURSES in configure.ac (after the call to
dnl      AC_PROG_CC, obviously)
dnl  (-) AC_CHECK_CURSES will look for a curses library (or look at the
dnl  	location the user specified on the ./configure command line) , and
dnl  	checks if it is usable.  If it is found, and is usable, the shell
dnl  	variable $USE_CURSES is set for use in configure.ac, and the
dnl  	autoheader USE_CURSES will be defined.  
dnl  (-) the includes and libraries cc options are put into the
dnl      $CURSES_INCLUDES and $CURSES_LIBS shell variables and the automake 
dnl      variables @CURSES_INCLUDES@ and @CURSES_LIBS@.  Additional flags 
dnl      that are needed are put in $CURSES_FLAGS and @CURSES_FLAGS@



dnl First define the headers for config.h.in:
AH_TEMPLATE([USE_CURSES],[
	Use Ncurses?
])
AH_TEMPLATE([CURSES_WCHAR],[
	Does curses have support for non-iso8859 charsets?
])
AH_TEMPLATE(CURSES_H_NAME, [
	Name of the ncurses.h file that should be included
])
AH_TEMPLATE([CURSES_COLORS],[
	Defined if curses has support for colors
])
AH_TEMPLATE(CURSES_KEY_END,[
	Defined if curses has KEY_END definition
])
AH_TEMPLATE(CURSES_MOUSE,[
	Defined if curses has support for mouse events
])

dnl
dnl  "main" function.  Call this from configure.ac
dnl
AC_DEFUN([AC_CHECK_CURSES],[
	CFLAGS=${CFLAGS--O}

	curses_location=false
	use_curses=true
	AC_ARG_WITH(curses,
		[  --with-curses=dir       Use <dir> as base dir for curses library],
		[
			if test "x$withval" = "xno"
			then
				use_curses=false
			fi
			if test "x$withval" != "xyes"
			then
				curses_location=$withval
			fi
		]
	)

	dnl  search for (n)curses.h
	if test "x$use_curses" = "xtrue"
	then
		AC_SEARCH_CURSES_H()
	fi
		
	dnl  if we found anything, check if it works
	if test "x$found_curses_h" = "xtrue"; then
		AC_CHECK_CURSES_COMPILE()
	else
		curses_usable=false
	fi

	dnl  if it works, check for several features and set variables
	if test "x$curses_usable" = "xtrue"
	then
		USE_CURSES=true

		dnl  use curses
		AC_DEFINE(USE_CURSES)

		dnl  define lib and include dirs
		CURSES_INCLUDES=$curses_includes
		CURSES_LIBS=$curses_libs
		AC_SUBST(CURSES_INCLUDES)
		AC_SUBST(CURSES_LIBS)

		dnl  name of ncurses.h file
		AC_DEFINE_UNQUOTED(CURSES_H_NAME, [<$curses_h>])

		dnl  colors?
		AC_CURSES_CHECK_COLORS()
		if test "x$curses_colors" = "xtrue"
		then
			AC_DEFINE(CURSES_COLORS)
			CURSES_COLORS=true
		else
			CURSES_COLORS=false
		fi

		dnl  mouse?
		AC_CURSES_CHECK_MOUSE()
		if test "x$curses_mouse" = "xtrue"
		then
			AC_DEFINE(CURSES_MOUSE)
			CURSES_MOUSE=true
		else
			CURSES_MOUSE=false
		fi

		dnl  wchar?
		AC_CURSES_CHECK_WIDECHAR()
		if test "x$curses_wchar" = "xfalse"
		then
			CURSES_WCHAR=false
		else
			AC_DEFINE(CURSES_WCHAR)
			CURSES_WCHAR=true
			CURSES_FLAGS=$curses_wchar
			AC_SUBST(CURSES_FLAGS)
		fi

		dnl  end key?
		AC_CURSES_CHECK_ENDKEY()
		if test "x$curses_endkey" = "xtrue"
		then
			AC_DEFINE(CURSES_KEY_END)
			CURSES_KEY_END=true
		else
			CURSES_KEY_END=false
		fi

	fi
		
])


		
dnl
dnl searches for curses in a specific place
dnl Parameters: directory filename curses_LIBS curses_INCLUDES
dnl
AC_DEFUN([AC_SEARCH_CURSES_FILE], [
	if test "x$stop_searching" = "xfalse"
	then
		if test -f $1/$2
		then
			AC_MSG_RESULT([$1/$2])
			curses_h="$2"
			curses_libs="$3"
			curses_includes="$4"
			stop_searching=true
			found_curses_h=true
		fi
	fi
])

dnl
dnl  Search for curses.h in several different places
dnl
AC_DEFUN([AC_SEARCH_CURSES_H], [
	AC_MSG_CHECKING([location of curses.h file])

	stop_searching=false
	found_curses_h=false

	dnl  if a particular location was specified
	if test "x$curses_location" != "xfalse"
	then
		dnl  check this particular location
		AC_SEARCH_CURSES_FILE($curses_location/include, ncursesw/ncurses.h, 
					-L$curses_location/lib -lncursesw, 
					-I$curses_location/include )
		AC_SEARCH_CURSES_FILE($curses_location/include, ncursesw.h, 
					-L$curses_location/lib -lncursesw, 
					-I$curses_location/include )
		AC_SEARCH_CURSES_FILE($curses_location/include, ncurses/ncurses.h, 
					-L$curses_location/lib -lncurses, 
					-I$curses_location/include )
		AC_SEARCH_CURSES_FILE($curses_location/include, ncurses.h, 
					-L$curses_location/lib -lncurses, 
					-I$curses_location/include )
		AC_SEARCH_CURSES_FILE($curses_location/include, curses.h, 
					-L$curses_location/lib -lncurses, 
					-I$curses_location/include )
	
	else
		dnl  check lots of default locations

		dnl  first preference is ncursesw
		AC_SEARCH_CURSES_FILE(/usr/local/include, ncursesw/ncurses.h, 
					-L/usr/local/lib -lncursesw, 
					-I/usr/local/include )
		AC_SEARCH_CURSES_FILE(/usr/local/include/, ncursesw.h, 
					-L/usr/local/lib -lncursesw, 
					-I/usr/local/include )
	
		AC_SEARCH_CURSES_FILE(/usr/include, ncursesw/ncurses.h, 
					-L/usr/lib -lncursesw, 
					-I/usr/include )
		AC_SEARCH_CURSES_FILE(/usr/include/, ncursesw.h, 
					-L/usr/lib -lncursesw, 
					-I/usr/include )
			
		dnl  after that, look for normal ncurses
		AC_SEARCH_CURSES_FILE(/usr/local/include, ncurses.h, 
					-L/usr/local/lib -lncurses, 
					-I/usr/local/include )
		AC_SEARCH_CURSES_FILE(/usr/local/include, ncurses/ncurses.h, 
					-L/usr/local/lib -lncurses, 
					-I/usr/local/include )
	
		AC_SEARCH_CURSES_FILE(/usr/include, ncurses.h, 
					-L/usr/lib -lncurses,
					-I/usr/include )
		AC_SEARCH_CURSES_FILE(/usr/include, ncurses/ncurses.h, 
					-L/usr/lib -lncurses, 
					-I/usr/include )

		dnl  after that, look for curses
		AC_SEARCH_CURSES_FILE(/usr/local/include, curses.h, 
					-L/usr/local/lib -lncurses, 
					-I/usr/local/include )
		AC_SEARCH_CURSES_FILE(/usr/local/include, curses/curses.h, 
					-L/usr/local/lib -lncurses, 
					-I/usr/local/include )
	
		AC_SEARCH_CURSES_FILE(/usr/include, curses.h, 
					-L/usr/lib -lncurses,
					-I/usr/include )
		AC_SEARCH_CURSES_FILE(/usr/include, curses/curses.h, 
					-L/usr/lib -lncurses, 
					-I/usr/include )
	fi

])

dnl
dnl check if the curses header we found, works
dnl
AC_DEFUN([AC_CHECK_CURSES_COMPILE], [

	dnl save CFLAGS and LIBS and set new ones
	CFLAGS_OLD=$CFLAGS
	CFLAGS="$CFLAGS $curses_includes"
	LIBS_OLD=$LIBS
	LIBS="$LIBS $curses_libs"

	dnl do the compile test 
	AC_MSG_CHECKING([if curses is usable])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM(
			[[
				#include <$curses_h> 
			]], 
			[[
				initscr();
				printw("Hello World !!!");
				refresh();
				getch();
				endwin();
				return 0;
			]]
		)],
		[
			curses_usable=true
			AC_MSG_RESULT([yes])
		],
		[
			curses_usable=false
			AC_MSG_RESULT([no])
		]
	)

	dnl restore variables
	CFLAGS=$CFLAGS_OLD
	LIBS=$LIBS_OLD

])

dnl
dnl  check if this version of curses has support for colors
dnl 
AC_DEFUN([AC_CURSES_CHECK_COLORS], [

	dnl save and change CFLAGS
	CFLAGS_OLD=$CFLAGS
	CFLAGS="$CFLAGS $curses_includes"
	
	dnl  print nice message
	AC_MSG_CHECKING([if ncurses supports colors])

	dnl  we check for definition of COLOR_YELLOW
	AC_EGREP_CPP(
		HAVE_COLOR_YELLOW,
		[
    		#include <$curses_h>
    		#ifdef COLOR_YELLOW
		        HAVE_COLOR_YELLOW
		    #endif
		],
		[
    		curses_colors=true
		    AC_MSG_RESULT(yes)
		],
		[
    		curses_colors=false
		    AC_MSG_RESULT(no)
		]
	)

	dnl  restore CFLAGS
	CFLAGS=$CFLAGS_OLD

])
dnl
dnl  check if this version of curses can handle utf8 and friends
dnl 
AC_DEFUN([AC_CURSES_CHECK_WIDECHAR], [

	dnl save and change CFLAGS
	CFLAGS_OLD=$CFLAGS
	CFLAGS="$CFLAGS $curses_includes"
	
	dnl  print nice message
	AC_MSG_CHECKING([if ncurses supports extended chars])

	dnl  this will contain the define we need
	curses_wchar=false

	dnl  first check with default options
	dnl  we check for existence of the add_wch() function
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM(
			[[ #include <$curses_h> ]], 
			[[ char *p = (char *) add_wch; ]]
		)],
		[
			dnl if found, set variables and print result
			curses_wchar=true
			AC_MSG_RESULT([yes])
		],
		[
			dnl if not found, do NOP
			true
		]
	)

	dnl  not found, now try with -D_XOPEN_SOURCE_EXTENDED
	if test "x$curses_wchar" = "xfalse"
	then
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM(
				[[ 
					#define _XOPEN_SOURCE_EXTENDED
					#include <$curses_h> 
				]], 
				[[ char *p = (char *) add_wch; ]]
			)],
			[
				dnl if found, set variables and print result
				curses_wchar=-D_XOPEN_SOURCE_EXTENDED
				AC_MSG_RESULT([yes, with $curses_wchar])
			],
			[
				dnl if not found, do NOP
				true
			]
		)
	fi
	
	dnl  restore CFLAGS
	CFLAGS=$CFLAGS_OLD

	dnl  print message if not found
	if test "x$curses_wchar" = "xfalse"
	then
		AC_MSG_RESULT([no]);
	fi
	
])

dnl
dnl  check if this version of curses has an end key definition
dnl 
AC_DEFUN([AC_CURSES_CHECK_ENDKEY], [

	dnl save and change CFLAGS
	CFLAGS_OLD=$CFLAGS
	CFLAGS="$CFLAGS $curses_includes"
	
	dnl  print nice message
	AC_MSG_CHECKING([if ncurses supports the end key])

	dnl  we check for definition of HAVE_KEY_END
	AC_EGREP_CPP(
		HAVE_KEY_END,
		[
    		#include <$curses_h>
    		#ifdef KEY_END
		        HAVE_KEY_END
		    #endif
		],
		[
    		curses_endkey=true
		    AC_MSG_RESULT(yes)
		],
		[
    		curses_endkey=false
		    AC_MSG_RESULT(no)
		]
	)

	dnl  restore CFLAGS
	CFLAGS=$CFLAGS_OLD

])

dnl
dnl  check if this version of curses has support for mouse events
dnl 
AC_DEFUN([AC_CURSES_CHECK_MOUSE], [

	dnl save and change CFLAGS
	CFLAGS_OLD=$CFLAGS
	CFLAGS="$CFLAGS $curses_includes"
	
	dnl  print nice message
	AC_MSG_CHECKING([if ncurses supports mouse events])

	dnl  we check for definition of BUTTON1_CLICKED
	AC_EGREP_CPP(
		HAVE_BUTTON1_CLICKED,
		[
    		#include <$curses_h>
    		#ifdef BUTTON1_CLICKED
		        HAVE_BUTTON1_CLICKED
		    #endif
		],
		[
    		curses_mouse=true
		    AC_MSG_RESULT(yes)
		],
		[
    		curses_mouse=false
		    AC_MSG_RESULT(no)
		]
	)

	dnl  restore CFLAGS
	CFLAGS=$CFLAGS_OLD

])

dnl vim:ts=4:sw=4
