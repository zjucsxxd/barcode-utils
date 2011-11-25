#DBG_CFLAGS := -ggdb

pkg_packages := dbus-glib-1 libudev

PKG_CFLAGS  := $(shell pkg-config --cflags $(pkg_packages))
PKG_LDFLAGS := $(shell pkg-config --libs $(pkg_packages))

ADD_CFLAGS := -Wall

CFLAGS  := $(PKG_CFLAGS) $(ADD_CFLAGS) $(DBG_CFLAGS) $(CFLAGS)
LDFLAGS := $(PKG_LDFLAGS) $(LDFLAGS)

.PHONY: all clean

all: barcode-dbus-service man

clean:
	@/bin/rm -f *~ \
		    barcode-dbus-service barcode-dbus-service.1 \
		    build-stamp configure-stamp

INSTALL=install

BINDIR=$(DESTDIR)/usr/bin
ETCDIR=$(DESTDIR)/etc/barcode-utils
MANDIR=$(DESTDIR)/usr/share/man
SERVICEDIR=$(DESTDIR)/usr/share/dbus-1/services

man: barcode-dbus-service.1

barcode-dbus-service.1: barcode-dbus-service.pod
	pod2man barcode-dbus-service.pod > barcode-dbus-service.1

test:

install: all
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 644 barcode-dbus-service.1 $(MANDIR)/man1
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -m 755 barcode-dbus-service $(BINDIR)
	$(INSTALL) -d -m 755 $(SERVICEDIR)
	$(INSTALL) -m 644 me.koppi.BarcodeReader.service $(SERVICEDIR)

