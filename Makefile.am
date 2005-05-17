
ALL_LINGUAS =
always_built_SUBDIRS = src

sometimes_built_SUBDIRS = 

SUBDIRS = doc macros po intl $(always_built_SUBDIRS)

EXTRA_DIST = TECHSTUFF TODO pinfo.spec INSTALL


## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

## make rpms
rpm: Makefile
	$(MAKE) dist
	rpm -ta --clean $(PACKAGE)-$(VERSION).tar.gz
		
