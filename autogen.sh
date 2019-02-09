#!/bin/sh
set -e

# Refresh GNU autotools toolchain.
echo Cleaning autotools files...
for f in autom4te.cache configure \
         Makefile.in src/Makefile.in po/Makefile.in doc/Makefile.in macros/Makefile.in \
         tools/mkinstalldirs tools/ltmain.sh tools/missing tools/config.guess tools/depcomp \
         tools/config.sub tools/install-sh
do
	test -e "$f" && rm -rf "$f" || true
done

echo Running autoreconf...
autoreconf --install --symlink --verbose
if [ -d ".git" ]
then
	git log > Changelog
fi

exit 0
