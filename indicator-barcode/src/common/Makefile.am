noinst_LTLIBRARIES = libmarshal.la liblog.la

libmarshal_la_SOURCES = \
	marshal-main.c

libmarshal_la_LIBADD = \
	$(GLIB_LIBS)

libmarshal_la_CPPFLAGS = \
	$(GLIB_CFLAGS)

marshal.h: marshal.list
	glib-genmarshal --header --prefix=_marshal $< > $@

marshal.c: marshal.list
	glib-genmarshal --body --prefix=_marshal $< > $@

liblog_la_SOURCES = \
	log.c \
	log.h

liblog_la_LIBADD = \
	$(GLIB_LIBS)

liblog_la_CPPFLAGS = \
	$(GLIB_CFLAGS)

DBUS_INTERFACES = \
	barman-agent.xml \
	barman-manager.xml \
	barman-technology.xml \
	barman-service.xml \
	indicator-barcode-agent.xml \
	indicator-barcode-service.xml

DBUS_XML_FILES = $(DBUS_INTERFACES:.xml=-xml.h)

all-local: \
	$(DBUS_XML_FILES)

%.check-xml : %.xml
	@xmllint -valid -noout $<
	@echo $< OK

# ugly trick to get a check target for each xml file
checkxml: $(DBUS_INTERFACES:.xml=.check-xml)

# create a .h file containing the introspection data in a variable for gdbus
%-xml.h: %.xml
	name=`echo $(<F:.xml=_xml) | sed 's/-/_/g'`; \
	echo "static const gchar $$name[] = " > $@; \
	grep -v DOCTYPE $< | tr \" \' | sed 's/^/\"/g' | sed 's/$$/\"/g' >> $@; \
	echo ";" >> $@;

BUILT_SOURCES = \
	marshal.c \
	marshal.h \
	$(DBUS_XML_FILES)

CLEANFILES = \
	$(BUILT_SOURCES)

EXTRA_DIST = \
	marshal.list \
	dbus-shared-names.h \
	$(DBUS_INTERFACES)
