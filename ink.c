#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>


typedef struct printer_s{
    libusb_device *device;  /* libusb device */
    libusb_device_handle *handle; /* libusb handle */
    int bus,
        addr,
        ep_read,
        ep_write,
        iface,
        altset,
        conf;
} printer_t;

typedef struct ink_level_s{
    int cyan;
    int magenta;
    int yellow;
    int black;
} ink_level_t;

enum errors {
    ERR_NO_DEVICES = 0x01,
    ERR_NO_PRINTER_FOUND = 0x02
};

/*
 * So we need to do the following:
 * Connect to the printer
 * Clear buffers
 * Enter IEEE1284.4 mode
 * Init IEEE1284.4 transaction
*/

int clear_buffer(printer_t *printer) {
    int err = 0;
    int trans;

    char buf[200];
    err = libusb_bulk_transfer(printer->handle, printer->ep_read, buf, 200, &trans, 0);

    return err;
}


int enter_ieee(printer_t *printer) {
    clear_buffer(printer);
    unsigned char cmd[] = {
        0x00, 0x00, 0x00, 0x1b, 0x01, '@', 'E', 'J', 'L', ' ',
        '1', '2', '8', '4', '.', '4', 0x0a, '@', 'E', 'J',
        'L', 0x0a, '@', 'E', 'J', 'L', 0x0a
    };

    int err = 0;
    int trans;

    printf("Entering IEEE mode .. \n");
    err = libusb_bulk_transfer(printer->handle, printer->ep_write, cmd, sizeof(cmd), &trans, 1000);
    //write_cmd(printer, cmd, sizeof(cmd), &trans);

    printf("(%d) %d\n", err, trans);

    return 0;
}

int open_channel(printer_t *printer) {

}

int init_printer(printer_t *printer) {
    enter_ieee(printer);
    int err = 0;
    //const char data[] = "\033\1@EJL ID\r\n";
    /*
    int trans;
    printf("%d: %s\n", sizeof(data), data);

    err = libusb_bulk_transfer(printer->handle, printer->epWrite, data, sizeof(data), &trans, 5000);
    printf("(%d) %d\n", err, trans);

    unsigned char buf[1024];

    err = libusb_bulk_transfer(printer->handle, printer->epRead, buf, 1024, &trans, 5000);
    printf("(%d) %d\n", err, trans);
    */
    return 0;
}

int find_printer(libusb_device **list, ssize_t count, printer_t **printer) {
    ssize_t i = 0;
    int found = 0;

    int err = 0;
    if(count < 0) {
        return -ERR_NO_DEVICES;
    }

    for(i = 0; i < count; i++) {
        libusb_device *device = list[i];
        struct libusb_config_descriptor *config_desc;

        err = libusb_get_active_config_descriptor(device, &config_desc);

        int numInterfaces = config_desc->bNumInterfaces;
        int j;

        for(j = 0; j < numInterfaces; j++) {
            struct libusb_interface interface = config_desc->interface[j];
            if(interface.altsetting->bInterfaceClass == LIBUSB_CLASS_PRINTER) {
                /* BINGO! */
                int bus = libusb_get_bus_number(device);
                int addr = libusb_get_device_address(device);

                *printer = malloc(sizeof(printer_t));

                (*printer)->iface = interface.altsetting->bInterfaceNumber;
                (*printer)->bus = bus;
                (*printer)->addr = addr;
                (*printer)->device = device;
                (*printer)->conf = config_desc->bConfigurationValue;
                (*printer)->altset = interface.altsetting->bAlternateSetting;

                int ep = 0;
                for(ep = 0; ep < interface.altsetting->bNumEndpoints; ep++) {
                    if((interface.altsetting->endpoint[ep].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK) {
                        continue;
                    }

                    unsigned char eddr_bits = interface.altsetting->endpoint[ep].bEndpointAddress;
                    unsigned char dir = (eddr_bits & LIBUSB_ENDPOINT_DIR_MASK);

                    switch(dir) {
                        case LIBUSB_ENDPOINT_IN:
                            (*printer)->ep_read = eddr_bits;
                            break;
                        case LIBUSB_ENDPOINT_OUT:
                            (*printer)->ep_write = eddr_bits;
                            break;
                        default:
                            break;
                    }
                }

                found = 1;
                break;
            }
        }

        libusb_free_config_descriptor(config_desc);
        if(found) {
            return 0;
        }
    }

    return -ERR_NO_PRINTER_FOUND;

}

int get_ink_level(printer_t *printer, ink_level_t **ink) {
    unsigned char *data = "di\1\0\1";
    int trans = 0;
    int err = 0;

    err = init_printer(printer);

    printf("Sending status .. \n");

    err = libusb_bulk_transfer(printer->handle, printer->ep_write, data, 5, &trans, 5000);
    printf("%d\n", err);

    return 0;
}


int main(int argc, char **argv) {
    if(libusb_init(NULL)) {
        printf("%s\n", "No usb :(");
        return 1;
    }

    libusb_set_debug(NULL, 3);

    libusb_device **list;
    ssize_t count = libusb_get_device_list(NULL, &list);

    printer_t *printer = NULL;

    int found = find_printer(list, count, &printer);

    if(!found) {
        int err = 0;
        libusb_device_handle *dev;
        err = libusb_open(printer->device, &dev);

        printer->handle = dev;

        /* TODO: Add if to check returns */
        if(err = libusb_set_configuration(dev, printer->conf)) {
            printf("ERR set_config: %d\n", err);
        }

        if(err = libusb_claim_interface(dev, printer->iface)) {
            printf("ERR claim_usb: %d\n", err);
        }

        if(err = libusb_set_interface_alt_setting(dev, printer->iface, printer->altset)) {
            printf("ERR set_alt: %d\n", err);
        }

        printf("Found Printer @ %d:%d\n", printer->bus, printer->addr);
        printf("bInterfaceNumber: %d\n", printer->iface);
        printf("Config: %d\n", printer->conf);
        printf("Read: 0x%x\n", printer->ep_read);
        printf("Write: 0x%x\n", printer->ep_write);

        printf("Max Write Size: %d\n", libusb_get_max_packet_size(printer->device, printer->ep_write));
        printf("Max Read Size: %d\n", libusb_get_max_packet_size(printer->device, printer->ep_read));

        ink_level_t *ink = NULL;
        //get_ink_level(printer, &ink);
        init_printer(printer);

        err = libusb_release_interface(dev, printer->iface);
        libusb_close(dev);

        free(printer);
    } else {
        printf("Unable to find printer.\n");
    }

    libusb_free_device_list(list, 1);

    libusb_exit(NULL);

    return 0;
}
