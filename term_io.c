/* term_io.c - Code for printing to the terminal

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
#include <stdarg.h>
#include "term_io.h"

#define TERM_BLACK   "\e[0;30m"
#define TERM_RED     "\e[0;31m"
#define TERM_GREEN   "\e[0;32m"
#define TERM_YELLOW  "\e[0;33m"
#define TERM_BLUE    "\e[0;34m"
#define TERM_MAGENTA "\e[0;35m"
#define TERM_CYAN    "\e[0;36m"
#define TERM_WHITE   "\e[0;37m"
#define TERM_RESET   "\e[0m"

int _error_(char* filename,int line,const char *format, ...){
	va_list args;
	va_start(args, format);

	if(!fprintf(stderr,"%s:%d: ",filename,line)){
		va_end(args);
	    return 1;
	}
	if(!vfprintf(stderr,format, args)){
		va_end(args);
	    return 1;
	}
	if(fprintf(stderr,"\n")){
		va_end(args);
	    return 1;
	}
	fflush(stderr);

	va_end(args);
	return 0;
}

void term_hexdump(uint8_t *data,size_t size){
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

void term_chip_data(struct chip_id_t *chip_id){
	printf( TERM_YELLOW "    Nand ID          :" TERM_GREEN " 0x%012lX\n"
	        TERM_YELLOW "    Nand extended ID :" TERM_GREEN " 0x%012lX\n"
	        TERM_YELLOW "    Nand information :" TERM_GREEN " %s" TERM_RESET "\n",
	        chip_id->nand_id,
	        chip_id->nand_extended_id,
	        chip_id->nand_information );
}

void term_info(const char* format, ...){
	va_list args;
	va_start(args, format);

	printf( TERM_BLUE );
	vprintf(format,args);
	printf( TERM_RESET );
	fflush(stdout); // Hit what might be a bug where stderr was printed right after this and the color was still blue

	va_end(args);
}
