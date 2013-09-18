core_reducer_LDFLAGS = \
	$(ELF_LIBS)	\
	$(COVERAGE_LIBS)\
	$(NULL)

core_reducer_CFLAGS = \
	-I$(top_srcdir)/core-reducer \
	$(COVERAGE_FLAGS)\
	$(NULL)

noinst_HEADERS = \
	$(top_srcdir)/core-reducer/defines.h \
	$(top_srcdir)/core-reducer/elfbinaryreader.h \
	$(top_srcdir)/core-reducer/elfcorereader.h \
	$(top_srcdir)/core-reducer/procinterface.h \
	$(top_srcdir)/core-reducer/rawelfwriter.h \
	$(top_srcdir)/core-reducer/reducer.h \
	$(NULL)

core_reducer_SOURCES = \
	main.cpp \
	elfbinaryreader.cpp \
	elfcorereader.cpp \
	procinterface.cpp \
	rawelfwriter.cpp \
	reducer.cpp \
	$(NULL)

bin_PROGRAMS = core-reducer

core_reducer_CXXFLAGS = $(core_reducer_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in


default-local: core-reducer

clean-local:
	rm -rf $(bin_PROGRAMS) *.o core.* *.gcda *.gcno *.info *.xml *.out

distclean-local: clean-local
