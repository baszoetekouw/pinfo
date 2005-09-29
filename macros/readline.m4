dnl readline detection
dnl based on curses.m4 from gnome
dnl
dnl What it does:
dnl =============
dnl
dnl - Determine which version of readline is installed on your system
dnl   and set the -I/-L/-l compiler entries and add a few preprocessor
dnl   symbols 
dnl - Do an AC_SUBST on the READLINE_INCLUDES and READLINE_LIBS so that
dnl   @READLINE_INCLUDES@ and @READLINE_LIBS@ will be available in
dnl   Makefile.in's
dnl - Modify the following configure variables (these are the only
dnl   readline.m4 variables you can access from within configure.in)
dnl   READLINE_INCLUDES - contains -I's
dnl   READLINE_LIBS       - sets -L and -l's appropriately
dnl   has_readline        - exports result of tests to rest of configure
dnl
dnl Usage:
dnl ======
dnl 1) call AC_CHECK_READLINE after AC_PROG_CC in your configure.in
dnl 2) Make sure to add @READLINE_INCLUDES@ to your preprocessor flags
dnl 3) Make sure to add @READLINE_LIBS@ to your linker flags or LIBS
dnl
dnl Notes with automake:
dnl - call AM_CONDITIONAL(HAS_READLINE, test "$has_readline" = true) from
dnl   configure.in
dnl - your Makefile.am can look something like this
dnl   -----------------------------------------------
dnl   INCLUDES= blah blah blah $(READLINE_INCLUDES) 
dnl   if HAS_READLINE
dnl   READLINE_TARGETS=name_of_readline_prog
dnl   endif
dnl   bin_PROGRAMS = other_programs $(READLINE_TARGETS)
dnl   other_programs_SOURCES = blah blah blah
dnl   name_of_readline_prog_SOURCES = blah blah blah
dnl   other_programs_LDADD = blah
dnl   name_of_readline_prog_LDADD = blah $(READLINE_LIBS)
dnl   -----------------------------------------------
dnl
dnl

AH_TEMPLATE([HAS_READLINE],
	[ Defined if found readline ])

AC_DEFUN([AC_CHECK_READLINE],[
	search_readline=true
	has_readline=false

dnl	CFLAGS=${CFLAGS--O}

	AC_SUBST(READLINE_LIBS)
	AC_SUBST(READLINE_INCLUDES)

	AC_ARG_WITH(readline,
	  [  --with-readline[=dir]     Compile with readline/locate base dir [no compile]],
	  if test "x$withval" = "xno" ; then
		search_readline=false
	  elif test "x$withval" != "xyes" ; then
		READLINE_LIBS="$LIBS -L$withval/lib -lreadline"
		READLINE_INCLUDES="-I$withval/include"
		search_readline=false
		AC_DEFINE(HAS_READLINE)
		has_readline=true
	  else
	  	search_readline=true
	  fi
	)

	if $search_readline
	then
		AC_SEARCH_READLINE()
	fi

	if $has_readline
	then
		AC_DEFINE(HAS_READLINE)
		AC_READLINE_VERSION()
	fi



])
	
dnl
dnl Parameters: directory filename cureses_LIBS curses_INCLUDES nicename
dnl
AC_DEFUN([AC_READLINE], [
    if $search_readline
    then
        if test -f $1/$2
	then
	    AC_MSG_RESULT(Found readline on $1/$2)
 	    READLINE_LIBS="$3"
	    READLINE_INCLUDES="$4"
	    search_readline=false
            has_readline=true
	fi
    fi
])

AC_DEFUN([AC_SEARCH_READLINE], [
    AC_CHECKING(location of readline.h file)

    AC_READLINE(/usr/include, readline.h, -lreadline,, "readline on /usr/include")
    AC_READLINE(/usr/include/readline, readline.h, -lreadline, -I/usr/include/readline, "readline on /usr/include/readline")
    AC_READLINE(/usr/local/include, readline.h, -L/usr/local/lib -lreadline, -I/usr/local/include, "readline on /usr/local")
    AC_READLINE(/usr/local/include/readline, readline.h, -L/usr/local/lib -L/usr/local/lib/readline -lreadline, -I/usr/local/include/readline, "readline on /usr/local/include/readline")
] ) 

AC_DEFUN([AC_READLINE_VERSION], [
	AC_MSG_CHECKING(for readline version)
	readline_version=unknown
cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
#include <readline.h>
#undef VERSION
VERSION:RL_VERSION_MAJOR.RL_VERSION_MINOR
EOF
	if (eval "$ac_cpp $READLINE_INCLUDES conftest.$ac_ext") 2>&AC_FD_CC |
	  egrep "VERSION:" >conftest.out 2>&1; then
changequote(,)dnl
		readline_version=`cat conftest.out|sed -e 's/ //g' -e 's/^VERSION://' -e 's/\..*$//'`
changequote([,])dnl
	fi
	rm -rf conftest*
	AC_MSG_RESULT($readline_version)
] )
