/*
    loadrom.c --
    File loading and management.
*/

#include "shared.h"

typedef struct
{
    uint32_t crc;
    int mapper;
    int display;
    int territory;
    char *name;
} rominfo_t;

// rominfo_t game_list[] = {
//     {0x17AB6883, MAPPER_NONE, DISPLAY_NTSC, TERRITORY_EXPORT, "FA Tetris (KR)"},
//     {0x61E8806F, MAPPER_NONE, DISPLAY_NTSC, TERRITORY_EXPORT, "Flash Point (KR)"},
//     {0x192949D5, MAPPER_KOREA2, DISPLAY_NTSC, TERRITORY_EXPORT, "Janggun-iuo Adeul (KR)"},
//     {0xA05258F5, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Won-Si-In (KR)"},
//     {0x83F0EEDE, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Street Master (KR)"},
//     {0x445525E2, MAPPER_KOREA, DISPLAY_NTSC, TERRITORY_EXPORT, "Penguin Adventure (KR)"},
//     {0x29822980, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Cosmic Spacehead"},
//     {0xB9664AE1, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Fantastic Dizzy"},
//     {0xA577CE46, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Micro Machines"},
//     {0x8813514B, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Excellent Dizzy (Proto)"},
//     {0xAA140C9C, MAPPER_CODIES, DISPLAY_PAL, TERRITORY_EXPORT, "Excellent Dizzy (Proto - GG)"},
//     {-1, -1, -1, -1, NULL},
// };

// int load_rom(char *filename)
// {
//     int i;
//     int size;

//     if(cart.rom)
//     {
//         free(cart.rom);
//         cart.rom = NULL;
//     }

// 	FILE *fd = NULL;

// 	fd = fopen(filename, "rb");
// 	if(!fd) return 0;

// 	/* Seek to end of file, and get size */
// 	fseek(fd, 0, SEEK_END);
// 	size = ftell(fd);
// 	fseek(fd, 0, SEEK_SET);

// 	cart.rom = malloc(size);
// 	if(!cart.rom) return 0;
// 	fread(cart.rom, size, 1, fd);

// 	fclose(fd);

//     /* Don't load games smaller than 16K */
//     if(size < 0x4000) return 0;

//     /* Take care of image header, if present */
//     if((size / 512) & 1)
//     {
//         size -= 512;
//         memmove(cart.rom, cart.rom + 512, size);
//     }

//     cart.pages = (size / 0x4000);
//     cart.crc = crc32(0L, cart.rom, size);

//     uint8_t *temprom = malloc(size * sizeof(uint8_t));
//     memcpy(temprom, cart.rom, size);
//     sha1(cart.sha1, temprom, size);
//     free(temprom);

//     /* Assign default settings (US NTSC machine) */
//     cart.mapper     = MAPPER_SEGA;
//     sms.display     = DISPLAY_NTSC;
//     sms.territory   = TERRITORY_EXPORT;

//     /* Look up mapper in game list */
//     for(i = 0; game_list[i].name != NULL; i++)
//     {
//         if(cart.crc == game_list[i].crc)
//         {
//             cart.mapper     = game_list[i].mapper;
//             sms.display     = game_list[i].display;
//             sms.territory   = game_list[i].territory;
//         }
//     }

//     system_assign_device(PORT_A, DEVICE_PAD2B);
//     system_assign_device(PORT_B, DEVICE_PAD2B);

//     return 1;
// }
int load_rom(uintptr_t addr, int size, int cartType)
{
    uint8_t *start = (uint8_t *)addr;
    /* SG-1000 images have no copier header, and sizes such as 49136 or
       65535 bytes would wrongly trigger the strip below. */
    if (cartType != TYPE_SG && ((size / 512) & 1))
    {
        size -= 512;
        start += 512;
    }
    sms.use_fm = 0;
    sms.country = TYPE_OVERSEAS;
    /* sms.dummy marks the pages where writes are discarded. It is compared by
       pointer and never written through - cpu_writemem16() and sg_writemem()
       both test for it - so it can share the 256-byte line buffer. */
    sms.dummy = smsBufferLine;
    bitmap.data = smsBufferLine;
    bitmap.width = BMP_WIDTH;
    bitmap.height = BMP_HEIGHT;
    bitmap.pitch = BMP_WIDTH;
    bitmap.depth = 8;
    cart.rom = start;
    cart.size = size;
    cart.type = cartType;

    /* Codemasters cartridges carry their own header at $7FE0, with a checksum
       at $7FE6 and its complement at $7FE8 that add up to $10000; Mesen2 detects
       them the same way. That finds every Codemasters rom in SMS Plus GX's CRC
       list and also their betas, hacks and translations, without a table.
       Anything else up to 48K has no mapper chip, as in Mesen2: the rom fills
       $0000-$BFFF and a write to $FFFC-$FFFF reaches work RAM only. The MSX
       conversions clear all of work RAM and then enter the game through the
       MSX header at $4002, which a Sega mapper would have switched to page 0. */
    cart.mapper = (size > 0xC000) ? MAPPER_SEGA : MAPPER_NONE;
    if (cartType != TYPE_SG && size > 0x8000)
    {
        int checksum = start[0x7FE6] | (start[0x7FE7] << 8);
        int complement = start[0x7FE8] | (start[0x7FE9] << 8);
        if (checksum + complement == 0x10000)
            cart.mapper = MAPPER_CODIES;
    }

    if (cartType == TYPE_SG)
    {
        /* SG ROMs can be 8 KB or not a multiple of 16 KB: round up so the
           mapper modulo never sees zero pages and the last partial page counts. */
        cart.pages = (size + 0x3FFF) >> 14;
    }
    else
    {
        /* Never zero: sms_mapper_w() reduces every bank number modulo this,
           and a rom under 16K would round down to no pages at all. A rom that
           small has one page as far as the mapper is concerned, and every bank
           number resolves to it, which is what the hardware does when the
           cartridge has no bank lines to drive. This is a floor, not a
           rounding: rounding a partial page up would let the mapper select a
           page the buffer does not hold. */
        cart.pages = size / 0x4000;
        if (cart.pages == 0)
            cart.pages = 1;
    }
    return 1;
}

