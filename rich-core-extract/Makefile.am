rich_core_extract_LDFLAGS = \
	$(COVERAGE_LIBS)\
	$(NULL)

rich_core_extract_CFLAGS = \
	-I$(top_srcdir)/src \
	$(COVERAGE_FLAGS)\
	$(NULL)

rich_core_extract_SOURCES = \
	rich-core-extract.c \
	$(NULL)

bin_PROGRAMS = rich-core-extract

MAINTAINERCLEANFILES = Makefile.in

default-local: rich-core-extract

clean-local:
	rm -rf $(bin_PROGRAMS) *.o *.gcno *.gcda

distclean-local: clean-local
