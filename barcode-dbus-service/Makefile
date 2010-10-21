CFLAGS=`pkg-config --cflags dbus-glib-1`
LDFLAGS=-g -ludev `pkg-config --libs dbus-glib-1`

all: barcode-dbus-service barcode-reader

clean:
	@/bin/rm -f *~ \
		    barcode-dbus-service barcode-dbus-service.1 \
		    barcode-reader barcode-reader.1 \
		    build-stamp configure-stamp

INSTALL=install

BINDIR=$(DESTDIR)/usr/bin
ETCDIR=$(DESTDIR)/etc/barcode-utils
MANDIR=$(DESTDIR)/usr/share/man
SERVICEDIR=$(DESTDIR)/usr/share/dbus-1/services

all:

man:
	pod2man barcode-dbus-service.pod > barcode-dbus-service.1
	pod2man barcode-reader.pod > barcode-reader.1

test:

install: man
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 755 barcode-dbus-service.1 $(MANDIR)/man1
	$(INSTALL) -m 755 barcode-reader.1 $(MANDIR)/man1
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -m 755 barcode-reader $(BINDIR)
	$(INSTALL) -m 755 barcode-dbus-service $(BINDIR)
	$(INSTALL) -d -m 755 $(SERVICEDIR)
	$(INSTALL) -m 622 me.koppi.BarcodeReader.service $(SERVICEDIR)

