if TESTS
  SUBDIRS = unit_tests
  DIST_SUBDIRS = unit_tests
endif

MAINTAINERCLEANFILES = Makefile.in

install-data-local:
	install -m 644 ./tests.xml $(DESTDIR)/usr/share/sp-rich-core-tests
	install -m 755 ./test_script $(DESTDIR)/usr/share/sp-rich-core-tests
	install -m 755 ./test_whiteblacklist $(DESTDIR)/usr/share/sp-rich-core-tests
	install -m 755 ./test_extra_files $(DESTDIR)/usr/share/sp-rich-core-tests
	install -m 755 ./test_extract $(DESTDIR)/usr/share/sp-rich-core-tests
