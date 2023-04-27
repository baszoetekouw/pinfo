dnl Detection of wchar_t and friends
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
dnl  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
dnl  USA


dnl  To use thisscript , put a call to AC_CHECK_WCHAR in your configure.ac
dnl  After the call, USE_WCHAR will be defined and $USE_WCHAR=true if 
dnl  wide characters are supported.
dnl  Additional flags that are needed are defined in
dnl  $WCHAR_FLAGS and @WCHAR_FLAGS@

AH_TEMPLATE([USE_WCHAR],
	[Defined if support for wide chars is wanted and supported.
	wchar.h, wchar_t, mbstowcs(), and friends will be available.]
)
AH_TEMPLATE([HAVE_WCSWIDTH],
	[Defined if wcswidth() is available in <wchar.h>]
)

AC_DEFUN([AC_CHECK_WCHAR],[

	wchar_flags=

	dnl  first check for wchar_t
	AC_MSG_CHECKING([for wchar_t in wchar.h])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([ 
			[ #include <wchar.h> ], 
			[ wchar_t foo =	(wchar_t)'\0'; ] 
		])],
		[
			AC_MSG_RESULT([yes])
			wchar_usable=true
		],
		[
			AC_MSG_RESULT([no])
			wchar_usable=false
		]
	)

	if test "x$wchar_usable" = "xtrue"
	then
		dnl  then check for mbstowcs
		AC_CHECK_DECL([mbstowcs],
			[ have_mbstowcs=true  ],
			[ have_mbstowcs=false ]
		)
	
		dnl  then check for wcwidth
		have_wcwidth=false
		AC_MSG_CHECKING([for wcwidth])
		AC_COMPILE_IFELSE([
			AC_LANG_PROGRAM( 
				[ #include <wchar.h> ], 
				[ char *p = (char *) wcwidth; ] 
			)],
			[
				dnl if found, set variables and print result
				have_wcwidth=true
				AC_MSG_RESULT([yes])
			],
			[ ]
		)
		if test "x$have_wcwidth" = "xfalse"
		then
			AC_COMPILE_IFELSE([
				AC_LANG_PROGRAM( 
					[[
						#define _DEFAULT_SOURCE
						#define _XOPEN_SOURCE 600
						#include <wchar.h> 
					]],
					[[ char *p = (char *) wcwidth; ]]
				)],
				[
					dnl if found, set variables and print result
					have_wcwidth=true
					wchar_flags="$wchar_flags -D_DEFAULT_SOURCE -D_XOPEN_SOURCE=600"
					AC_MSG_RESULT([with -D_XOPEN_SOURCE=600])
				],
				[ ]
			)
		fi

	fi


	if test \( "x$wchar_usable"  = "xtrue" \) \
		 -a \( "x$have_mbstowcs" = "xtrue" \) \
		 -a \( "x$have_wcwidth" = "xtrue" \)
	then
		USE_WCHAR=true
		AC_DEFINE(USE_WCHAR)
		WCHAR_FLAGS=$wchar_flags
		AC_SUBST(WCHAR_FLAGS)
	else
		USE_WCHAR=false
	fi

	if test "x$USE_WCHAR" = "xtrue"
	then
		dnl  check for wcswidth
		CPPFLAGS_OLD=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS $wchar_flags"
		AC_CHECK_DECL([wcswidth],
			[ AC_DEFINE(HAVE_WCSWIDTH)  ],
			[ true ],
			[ #include <wchar.h> ]
		)
		CPPFLAGS=$CPPFLAGS_OLD
	fi

])

dnl vim:ts=4:sw=4
