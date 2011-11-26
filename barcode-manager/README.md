# barcode-manager

## Barcode Scanning that just works

The point of BarcodeManager is to make barcode scanner configuration and setup as painless and automatic as possible.

## How it works

The BarcodeManager daemon runs as a privileged service (since it must access and control hardware), but provides a D-Bus interface on the system bus to allow for fine-grained control of barcode scanner setup. BarcodeManager does not store settings, it is only the mechanism by which those barcode scanner are selected and activated.

A variety of other system services are used by BarcodeManager to provide barcode scanning functionality.


## Why doesn't my Barcode Scanner Hardware Just Work?

Driver problems are the #1 cause of why BarcodeManager sometimes fails to connect to barcode scanner hardwares. Often, the driver simply doesn't behave in a consistent manner, or is just plain buggy. BarcodeManager supports _only_ those drivers that are shipped with the upstream Linux kernel, because only those drivers can be easily fixed and debugged.

Sometimes, it really is BarcodeManager's fault. If you think that's the case, please file a bug at https://github.com/koppi/barcode-utils and choose the barcode-manager component. Attaching the output of /var/log/messages or /var/log/daemon.log (wherever your distribution directs syslog's 'daemon' facility output) is often very helpful.

