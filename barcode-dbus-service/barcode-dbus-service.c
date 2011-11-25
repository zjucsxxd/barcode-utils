#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dbus/dbus.h>
#include <libudev.h>

const char * bus_str(int bus) {
 switch (bus) {
   case BUS_USB:
     return "USB";
     break;
   case BUS_HIL:
     return "HIL";
     break;
   case BUS_BLUETOOTH:
     return "Bluetooth";
     break;
   case BUS_VIRTUAL:
     return "Virtual";
     break;
   default:
     return "Other";
     break;
  }
}

static void dbus_send(DBusConnection *connection,
                      const char *action, const char *msg) {

  DBusMessage *message;
 
  message = dbus_message_new_signal("/me/koppi/BarcodeReader/read",
                                    "me.koppi.BarcodeReader", action);

  dbus_message_append_args(message, DBUS_TYPE_STRING, &msg, DBUS_TYPE_INVALID);

  dbus_connection_send (connection, message, NULL);
  dbus_connection_flush(connection);
  dbus_message_unref (message);
}

static int open_hid(struct udev_device *dev) {
  int fd;
  int i, res, desc_size = 0;

  char buf[256];
  char *devname;

  struct hidraw_report_descriptor rpt_desc;
  struct hidraw_devinfo info;
  struct udev_device *dev_parent;

  printf("%s, ", udev_device_get_devnode(dev));

  dev_parent = udev_device_get_parent_with_subsystem_devtype(dev,
                                           "usb", "usb_device");

  if (!dev_parent) {
    printf("  Unable to find parent usb device.");
    exit(1);
  }
	
  printf(" ID: %s:%s\n",
    udev_device_get_sysattr_value(dev_parent,"idVendor"),
    udev_device_get_sysattr_value(dev_parent, "idProduct"));
  printf("  %s\n", udev_device_get_sysattr_value(dev_parent,"product"));

  const char* vId = udev_device_get_sysattr_value(dev_parent, "idVendor");

  // FIXME add more flexible filter mechanism here.
  if (strcmp(vId, "05e0") == 0) {
    fd = open(udev_device_get_devnode(dev), O_RDWR|O_NONBLOCK);
 
    if (fd < 0) {
      perror("Unable to open device.");

      return 0;
    }

    /*
    memset(&rpt_desc, 0x0, sizeof(rpt_desc));
    memset(&info, 0x0, sizeof(info));
    memset(buf, 0x0, sizeof(buf));

    res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
 
    if (res < 0)
      perror("  HIDIOCGRDESCSIZE");
    else
      printf("  Report Descriptor Size: %d\n", desc_size);

    rpt_desc.size = desc_size;
    res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);

    if (res < 0) {
      perror("  HIDIOCGRDESC");
    } else {
      printf("  Report Descriptor:\n");
      for (i = 0; i < rpt_desc.size; i++)
        printf("%hhx ", rpt_desc.value[i]);
      puts("\n");
    }

    res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
    if (res < 0)
      perror("  HIDIOCGRAWNAME");
    else
      printf("  Raw Name: %s\n", buf);

    res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
    if (res < 0)
      perror("  HIDIOCGRAWPHYS");
    else
      printf("  Raw Phys: %s\n", buf);

    res = ioctl(fd, HIDIOCGRAWINFO, &info);
    if (res < 0) {
      perror("  HIDIOCGRAWINFO");
    } else {
      printf("  Raw Info:\n");
      printf("    bustype: %d (%s)\n", info.bustype, bus_str(info.bustype));
      printf("    vendor: 0x%04hx\n", info.vendor);
      printf("    product: 0x%04hx\n", info.product);
      } */

    printf("%s opened.\n", udev_device_get_devnode(dev));

    return fd;
  } else {
    return -1;
  }
}

