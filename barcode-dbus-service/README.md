# barcode-utils dbus service

```barcode-dbus-service``` connects the system's barcode readers to the DBus and exports them as the me.koppi.BarcodeReader service.

## udev rules for barcode readers

The file [debian/40-barcode-dbus-service.rules](barcode-utils/blob/master/barcode-dbus-service/debian/barcode-dbus-service.rules) sets the group id to "plugdev" and device permissions to "0644". Reloading the udev rules is triggered by:

```
$ sudo service restart udev
$ sudo udevadm trigger
```

on Ubuntu 10.04 +.
