#include <stdio.h>
#include <libusb-1.0/libusb.h>

int main(int argc, char **argv){
	libusb_device_handle *usb_programmer;

	if(libusb_init(NULL)){
		perror("Error init libusb");
		return 1;
	}

	libusb_exit(NULL);
	return 0;
}
