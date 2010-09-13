#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

struct printer_device {
    int vendor;
    int product;
    libusb_device *dev;
    libusb_device_handle *handle;
};

int get_printer() {
    libusb_device **list;
    libusb_device *found = NULL;

    ssize_t count = libusb_get_device_list(NULL, &list);
    ssize_t i = 0;

    int err = 0;
    if(count < 0) {
        return 1;
    }

    for(i = 0; i < count; i++) {
        libusb_device *device = list[i];
        struct libusb_device_descriptor opts;

        int bus = libusb_get_bus_number(device);
        int addr = libusb_get_device_address(device);

        printf("%d:%d ", bus, addr);

        err = libusb_get_device_descriptor(device, &opts);
        if(!err) {
            printf("0x%x\n", opts.bDeviceSubClass);
            if(opts.bDeviceClass == LIBUSB_CLASS_PRINTER) {
                printf("%d: %X %X %d\n", i, (int) opts.idVendor, (int) opts.idProduct, opts.bDeviceClass);
            } else if(opts.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE) {
            }
        }
    }

    if(found) {
        libusb_device_handle *handle;

        err = libusb_open(found, &handle);
        if(err) {
            return 1;
        }
    }

    libusb_free_device_list(list, 1);

    return 0;

}

int main(int argc, char **argv) {
    if(libusb_init(NULL)) {
        printf("%s\n", "No usb :(");
        return 1;
    }

    printf("%s\n", "We has usb!");

    get_printer();

    libusb_exit(NULL);

    return 0;
}
