/* main.c

   This file is part of the Open Programmer for Apple NANDs project

   Copyright (c) 2025 Efthymios Kritikos

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "usb_com.h"
#include "term_io.h"

#define VERSION "v0.0-dev"

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
		printf("Couldn't connect to IP BOX\n");
		return 1;
	}

	struct chip_id_t *chip_id=read_chip_id(programmer);
	if( !chip_id ){
		close_programmer(programmer);
		return 1;
	}

	term_info("#Got from NAND directly\n");
	term_chip_data(chip_id);

	if ( action == TEST_READ ){
		uint64_t address=0;
		uint8_t *data=malloc(256);
		if(read_chip(programmer,data,address)){
			term_info("# Read failed\n");
		}else{
			term_info("# Read succeeded\n");
			term_hexdump(data,256);
		}
		free(data);
	}

	free_chip_id(chip_id);
	close_programmer(programmer);
	return 0;
}
