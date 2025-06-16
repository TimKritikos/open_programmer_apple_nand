#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "usb_com.h"

#define VERSION "v0.0-dev"

#define TERM_BLACK   "\e[0;30m"
#define TERM_RED     "\e[0;31m"
#define TERM_GREEN   "\e[0;32m"
#define TERM_YELLOW  "\e[0;33m"
#define TERM_BLUE    "\e[0;34m"
#define TERM_MAGENTA "\e[0;35m"
#define TERM_CYAN    "\e[0;36m"
#define TERM_WHITE   "\e[0;37m"
#define TERM_RESET   "\e[0m"

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
			"  -t                        Perform read test\n"
			, progname);
}

enum ACTION {NOTHING,ID_READ,TEST_READ,READ};

int main(int argc, char **argd){

	enum ACTION action=NOTHING;

	int opt;
	while ((opt = getopt(argc, argd, "vhqt")) != -1) {
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
			case 't':
				action=TEST_READ;
				break;
			default:
				help(argd[0]);
				return 1;
		}
	}

	if(action==NOTHING){
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
			TERM_BLUE "# Got from NAND directly\n"
			TERM_YELLOW "    Nand ID          :" TERM_GREEN " 0x%012lX\n"
			TERM_YELLOW "    Nand extended ID :" TERM_GREEN " 0x%012lX\n"
			TERM_YELLOW "    Nand information :" TERM_GREEN " %s" TERM_RESET "\n",
			chip_id->nand_id,
			chip_id->nand_extended_id,
			chip_id->nand_information
			);

	if ( action == TEST_READ ){
		uint64_t address=0;
		uint8_t *data=malloc(8192);
		if(read_chip_page(programmer,data,address)){
			printf(TERM_BLUE "# Read failed\n");
		}else{
			printf(TERM_BLUE "# Read succeeded\n"TERM_RESET);
			hexdump(data,512);
		}
		free(data);
	}

	free_chip_id(chip_id);
	close_programmer(programmer);
	return 0;
}
