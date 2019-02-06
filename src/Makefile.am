## Process this file with automake to produce Makefile.in

@SET_MAKE@

bin_PROGRAMS = pinfo

pinfo_COMMON_SRC = pinfo.c \
		colors.c \
		datatypes.c \
		filehandling_functions.c \
		mainfunction.c \
		manual.c \
		menu_and_note_utils.c \
		parse_config.c \
		regexp_search.c \
		signal_handler.c \
		utils.c \
		video.c \
		colors.h \
		common_includes.h \
		datatypes.h \
		filehandling_functions.h \
		keyboard.h \
		mainfunction.h \
		manual.h \
		menu_and_note_utils.h \
		parse_config.h \
		regexp_search.h \
		signal_handler.h \
		utils.h \
		video.h \
		initializelinks.c \
		initializelinks.h \
		printinfo.c \
		printinfo.h \
		localestuff.h

pinfo_READLINE = readlinewrapper.c readlinewrapper.h
pinfo_SNPRINTF = snprintf.c # snprintf.h
pinfo_SIGBLOCK = sigblock.c sigblock.h

if HAS_READLINE
pinfo_READLINE_OBJ =
else
pinfo_READLINE_OBJ = readlinewrapper.o
endif

if HAVE_SNPRINTF
pinfo_SNPRINTF_OBJ =
else
pinfo_SNPRINTF_OBJ = snprintf.o
endif

if HAVE_SIGBLOCK
pinfo_SIGBLOCK_OBJ =
else
pinfo_SIGBLOCK_OBJ = sigblock.o
endif

pinfo_SOURCES =	$(pinfo_COMMON_SRC)

pinfo_LDADD = \
	$(pinfo_READLINE_OBJ) $(pinfo_SNPRINTF_OBJ) \
	$(pinfo_SIGBLOCK_OBJ) $(READLINE_LIBS) $(INTLLIBS) \
	$(CURSES_LIBS)

pinfo_CFLAGS = $(CURSES_CFLAGS) $(WCHAR_FLAGS) \
	$(READLINE_INCLUDES) $(WFLAGS) \
	-DLOCALEDIR=\"$(LOCALEDIR)\" -DCONFIGDIR=\"$(sysconfdir)/pinforc\"

pinfo_DEPENDENCIES = $(pinfo_READLINE_OBJ)\
                     $(pinfo_SNPRINTF_OBJ)\
                     $(pinfo_SIGBLOCK_OBJ)\
                     $(INTLDEPS)


if HAS_READLINE
else
readlinewrapper.o: $(pinfo_READLINE)
endif

if HAVE_SNPRINTF
else
snprintf.o: $(pinfo_SNPRINTF)
endif

if HAVE_SIGBLOCK
else
sigblock.o: $(pinfo_SIGBLOCK)
endif

install-data-local:
	if [ ! -f $(DESTDIR)$(sysconfdir)/pinforc ] ; then \
		if [ ! -d $(DESTDIR)$(sysconfdir) ]; then \
		   $(INSTALL) -d $(DESTDIR)$(sysconfdir); \
		fi; \
		$(INSTALL) -m 644 pinforc $(DESTDIR)$(sysconfdir) ; \
	else \
		echo "WARNING! Old pinforc detected. I'm not installing the new file"; \
	fi
uninstall-local:
	rm -f $(DESTDIR)$(sysconfdir)/pinforc



EXTRA_DIST = $(pinfo_READLINE) $(pinfo_SNPRINTF) $(pinfo_SIGBLOCK)

