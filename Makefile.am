EXTRA_DIST = autogen.sh

INCLUDES = $(DEPS_CFLAGS)

SUBDIRS = rich-core-extract core-reducer scripts tests
DIST_SUBDIRS = $(SUBDIRS)

MAINTAINERCLEANFILES = Makefile.in

distclean-local:
	rm -Rf autom4te.cache compile config.guess config.sub aclocal.m4
	rm -f config.h.in configure
	rm -f depcomp install-sh ltmain.sh missing
