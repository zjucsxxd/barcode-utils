SUBDIRS = \
	barman-mock \
	libbarman-vala

GCOV_FLAGS = -fprofile-arcs -ftest-coverage

GCOV_FILES = \
	service \
	service-manager \
	network-menu \
	manager \
	manager-proxy

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/common \
	-I$(top_srcdir)/src/backend \
	-I$(top_builddir)/src/common

AM_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GOBJECT_CFLAGS) \
	$(DBUSGLIB_CFLAGS) \
	$(DBUSMENUGLIB_CFLAGS) \
	$(LIBNOTIFY_CFLAGS) \
	$(BARMAN_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-DBINDIR=\"$(bindir)\" \
	$(GCOV_FLAGS)

AM_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GOBJECT_LIBS) \
	$(top_builddir)/src/common/libmarshal.la \
	$(GCOV_FLAGS)

TEST_PROGS =
noinst_PROGRAMS = \
	$(TEST_PROGS) \
	libbarman-tool

TEST_PROGS += test-service-manager
test_service_manager_SOURCES = \
	test-service-manager.c \
	$(top_srcdir)/src/backend/service-manager.c \
	mock-manager.c \
	mock-service.c \
	mock-barman-manager.c \
	mock-barman-manager.h \
	mock-barman-service.c

TEST_PROGS += test-service
test_service_SOURCES = \
	test-service.c \
	$(top_srcdir)/src/backend/service.c \
	mock-manager.c \
	mock-service-manager.c \
	mock-dbusmenu-menuitem.c \
	mock-barman-manager.c \
	mock-barman-manager.h \
	mock-barman-service.c

TEST_PROGS += test-network-menu
test_network_menu_SOURCES = \
	test-network-menu.c \
	$(top_srcdir)/src/backend/network-menu.c \
	mock-manager.c \
	mock-dbusmenu-menuitem.c \
	mock-service-manager.c \
	mock-dbusmenu-server.c \
	mock-ui-proxy.c \
	mock-barman-manager.c \
	mock-barman-manager.h \
	mock-barman-service.c

TEST_PROGS += test-manager
test_manager_SOURCES = \
	test-manager.c \
	$(top_srcdir)/src/backend/manager.c \
	mock-service-manager.c \
	mock-service.c \
	mock-network-menu.c \
	mock-network-menu.h \
	mock-manager-proxy.c \
	mock-log.c \
	mock-notify-notification.c \
	mock-dbusmenu-menuitem.c \
	mock-ofono-manager.c \
	mock-ui-proxy.c \
	mock-barman-manager.c \
	mock-barman-manager.h \
	mock-barman-service.c

libbarman_tool_SOURCES = libbarman-tool.c

libbarman_tool_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(BARMAN_CFLAGS)

libbarman_tool_LDADD = \
	$(GLIB_LIBS) \
	$(BARMAN_LIBS)

#run make test as part of make check
check-local: test
test:
	@for prog in $(TEST_PROGS); do \
		gtester --verbose -k -o=$$prog-results.xml ./$$prog; \
	done

# FIXME: check-report and full-report should use TEST_PROGS
.PHONY: check-report full-report
check-report:
	@gtester -k -o=test-service-results.xml -k ./test-service-manager \
		&& ( gtester-report test-service-results.xml \
				| sed 's/GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
				> test-service-results.html ) \
		&& gnome-open ./test-service-results.html

full-report:
		@gtester -k -o=test-service-results.xml -k -m=slow ./test-service \
	  && ( gtester-report test-service-results.xml \
	      | sed 's/>GTester Unit Test Report</>GTester Unit Test Report (normal)</' \
	      > test-service-results.html )

gcov:
	@for file in $(GCOV_FILES); do \
		echo "  GCOV $$file"; \
		gcov $$file > /dev/null; \
	done;

clean-generic:
	rm -f *-results.xml *-results.html *.gcda *.gcno *.gcov
