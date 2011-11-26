
if HAVE_SYSTEMD
systemdsystemunit_DATA = BarcodeManager.service

BarcodeManager.service: BarcodeManager.service.in
	$(edit) $< >$@

servicedir = $(datadir)/dbus-1/system-services
service_in_files = org.freedesktop.BarcodeManager.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(edit) $< >$@
endif

edit = sed \
	-e 's|@sbindir[@]|$(sbindir)|g' \
	-e 's|@sysconfdir[@]|$(sysconfdir)|g' \
	-e 's|@localstatedir[@]|$(localstatedir)|g'

EXTRA_DIST = \
	BarcodeManager.service.in \
	org.freedesktop.BarcodeManager.service.in

CLEANFILES = BarcodeManager.service org.freedesktop.BarcodeManager.service

