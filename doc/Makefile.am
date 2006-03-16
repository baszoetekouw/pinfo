## Process this file with automake to produce Makefile.in

@SET_MAKE@

man_MANS = pinfo.1
info_TEXINFOS = pinfo.texi
EXTRA_DIST = $(man_MANS) $(info_TEXINFOS)

clean-generic: my-clean
my-clean:
	rm -f pinfo.1 pinfo.info
