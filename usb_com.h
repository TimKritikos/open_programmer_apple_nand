#include <libusb-1.0/libusb.h>

struct programmer_t{
	libusb_device_handle *programmer_handle;
	libusb_context *usb_context;
};

#define MAX_NAND_INFORMATION_BUFFER 0x80
struct chip_id_t{
	uint64_t nand_id;
	uint64_t nand_extended_id;
	char* nand_information;
};

struct programmer_t *connect_to_programmer();
struct chip_id_t *read_chip_id(struct programmer_t *programmer);
void free_chip_id(struct chip_id_t *tofree);
void close_programmer(struct programmer_t *tofree);
int read_chip_page(struct programmer_t *programmer,uint8_t *data,uint64_t address);
