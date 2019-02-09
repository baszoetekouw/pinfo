# no FSF/GNU fascism
AUTOMAKE_OPTIONS=foreign

always_built_SUBDIRS = src

sometimes_built_SUBDIRS =

SUBDIRS = doc macros po $(always_built_SUBDIRS)

EXTRA_DIST =  TECHSTUFF pinfo.spec Changelog.old Changelog

## make rpms
rpm: Makefile
	$(MAKE) dist
	rpm -ta --clean $(PACKAGE)-$(VERSION).tar.gz


ACLOCAL_AMFLAGS = -I macros
