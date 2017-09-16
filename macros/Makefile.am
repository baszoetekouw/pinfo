## Please update this variable if any new macros are created
MACROS=	\
	compiler-flags.m4		\
	aclocal-include.m4		\
	curses.m4			\
	readline.m4	\
	wchar.m4 \
	ax_check_compile_flag.m4

EXTRA_DIST=$(MACROS)
MAINTAINERCLEANFILES=macros.dep

@MAINT@macros.dep: Makefile.am
@MAINT@	@echo '$$(top_srcdir)/aclocal.m4: $(MACROS:%=macros/%)' > $@
