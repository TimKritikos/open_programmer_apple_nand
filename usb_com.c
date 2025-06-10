#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "usb_com.h"

/////////// USB CONSTANTS //////////////
#define VENDOR_ID  0xd369
#define PRODUCT_ID 0x0008

#define ENDPOINT_OUT 0x01
#define ENDPOINT_IN  0x81
#define INTERFACE_NUM   0

#define USB_TIMEOUT_IN_MILLIS 1000000000

struct programmer_t *connect_to_programmer(){

	struct programmer_t *ret = malloc(sizeof(struct programmer_t));

	ret->usb_context = NULL;

	int libusb_ret;

	libusb_ret=libusb_init_context(&ret->usb_context,NULL,0);
	if(libusb_ret){
		fprintf(stderr, "Error init libusb: %s\n", libusb_error_name(libusb_ret));
		free(ret);
		return NULL;
	}

	ret->programmer_handle=libusb_open_device_with_vid_pid(NULL,VENDOR_ID,PRODUCT_ID);
	if(ret->programmer_handle==NULL){
		fprintf(stderr,"Error finding programmer\n");
		libusb_exit(ret->usb_context);
		free(ret);
		return NULL;
	}

	if (libusb_kernel_driver_active(ret->programmer_handle, INTERFACE_NUM) == 1) {
		libusb_detach_kernel_driver(ret->programmer_handle, INTERFACE_NUM);
	}

	libusb_ret=libusb_claim_interface(ret->programmer_handle, INTERFACE_NUM);
	if(libusb_ret){
		fprintf(stderr, "Cannot claim interface: %s\n", libusb_error_name(libusb_ret));
		libusb_close(ret->programmer_handle);
		libusb_exit(ret->usb_context);
		free(ret);
		return NULL;
	}

	return ret;
}


uint64_t read_six_bytes(uint8_t *pointer){
	return	(uint64_t)pointer[0]<<40|
		(uint64_t)pointer[1]<<32|
		(uint64_t)pointer[2]<<24|
		(uint64_t)pointer[3]<<16|
		(uint64_t)pointer[4]<<8 |
		(uint64_t)pointer[5];
}

struct chip_id_t *read_chip_id(struct programmer_t *programmer){

	struct chip_id_t *ret=malloc(sizeof(struct chip_id_t));

	int libusb_ret;
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
	libusb_ret=libusb_bulk_transfer(programmer->programmer_handle, ENDPOINT_OUT, id_request_packet, sizeof(id_request_packet), &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret||transferred!=sizeof(id_request_packet)){
		fprintf(stderr, "Cannot send on interface: %s\n", libusb_error_name(libusb_ret));
		free(ret);
		return NULL;
	}

	uint8_t *in_packet=malloc(1024*1024);
	libusb_ret = libusb_bulk_transfer(programmer->programmer_handle, ENDPOINT_IN, in_packet, 1024*1024, &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret){
		fprintf(stderr, "Error receiving data: %s\n", libusb_error_name(libusb_ret));
		free(in_packet);
		free(ret);
		return NULL;
	}

	if(in_packet[0]!='O'||in_packet[1]!='K'||in_packet[2]!='A'||in_packet[3]!='Y'){
		fprintf(stderr, "Programmer didn't send valid data\n");
		free(in_packet);
		free(ret);
		return NULL;
	}

	ret->nand_id=read_six_bytes(in_packet+0x80);
	ret->nand_extended_id=read_six_bytes(in_packet+0x90);
	ret->nand_information=malloc(MAX_NAND_INFORMATION_BUFFER);
	strncpy(ret->nand_information,(char*)(in_packet+0xA0),MAX_NAND_INFORMATION_BUFFER-1);
	ret->nand_information[MAX_NAND_INFORMATION_BUFFER-1]=0;
	for(int i=0;i<MAX_NAND_INFORMATION_BUFFER;i++)
		if((uint8_t)(ret->nand_information[i])==0xFF)
			ret->nand_information[i]=0;

	free(in_packet);
	return ret;
}

void free_chip_id(struct chip_id_t *tofree){
	free(tofree->nand_information);
	free(tofree);
}

void close_programmer(struct programmer_t *tofree){
	libusb_close(tofree->programmer_handle);
	libusb_exit(tofree->usb_context);
	free(tofree);
}
