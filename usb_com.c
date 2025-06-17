#include <libusb-1.0/libusb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "usb_com.h"
#include "term_io.h"
#include <assert.h>

/////////// USB CONSTANTS //////////////
#define VENDOR_ID  0xd369
#define PRODUCT_ID 0x0008

#define ENDPOINT_OUT 0x01
#define ENDPOINT_IN  0x81
#define INTERFACE_NUM   0

#define USB_TIMEOUT_IN_MILLIS 8000

// connect_to_programmer - Establish a connection to the USB programmer
// (return)   : NULL on failure, a pointer to a struct programmer_t on success
struct programmer_t *connect_to_programmer(){

	struct programmer_t *ret = malloc(sizeof(struct programmer_t));

	ret->usb_context = NULL;

	int libusb_ret;

	libusb_ret=libusb_init_context(&ret->usb_context,NULL,0);
	if(libusb_ret){
		error_args("Error init libusb: %s", libusb_error_name(libusb_ret));
		free(ret);
		return NULL;
	}

	ret->programmer_handle=libusb_open_device_with_vid_pid(NULL,VENDOR_ID,PRODUCT_ID);
	if(ret->programmer_handle==NULL){
		error("Error finding programmer");
		libusb_exit(ret->usb_context);
		free(ret);
		return NULL;
	}

	if (libusb_kernel_driver_active(ret->programmer_handle, INTERFACE_NUM) == 1) {
		libusb_detach_kernel_driver(ret->programmer_handle, INTERFACE_NUM);
	}

	libusb_ret=libusb_claim_interface(ret->programmer_handle, INTERFACE_NUM);
	if(libusb_ret){
		error_args("Cannot claim interface: %s", libusb_error_name(libusb_ret));
		libusb_close(ret->programmer_handle);
		libusb_exit(ret->usb_context);
		free(ret);
		return NULL;
	}

	return ret;
}


// read_six_bytes - Read size bytes in big endian and return the data as a uint64_t
// pointer    : The pointer to the six bytes that need to be converted
// (return)   : The converted data
uint64_t read_six_bytes(uint8_t *pointer){
	return	(uint64_t)pointer[0]<<40|
		(uint64_t)pointer[1]<<32|
		(uint64_t)pointer[2]<<24|
		(uint64_t)pointer[3]<<16|
		(uint64_t)pointer[4]<<8 |
		(uint64_t)pointer[5];
}

// usb_send_bulk - Send usb data in bulk mode
// programmer : A pointer to a connected programmer
// packet     : A pointer to where to read the data from
// size       : The size of data to send over USB
// (return)   : 1 for failure, 0 for success
int usb_send_bulk(struct programmer_t *programmer,uint8_t* packet, size_t size){
	int libusb_ret, transferred;
	libusb_ret=libusb_bulk_transfer(programmer->programmer_handle, ENDPOINT_OUT, packet, size, &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret||transferred!=(int)size){
		error_args("Cannot send on interface: %s", libusb_error_name(libusb_ret));
		return 1;
	}
	return 0;
}

// usb_receive_bulk - Read usb data in bulk mode
// programmer : A pointer to a connected programmer
// packet     : A pointer to where to write the data to
// size       : The size of data alocated to packet
// (return)   : -1 on failure, a non negative value on success representing the amount of data read
int usb_receive_bulk(struct programmer_t *programmer,uint8_t* packet, size_t size){
	int libusb_ret, transferred;
	libusb_ret = libusb_bulk_transfer(programmer->programmer_handle, ENDPOINT_IN, packet, size, &transferred, USB_TIMEOUT_IN_MILLIS);
	if (libusb_ret){
		error_args("Error receiving data: %s", libusb_error_name(libusb_ret));
		return -1;
	}
	return transferred;
}

// read_chip_id - Read the reported chip id from the programmer
// programmer : A pointer to a connected programmer
// (return)   : NULL on failure, a pointer to a struct chip_id_t on success
struct chip_id_t *read_chip_id(struct programmer_t *programmer){

	struct chip_id_t *ret=malloc(sizeof(struct chip_id_t));

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


	if(usb_send_bulk(programmer,id_request_packet,sizeof(id_request_packet))){
		free(ret);
		return NULL;
	}

	uint8_t *in_packet=malloc(1024*1024);

	int size;
	if((size=usb_receive_bulk(programmer,in_packet,1024*1024))<0){
		free(in_packet);
		free(ret);
		return NULL;
	}

	if(in_packet[0]!='O'||in_packet[1]!='K'||in_packet[2]!='A'||in_packet[3]!='Y'){
		error("Programmer didn't send expected data");
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

struct read_command_packet_t{
	char command[8];
	uint32_t address;
	uint16_t size; // in  bytes
	uint8_t unkown_byte1;
	uint8_t unkown_byte2;
	uint8_t unkown_byte3;
	uint8_t boolean2;
	uint8_t boolean3;
	char unkown[109];
};
static_assert(sizeof(struct read_command_packet_t) == 128, "Struct read_command_packet_t needs to be 128 bytes");

// read_chip - Read data from the NAND memory
// programmer : A pointer to a connected programmer
// data       : A pointer to where to write the data. *THIS NEEDS TO BE AT LEAST 512*
// address    : The addess of the page to be read
// (return)   : 1 on failure, 0 on success
int read_chip(struct programmer_t *programmer,uint8_t *data,uint64_t address){
	struct read_command_packet_t *read_packet=malloc(sizeof(struct read_command_packet_t));
	memset(read_packet,0,sizeof(struct read_command_packet_t));
	memcpy(read_packet->command,"READ",4);
	read_packet->address=address;
	read_packet->size=256;
	read_packet->unkown_byte3=2;

	if(usb_send_bulk(programmer,(uint8_t*)read_packet,sizeof(struct read_command_packet_t)))
		return 1;

	uint8_t *in_packet=malloc(1024*1024);
	int size;

	if(!(size=usb_receive_bulk(programmer,in_packet,1024*1024))){
		free(in_packet);
		free(read_packet);
		return 1;
	}

	if(in_packet[0]!='O'||in_packet[1]!='K'||in_packet[2]!='A'||in_packet[3]!='Y'||size!=128+read_packet->size){
		error("Programmer didn't send expected data");
		free(in_packet);
		free(read_packet);
		return 1;
	}

	#define maximum_buffer_size 256
	memcpy(data,in_packet+128,read_packet->size>maximum_buffer_size?maximum_buffer_size:read_packet->size);

	free(in_packet);
	free(read_packet);
	return 0;
}
#pragma GCC diagnostic pop

// free_chip_id - Free a struct chip_id_t
// tofree     : A pointer to a struct chip_id_t to be freed
void free_chip_id(struct chip_id_t *tofree){
	free(tofree->nand_information);
	free(tofree);
}

// close_programmer - Close the connection to a programmer and free assoisated data
// tofree     : A pointer to a struct programmer_t to be closed and freed
void close_programmer(struct programmer_t *tofree){
	libusb_close(tofree->programmer_handle);
	libusb_exit(tofree->usb_context);
	free(tofree);
}
