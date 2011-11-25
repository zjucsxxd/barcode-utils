
dbus_servicesdir = $(DBUSSERVICEDIR)
service_in_files =  indicator-barcode.service.in \
	indicator-barcode-agent.service.in 
dbus_services_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

#$(dbus_services_DATA): $(service_in_files) Makefile
#	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =					\
	$(service_in_files) \
	ok-banner

CLEANFILES =					\
	$(dbus_services_DATA)
