all:
	cd barcode-dbus-service && debuild; cd ..
	cd barcode-reader-glib && debuild; cd .. 

clean:
	/bin/rm -f *.changes *.build *.dsc *.deb *.tar.gz
