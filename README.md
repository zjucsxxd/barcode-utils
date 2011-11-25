# barcode-utils

* [barcode-reader-glib](barcode-utils/blob/master/barcode-reader-glib): command line tool to connect to the DBus me.koppi.BarcodeReader service and "print read 'barcode'\n" to STDOUT.
* [barcode-dbus-service](barcode-utils/blob/master/barcode-dbus-service): DBus service, which connects the system's barcode readers to the DBus and exports the me.koppi.BarcodeReader service.
* [indicator-barcode](barcode-utils/blob/master/indicator-barcode): A barcode reader utility showing in the indicator bar.

## Tested Barcode Scanners

 *  LS3408-FZ20005 - Symbol LS 3408FZ, LS 3408 Series Scanner

    ![LS3408-FZ20005](https://raw.github.com/koppi/barcode-utils/master/barcode\-scanners/LS3408-FZ20005.png "Symbol LS 3408FZ, LS 3408 Series Scanner")

    The LS 3408FZ, LS 3408 Series uses fuzzy logic technology to interpret barcodes that have been damaged or soiled to give you the kind of versatility that's necessary for today's industrial applications. The rugged housing, sealed to IP65 standards, withstands repeated drops of up to 6.5 feet (2 meters) to concrete, ensuring limited downtime. The rugged LS 3408FZ, LS 3408 Series FZ handheld scanner from Symbol features fuzzy logic technology for fast and accurate reading of damaged, dirty and poorly printed one-dimensional (1D) barcodes typically found in industrial environments.

    ```
$ lsusb
Bus 004 Device 003: ID 05e0:1300 Symbol Technologies

## Get the source code

```
 $ wget -O barcode-utils.zip https://github.com/koppi/barcode-utils/zipball/master
 $ unzip barcode-utils.zip
 $ cd barcode-utils*
```

or

```
 $ git clone https://github.com/koppi/barcode-utils.git
 $ cd barcode-utils/
```

## Compile / Install

The following packages are required to build barcode-utils:

```
 $ sudo apt-get install debhelper libudev-dev libdbus-1-dev
```

Compile and install barcode-utils as follows:

```
 $ make install
```

or

```
 $ make build
 $ sudo dpkg -i barcode-dbus-service*.deb barcode-reader-glib*.deb
```

## Authors

 * Jakob Flierl https://github.com/koppi
