CFLAGS=`pkg-config --cflags dbus-glib-1`
LDFLAGS=-g -ludev `pkg-config --libs dbus-glib-1`

all: barcode-reader-glib

clean:
	@/bin/rm -f *~ configure-stamp build-stamp \
		    barcode-reader-glib barcode-reader-glib.1

INSTALL=install

BINDIR=$(DESTDIR)/usr/bin
ETCDIR=$(DESTDIR)/etc/barcode-utils
MANDIR=$(DESTDIR)/usr/share/man

all:

man:
	pod2man barcode-reader-glib.pod > barcode-reader-glib.1

test:

install: man
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 755 barcode-reader-glib.1 $(MANDIR)/man1
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) -m 755 barcode-reader-glib $(BINDIR)

