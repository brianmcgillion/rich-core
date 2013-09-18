
main_test_LDFLAGS = \
	$(ELF_LIBS) \
	$(COVERAGE_LIBS)\
	$(CPPUNIT_LIBS) \
	-ldl \
	-lpthread \
	$(NULL)

main_test_CFLAGS = \
	-I$(top_srcdir)/core-reducer \
	$(CPPUNIT_FLAGS) \
	$(COVERAGE_FLAGS)\
	$(NULL)

main_test_SOURCES = \
	main_test.cpp \
	test_elfbinaryreader.cpp \
	test_elfcorereader.cpp \
	$(top_srcdir)/core-reducer/elfbinaryreader.cpp \
	$(top_srcdir)/core-reducer/elfcorereader.cpp \
	signalcatcher.cpp \
	$(NULL)

main_test_CXXFLAGS = $(main_test_CFLAGS)

MAINTAINERCLEANFILES = Makefile.in

check_PROGRAMS = main_test

output:
	lcov -d . -c -o core-reducer-tests.info
	genhtml -t "Core-Reducer Unit Test Coverage" -o coverage core-reducer-tests.info

check-local: main_test
	(./main_test; [ $? != 0 ] || echo "One or More Tests failed")
	make output

clean-local:
	rm -rf $(check_PROGRAMS) *.o *.txt *.gcno
	rm -rf *.gcda *.xml *.gcov *.info *.out coverage

distclean-local: clean-local
