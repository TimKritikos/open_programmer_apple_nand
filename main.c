#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "usb_com.h"

#define VERSION "v0.0-dev"

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

	struct programmer_t *programmer=connect_to_programmer();
	if ( !programmer ){
		fprintf(stderr,"Couldn't connect to IP BOX\n");
		return 1;
	}

	struct chip_id_t *chip_id=read_chip_id(programmer);
	if( !chip_id ){
		close_programmer(programmer);
		return 1;
	}

	printf(
			"Nand ID          : 0x%012lX\n"
			"Nand extended ID : 0x%012lX\n"
			"Nand information : %s\n",
			chip_id->nand_id,
			chip_id->nand_extended_id,
			chip_id->nand_information
			);

	free_chip_id(chip_id);
	close_programmer(programmer);
	return 0;
}
