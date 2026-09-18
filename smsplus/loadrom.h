#ifndef _LOADROM_H_
#define _LOADROM_H_

/* Function prototypes */
//int load_rom(char *filename);
int load_rom(uintptr_t addr, int size, int cartType);
int load_rom_mem(uint8_t *rom, size_t size);

#endif /* _LOADROM_H_ */
