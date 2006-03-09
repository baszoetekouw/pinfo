# no FSF/GNU fascism
AUTOMAKE_OPTIONS=foreign

ALL_LINGUAS =
always_built_SUBDIRS = src

sometimes_built_SUBDIRS = 

SUBDIRS = doc macros po $(always_built_SUBDIRS)

EXTRA_DIST =  TECHSTUFF pinfo.spec INSTALL


## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

## make rpms
rpm: Makefile
	$(MAKE) dist
	rpm -ta --clean $(PACKAGE)-$(VERSION).tar.gz
		

ACLOCAL_AMFLAGS = -I macros
