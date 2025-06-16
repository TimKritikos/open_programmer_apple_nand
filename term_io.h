#include <stdint.h>
#include "usb_com.h"

#define error(message) error_args(message,"")
#define error_args(message,...) _error_(__FILE__,__LINE__,message,__VA_ARGS__)

int _error_(char* filename,int line,const char *format, ...);
void term_hexdump(uint8_t *data,size_t size);
void term_info(const char* format, ...);
void term_chip_data(struct chip_id_t *chip_id);
