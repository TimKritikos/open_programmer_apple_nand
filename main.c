#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

////////// PROGRAM CONSTANTS ///////////
#define VERSION "v0.0-dev"

/////////// USB CONSTANTS //////////////
#define VENDOR_ID  0xd369
#define PRODUCT_ID 0x0008

#define ENDPOINT_OUT 0x01
#define ENDPOINT_IN  0x81
#define INTERFACE_NUM   0

#define USB_TIMEOUT_IN_MILLIS 1000000000

void hexdump(uint8_t *data,size_t size){
	char ascii[17]={' '};
	ascii[16]=0;
	for (unsigned int i=0;i<size;i++){
		printf("%02X ",data[i]);
		if((data[i]>='a'&&data[i]<='z')||(data[i]>='A'&&data[i]<='Z')||(data[i]>='0'&&data[i]<='9'))
			ascii[i%16]=data[i];
		else
			ascii[i%16]='.';
		if((i%16)==15){
			printf(" | %s\n",ascii);
			for(int i=0;i<16;i++)
				ascii[i]=' ';
		}
	}
}

void help(char* progname){
	printf(
			"Usage: %s [options]  \n"
			"Options:\n"
			"  -v                        Print the program version and exit successfully\n"
			"  -h                        Print this help message and exit successfully\n"
			"  -q                        Query for the chip ID\n"
			, progname);
}

enum ACTION {NOTHING,ID_READ,TEST_READ,READ};

int main(int argc, char **argd){

	enum ACTION action=NOTHING;

	int opt;
	while ((opt = getopt(argc, argd, "vhq")) != -1) {
		switch (opt) {
			case 'v':
				printf("%s\n",VERSION);
				return 0;
			case 'q':
				action=ID_READ;
				break;
			case 'h':
				help(argd[0]);
				return 0;
				break;
			default:
				help(argd[0]);
				return 1;
		}
	}

	if(action!=ID_READ){
		printf("Nothing to do, exiting\n");
		return 0;
	}

	libusb_device_handle *programmer_handle;
	libusb_context *usb_context = NULL;
	int libusb_ret;

	libusb_ret=libusb_init_context(&usb_context,NULL,0);
	if(libusb_ret){
		fprintf(stderr, "Error init libusb: %s\n", libusb_error_name(libusb_ret));
		return -1;
	}

	programmer_handle=libusb_open_device_with_vid_pid(NULL,VENDOR_ID,PRODUCT_ID);
	if(programmer_handle==NULL){
		fprintf(stderr,"Error finding programmer\n");
		libusb_exit(usb_context);
		return -1;
	}

	if (libusb_kernel_driver_active(programmer_handle, INTERFACE_NUM) == 1) {
		libusb_detach_kernel_driver(programmer_handle, INTERFACE_NUM);
	}

	libusb_ret=libusb_claim_interface(programmer_handle, INTERFACE_NUM);
	if(libusb_ret){
		fprintf(stderr, "Cannot claim interface: %s\n", libusb_error_name(libusb_ret));
		libusb_close(programmer_handle);
		libusb_exit(usb_context);
		return EXIT_FAILURE;
	}

	uint8_t id_request_packet[128]={ 0x52,0x44,0x49,0x44,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x01,0x4a,0x01,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	                                 0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00};
	int transferred;
	libusb_ret=libusb_bulk_transfer(programmer_handle, ENDPOINT_OUT, id_request_packet, sizeof(id_request_packet), &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret||transferred!=sizeof(id_request_packet)){
		fprintf(stderr, "Cannot claim interface: %s\n", libusb_error_name(libusb_ret));
		libusb_close(programmer_handle);
		libusb_exit(usb_context);
		return EXIT_FAILURE;
	}

	uint8_t *in_packet=malloc(1024*1024);
	libusb_ret = libusb_bulk_transfer(programmer_handle, ENDPOINT_IN, in_packet, 1024*1024, &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret){
		fprintf(stderr, "Error receiving data: %s\n", libusb_error_name(libusb_ret));
		libusb_close(programmer_handle);
		libusb_exit(usb_context);
		return EXIT_FAILURE;
	}
	printf("Received %d bytes\n", transferred);
	hexdump(in_packet,(size_t)transferred);

	libusb_close(programmer_handle);
	libusb_exit(usb_context);
	return 0;
}
