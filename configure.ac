AC_INIT(configure.ac)
AM_CONFIG_HEADER(config.h)


VERSION=0.1

if test $CUSTOM_VERSION; then
VERSION=$CUSTOM_VERSION
fi
AM_INIT_AUTOMAKE(sp-rich-core, $VERSION)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_TOOL(AR, ar)
AC_HEADER_STDC
AC_PROG_CPP
AC_PROG_CXX
AC_HEADER_STDC

AC_CHECK_HEADERS([libelf.h])
ELF_LIBS="-lelf"
AC_SUBST(ELF_LIBS)

AC_ARG_ENABLE(debug, [ --enable-debug=[yes|no] ], [use_debug=$enableval ])
if test "$use_debug" = "yes"; then
        AC_DEFINE(DEBUG, 1,"Set to 1 if enable-debug is yes")
fi

AC_ARG_ENABLE(logging, [ --enable-logging=[yes|no] ], [use_logging=$enableval ])
if test "$use_logging" = "yes"; then
        AC_DEFINE(LOGGING, 1,"Set to 1 if enable-logging is yes")
fi

AC_ARG_ENABLE(armregs, [ --enable-armregs=[yes|no] ], [use_armregs=$enableval ])
if test "$use_armregs" = "yes"; then
        AC_DEFINE(ARM_REGS, 1,"Set to 1 if arm processor is to be used")
fi

# The standard output files to create
AC_CONFIG_FILES([Makefile rich-core-extract/Makefile core-reducer/Makefile scripts/Makefile tests/Makefile])

#!!!!Put in package checks that to ensure that the libcppunit and lcov are
#!!!!Both in place before trying to use them.
AC_ARG_ENABLE(tests,  [ --enable-tests=[yes|no] ], [tests_enabled=$enableval ])
if test "$tests_enabled" = "yes"; then
	CPPUNIT_LIBS="-lcppunit"
	CPPUNIT_FLAGS="-fexceptions"
	AC_SUBST(CPPUNIT_LIBS)
	AC_SUBST(CPPUNIT_FLAGS)
	COVERAGE_LIBS="-fprofile-arcs -pg"
	COVERAGE_FLAGS="-fprofile-arcs -ftest-coverage -pg -rdynamic"
	AC_SUBST(COVERAGE_LIBS)
	AC_SUBST(COVERAGE_FLAGS)
	AC_CONFIG_FILES([tests/unit_tests/Makefile])
fi

AM_CONDITIONAL([TESTS], [test "${tests_enabled}" = "yes"])

# Define ouput files
AC_OUTPUT
