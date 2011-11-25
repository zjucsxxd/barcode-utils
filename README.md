# barcode-utils

* [barcode-reader-glib](barcode-utils/blob/master/barcode-reader-glib)	cli tool to connect to the DBus me.koppi.BarcodeReader service and "print read 'barcode'\n" to STDOUT.
* [barcode-dbus-service](barcode-utils/blob/master/barcode-dbus-service)	connects the barcode readers to the DBus and exports the me.koppi.BarcodeReader service.

## Tested Barcode Scanners

 *  LS3408-FZ20005 - Symbol LS 3408FZ, LS 3408 Series Scanner

    ![LS3408-FZ20005](https://raw.github.com/koppi/barcode-utils/master/barcode\
-scanners/LS3408-FZ20005.png "Symbol LS 3408FZ, LS 3408 Series Scanner")

    The LS 3408FZ, LS 3408 Series uses fuzzy logic technology to interpret barc\
odes that have been damaged or soiled to give you the kind of versatility that'\
s necessary for today's industrial applications. The rugged housing, sealed to \
IP65 standards, withstands repeated drops of up to 6.5 feet (2 meters) to concr\
ete, ensuring limited downtime. The rugged LS 3408FZ, LS 3408 Series FZ handhel\
d scanner from Symbol features fuzzy logic technology for fast and accurate rea\
ding of damaged, dirty and poorly printed one-dimensional (1D) barcodes typical\
ly found in industrial environments.

    ```
$ lsusb
Bus 004 Device 003: ID 05e0:1300 Symbol Technologies
    ``` 

## Authors

 * Jakob Flierl https://github.com/koppi
