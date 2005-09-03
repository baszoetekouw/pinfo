## Process this file with automake to produce Makefile.in

@SET_MAKE@

bin_PROGRAMS = pinfo


pinfo_COMMON_SRC = pinfo.cxx \
		colors.cxx \
		datatypes.cxx \
		filehandling_functions.cxx \
		mainfunction.cxx \
		manual.cxx \
		parse_config.cxx \
		regexp_search.cxx \
		signal_handler.cxx \
		utils.cxx \
		video.cxx \
		colors.h \
		common_includes.h \
		datatypes.h \
		filehandling_functions.h \
		keyboard.h \
		mainfunction.h \
		manual.h \
		parse_config.h \
		regexp_search.h \
		signal_handler.h \
		utils.h \
		video.h \
		initializelinks.cxx \
		initializelinks.h \
		printinfo.cxx \
		printinfo.h \
		tmpfiles.cxx \
		tmpfiles.h \
		localestuff.h \
		rcsid.h

pinfo_SNPRINTF = snprintf.cxx # snprintf.h
pinfo_SIGBLOCK = sigblock.cxx sigblock.h

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

INCLUDES = @READLINE_INCLUDES@

pinfo_LDADD = $(pinfo_SNPRINTF_OBJ)\
              $(pinfo_SIGBLOCK_OBJ) @READLINE_LIBS@ $(INTLLIBS)

pinfo_DEPENDENCIES = $(pinfo_SNPRINTF_OBJ)\
                     $(pinfo_SIGBLOCK_OBJ)\
		     $(INTLDEPS)

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
		$(INSTALL) -m 644 $(srcdir)/pinforc $(DESTDIR)$(sysconfdir) ; \
	else \
		echo "WARNING! Old pinforc detected. I'm not installing the new file"; \
        fi

EXTRA_DIST = $(pinfo_SNPRINTF) $(pinfo_SIGBLOCK)