int main (void) {
  char buf[256];

  struct udev *udev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;
  struct udev_device *dev;

  struct udev_monitor *mon;
  int fd, fd_hid;
  int res = 0;

  fd_set master;
  fd_set read_fds;
  int fdmax = 0;

  DBusConnection *connection;
  DBusError error;

  char *name = "me.koppi.BarcodeReader";

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  udev = udev_new();

  if (!udev) {
    perror("Can't create udev.\n");

    return 1;
  }

  dbus_error_init (&error);
  connection = dbus_bus_get(DBUS_BUS_STARTER, &error);

  if (!connection) {
    printf ("Failed to connect to the D-BUS daemon: %s", error.message);
    dbus_error_free (&error);

    return 1;
  }

  dbus_bool_t ret = dbus_bus_name_has_owner(connection, name, &error);
 
  if (dbus_error_is_set (&error)) {
     dbus_error_free (&error);
     printf ("DBus Error: %s\n", error.message);

     return 1;
  }
 
  if (!ret) {
    printf ("Bus name %s doesn't have an owner, reserving it...\n", name);
    int nr = dbus_bus_request_name(connection,name,
                                   DBUS_NAME_FLAG_DO_NOT_QUEUE, &error);
 
    if (dbus_error_is_set(&error)) {
      dbus_error_free (&error);
      printf("Error requesting a bus name: %s\n", error.message);

      return 1;
    }
 
    if (nr == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
      printf("Bus name %s Successfully reserved!\n", name);
    } else {
      printf("Failed to reserve name %s\n", name);

      return 1;
    }
  } else {
    printf("%s is already reserved. '$ killall barcode-dbus-service; barcode-dbus-service &' to run it manually.\n", name);
    return 1;
  }
 
  mon = udev_monitor_new_from_netlink(udev, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(mon, "hidraw", NULL);
  udev_monitor_enable_receiving(mon);

  fd = udev_monitor_get_fd(mon);

  enumerate = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(enumerate, "hidraw");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  udev_list_entry_foreach(dev_list_entry, devices) {
    const char *path;

    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(udev, path);

    fd_hid = open_hid(dev);
    if (fd_hid > 0) {
      FD_SET(fd_hid, &master);
      if (fd_hid > fdmax) {
        fdmax = fd_hid;
      }
    }
    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);

  while (1) {
    fd_set fds;

    struct timeval tv;
    int ret, i = 0;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    ret = select(fd+1, &fds, NULL, NULL, &tv);
		
    if (ret > 0 && FD_ISSET(fd, &fds)) {
      dev = udev_monitor_receive_device(mon);
      if (dev) {
	printf(" %s ", udev_device_get_action(dev));
	printf("node: %s, ", udev_device_get_devnode(dev));
	printf("subsystem: %s, ", udev_device_get_subsystem(dev));
	printf("devtype: %s.\n", udev_device_get_devtype(dev));

        if (strcmp(udev_device_get_action(dev), "add") == 0) {
          fd_hid = open_hid(dev);
          if (fd_hid > 0) {
            FD_SET(fd_hid, &master);
            if (fd_hid > fdmax) {
              fdmax = fd_hid;
	    }
          }
        } else if (strcmp(udev_device_get_action(dev), "remove") == 0) {
          // close_hid(udev_device_get_devnode(dev));
	}
	udev_device_unref(dev);
      } else {
        printf("No Device from receive_device(). An error occured.\n");
      }					
    }

    read_fds = master;
    ret = select(fdmax+1, &read_fds, NULL, NULL, &tv);
    for (i = 0; i <= fdmax; i++) {
      if (ret > 0 && FD_ISSET(i, &read_fds)) {

        res = read(i, buf, 32);
        if (res < 0) {
          close(i);
          FD_CLR(i, &master);
        } else {
          printf("  read %d bytes: ", res);
	  for (i = 0; i < res; i++)
	    printf("%hhx ", buf[i]);
          puts("\n");

          dbus_send(connection, "read", &buf[4]);
        }
      }
    }
    usleep(250*1000);
    // printf("."); fflush(stdout);
  }

  udev_unref(udev);
  dbus_connection_unref(connection);

  return 0;       
}
