SUBDIRS = \
	icons \
	ui

bin_PROGRAMS = indicator-barcode-settings

indicator_barcode_settings_SOURCES = \
	main.vala \
	app.vala \
	enums.vala \
	logger.vala \
	backend/device.vala \
	frontend/utils.vala \
	frontend/pages/connections.vala \
	frontend/widgets/cellrenderers.vala \
	frontend/widgets/stores.vala \
	frontend/widgets/treeviews.vala \
	frontend/widgets/infobox.vala \
	frontend/widgets/toggleswitch.vala \
	frontend/widgets/device-boxes/base.vala \
	frontend/widgets/device-boxes/serial.vala \
	frontend/widgets/device-boxes/bluetooth.vala \
	frontend/widgets/device-boxes/wired.vala \
	frontend/widgets/device-boxes/wireless.vala \
	frontend/widgets/device-boxes/usb.vala \
	frontend/widgets/dialogs/edit-connection.vala

VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--vapidir=$(srcdir)/../../src/libbarman \
        --pkg=gmodule-2.0 \
        --pkg=gtk+-2.0 \
        --pkg=gdk-2.0 \
        --pkg=gio-2.0 \
        --pkg=gee-1.0 \
        --pkg=pango \
        --pkg=cairo \
	--pkg=config \
	--pkg=Barman-1.0 \
	--fatal-warnings

# -w is needed to disable gcc warnings because of valac
indicator_barcode_settings_CFLAGS = \
	$(GTK_CFLAGS) \
	$(LIBGEE_CFLAGS) \
	$(BARMAN_CFLAGS) \
	-w \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DPKGDATADIR=\""$(pkgdatadir)"\"

indicator_barcode_settings_LDADD = \
	$(GTK_LIBS) \
	$(LIBGEE_LIBS) \
	$(BARMAN_LIBS)
