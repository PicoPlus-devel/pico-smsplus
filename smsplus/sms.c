
#include "shared.h"

/* SMS context */
t_sms sms;

extern void ym2413_write(int chip, int offset, int data);

/* Run the virtual console emulation for one frame */
void in_ram(sms_frame)(int skip_render) {
    /* Take care of hard resets */
    if (input.system & INPUT_HARD_RESET) {
        system_reset();
    }

    /* Debounce pause key */
    if (input.system & INPUT_PAUSE) {
        if (!sms.paused) {
            sms.paused = 1;

            z80_set_nmi_line(ASSERT_LINE);
            z80_set_nmi_line(CLEAR_LINE);
        }
    } else {
        sms.paused = 0;
    }

    if (snd.log) snd.callback(0x00);

    /* Render audio per-scanline so that Z80 writes to PSG registers
       (e.g. Game Gear PCM speech streaming) modulate the output stream
       within a frame instead of being collapsed to a single end-of-frame
       sample-and-hold. 262 lines × 60 fps = 15.72 kHz update rate. */
    int samples_rendered = 0;

    for (vdp.line = 0; vdp.line < 262; vdp.line += 1) {
        /* Handle VDP line events */
        vdp_run();

        /* Draw the current frame */
        if (!skip_render) render_line(vdp.line);

        /* Run the Z80 for a line */
        z80_execute(227);

        /* Render the audio samples that correspond to this scanline.
           Accumulator math guarantees Σ samples_this_line == snd.bufsize. */
        if (snd.enabled) {
            int target = (vdp.line + 1) * snd.bufsize / 262;
            int n = target - samples_rendered;
            if (n > 0) {
                INT16 *part[2] = {
                    snd.buffer[0] + samples_rendered,
                    snd.buffer[1] + samples_rendered
                };
                SN76496Update(0, part, n, sms.psg_mask);
                system_mix_fm(part[0], part[1], n);
                samples_rendered = target;
            }
        }
    }
}


void sms_init(void) {
    cpu_reset();
    sms_reset();
}


void sms_reset(void) {
    /* Clear SMS context */

    __builtin_memset(sms.sram, 0, SRAM_BYTES);
    __builtin_memset(sms.ram, 0, RAMSIZEBYTES);

    sms.paused = sms.save = sms.port_3F = sms.port_F2 = sms.irq = 0x00;
    sms.psg_mask = 0xFF;

    if (IS_SG) {
        /* Linear paging for the > 48 KB images that use the Sega mapper */
        sms.fcr[0] = 0x00;
        sms.fcr[1] = 0x00;
        sms.fcr[2] = 0x01;
        sms.fcr[3] = 0x02;
        sg_memory_map();
        return;
    }

    /* Load memory maps with default values */
    cpu_readmap[0] = cart.rom + 0x0000;
    cpu_readmap[1] = cart.rom + 0x2000;
    cpu_readmap[2] = cart.rom + 0x4000;
    cpu_readmap[3] = cart.rom + 0x6000;
    cpu_readmap[4] = cart.rom + 0x0000;
    cpu_readmap[5] = cart.rom + 0x2000;
    cpu_readmap[6] = sms.ram;
    cpu_readmap[7] = sms.ram;

    cpu_writemap[0] = sms.dummy;
    cpu_writemap[1] = sms.dummy;
    cpu_writemap[2] = sms.dummy;
    cpu_writemap[3] = sms.dummy;
    cpu_writemap[4] = sms.dummy;
    cpu_writemap[5] = sms.dummy;
    cpu_writemap[6] = sms.ram;
    cpu_writemap[7] = sms.ram;

    sms.fcr[0] = 0x00;
    sms.fcr[1] = 0x00;
    sms.fcr[2] = 0x01;
    sms.fcr[3] = 0x00;
}


/* Reset Z80 emulator */
void cpu_reset(void) {
    z80_reset(0);
    z80_set_irq_callback(sms_irq_callback);
}


/*--------------------------------------------------------------------------*/
/* SG-1000                                                                  */
/*--------------------------------------------------------------------------*/

/* Unmapped ROM space reads as open bus. Kept in flash. */
static const uint8 sg_open_bus[0x2000] = {[0 ... 0x1FFF] = 0xFF};

/* Pages ($2000 and/or $8000) where a Taiwanese RAM adaptor may sit: bit n
   set = page n. Derived from the ROM image, so not part of the save state. */
static uint8 sg_adaptor_pages;

/* True when the ROM image has no data in this 8 KB page (only $00/$FF) */
static int sg_page_is_blank(int page) {
    int base = page << 13;
    int len = cart.size - base;
    int i;

    if (len > 0x2000) len = 0x2000;
    for (i = 0; i < len; i++) {
        uint8 b = cart.rom[base + i];
        if (b != 0x00 && b != 0xFF) return 0;
    }
    return 1;
}

/* Build the SG-1000 memory map. Called after reset and after a state load.
   $0000-$BFFF: cartridge ROM, 8 KB pages. Pages the ROM does not cover read
   open bus below $8000 and are cartridge RAM (sms.sram, 8 KB mirrored) from
   $8000 up (The Castle, Othello Multivision). Pages flagged in sms.port_3F
   were found to be Taiwanese RAM adaptor pages by sg_writemem.
   $C000-$FFFF: 8 KB work RAM, mirrored. */
void sg_memory_map(void) {
    int page;

    sg_adaptor_pages = 0;
    if (cart.size <= 0xC000) {
        if (sg_page_is_blank(1)) sg_adaptor_pages |= (1 << 1);
        if (sg_page_is_blank(4)) sg_adaptor_pages |= (1 << 4);
    }

    for (page = 0; page < 6; page++) {
        if ((page << 13) < cart.size) {
            cpu_readmap[page] = cart.rom + (page << 13);
            cpu_writemap[page] = sms.dummy;
        } else if (page < 4) {
            cpu_readmap[page] = (uint8 *)sg_open_bus;
            cpu_writemap[page] = sms.dummy;
        } else {
            cpu_readmap[page] = sms.sram;
            cpu_writemap[page] = sms.sram;
        }
        if (sms.port_3F & (1 << page)) {
            cpu_readmap[page] = sms.sram;
            cpu_writemap[page] = sms.sram;
        }
    }
    cpu_readmap[6] = sms.ram;
    cpu_readmap[7] = sms.ram;
    cpu_writemap[6] = sms.ram;
    cpu_writemap[7] = sms.ram;

    /* Images above 48 KB page through the Sega mapper (slots 1-3 only) */
    if (cart.size > 0xC000) {
        sms_mapper_w(1, sms.fcr[1]);
        sms_mapper_w(2, sms.fcr[2]);
        sms_mapper_w(3, sms.fcr[3]);
    }
}

static void sg_writemem(int address, int data) {
    int page = address >> 13;

    if (cpu_writemap[page] != sms.dummy) {
        cpu_writemap[page][address & 0x1FFF] = data;
        /* $FFFC (RAM paging) does not exist on SG carts */
        if (address >= 0xFFFD && cart.size > 0xC000) sms_mapper_w(address & 3, data);
        return;
    }

    /* A write into $2000 or $8000 means the Taiwanese 8 KB RAM adaptor: map
       RAM over that page from now on (idea from picodrive's write_bank_x8k).
       Only pages without ROM data qualify; Sega carts such as Pop Flamer
       write stray bytes into their own code, which must stay mapped. */
    if (sg_adaptor_pages & (1 << page)) {
        printf("SG: RAM adaptor detected at $%04X (write $%02X to $%04X)\n", page << 13, data, address);
        sms.port_3F |= (1 << page);
        cpu_readmap[page] = sms.sram;
        cpu_writemap[page] = sms.sram;
        sms.sram[address & 0x1FFF] = data;
    }
}

/* Pad bits shared by the SMS port $DC and the SG-1000 even $C0-$FF ports */
static uint8 read_port_dc(void) {
    uint8 temp = 0xFF;
    if (input.pad[0] & INPUT_UP) temp &= ~0x01;
    if (input.pad[0] & INPUT_DOWN) temp &= ~0x02;
    if (input.pad[0] & INPUT_LEFT) temp &= ~0x04;
    if (input.pad[0] & INPUT_RIGHT) temp &= ~0x08;
    if (input.pad[0] & INPUT_BUTTON2) temp &= ~0x10;
    if (input.pad[0] & INPUT_BUTTON1) temp &= ~0x20;
    if (input.pad[1] & INPUT_UP) temp &= ~0x40;
    if (input.pad[1] & INPUT_DOWN) temp &= ~0x80;
    return temp;
}

/* SG-1000 I/O is decoded on A7/A6 only */
static void sg_writeport(int port, int data) {
    switch (port & 0xC0) {
        case 0x40: /* SN76489 PSG */
            if (snd.log) {
                snd.callback(0x03);
                snd.callback(data);
            }
            if (snd.enabled) SN76496Write(0, data);
            break;

        case 0x80: /* TMS9918A: odd = control, even = data */
            if (port & 1)
                vdp_ctrl_w(data);
            else
                vdp_data_w(data);
            break;
    }
}

static int sg_readport(int port) {
    uint8 temp;

    switch (port & 0xC0) {
        case 0x80: /* TMS9918A: odd = status, even = data */
            return (port & 1) ? vdp_ctrl_r() : vdp_data_r();

        case 0xC0: /* Joypads: even = $DC layout, odd = $DD layout */
            if (!(port & 1)) return read_port_dc();
            temp = 0xFF;
            if (input.pad[1] & INPUT_LEFT) temp &= ~0x01;
            if (input.pad[1] & INPUT_RIGHT) temp &= ~0x02;
            if (input.pad[1] & INPUT_BUTTON2) temp &= ~0x04;
            if (input.pad[1] & INPUT_BUTTON1) temp &= ~0x08;
            return temp;
    }
    return 0xFF;
}


/* Write to memory */
void cpu_writemem16(int address, int data) {
    if (IS_SG) {
        sg_writemem(address, data);
        return;
    }
    cpu_writemap[(address >> 13)][(address & 0x1FFF)] = data;
    if (address >= 0xFFFC) sms_mapper_w(address & 3, data);
}


/* Write to an I/O port */
void cpu_writeport(int port, int data) {
    if (IS_SG) {
        sg_writeport(port, data);
        return;
    }
    switch (port & 0xFF) {
        case 0x01: /* GG SIO */
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
            break;

        case 0x06: /* GG STEREO */
            if (snd.log) {
                snd.callback(0x04);
                snd.callback(data);
            }
            sms.psg_mask = (data & 0xFF);
            break;

        case 0x7E: /* SN76489 PSG */
        case 0x7F:
            if (snd.log) {
                snd.callback(0x03);
                snd.callback(data);
            }
            if (snd.enabled) SN76496Write(0, data);
            break;

        case 0xBE: /* VDP DATA */
            vdp_data_w(data);
            break;

        case 0xBD: /* VDP CTRL */
        case 0xBF:
            vdp_ctrl_w(data);
            break;

        case 0xF0: /* YM2413 */
        case 0xF1:
            if (snd.log) {
                snd.callback((port & 1) ? 0x06 : 0x05);
                snd.callback(data);
            }
            if (snd.enabled && sms.use_fm) ym2413_write(0, port & 1, data);
            break;

        case 0xF2: /* YM2413 DETECT */
            if (sms.use_fm) sms.port_F2 = (data & 1);
            break;

        case 0x3F: /* TERRITORY CTRL. */
            sms.port_3F = ((data & 0x80) | (data & 0x20) << 1) & 0xC0;
            if (sms.country == TYPE_DOMESTIC) sms.port_3F ^= 0xC0;
            break;
    }
}


/* Read from an I/O port */
int cpu_readport(int port) {
    uint8 temp = 0xFF;

    if (IS_SG) return sg_readport(port);

    switch (port & 0xFF) {
        case 0x01: /* GG SIO */
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
            return (0x00);

        case 0x7E: /* V COUNTER */
            return (vdp_vcounter_r());
            break;

        case 0x7F: /* H COUNTER */
            return (vdp_hcounter_r());
            break;

        case 0x00: /* INPUT #2 */
            temp = 0xFF;
            if (input.system & INPUT_START) temp &= ~0x80;
            if (sms.country == TYPE_DOMESTIC) temp &= ~0x40;
            return (temp);

        case 0xC0: /* INPUT #0 */
        case 0xDC:
            return (read_port_dc());

        case 0xC1: /* INPUT #1 */
        case 0xDD:
            temp = 0xFF;
            if (input.pad[1] & INPUT_LEFT) temp &= ~0x01;
            if (input.pad[1] & INPUT_RIGHT) temp &= ~0x02;
            if (input.pad[1] & INPUT_BUTTON2) temp &= ~0x04;
            if (input.pad[1] & INPUT_BUTTON1) temp &= ~0x08;
            if (input.system & INPUT_SOFT_RESET) temp &= ~0x10;
            return ((temp & 0x3F) | (sms.port_3F & 0xC0));

        case 0xBE: /* VDP DATA */
            return (vdp_data_r());

        case 0xBD:
        case 0xBF: /* VDP CTRL */
            return (vdp_ctrl_r());

        case 0xF2: /* YM2413 DETECT */
            if (sms.use_fm) return (sms.port_F2);
            break;
    }
    return (0xFF);
}


void sms_mapper_w(int address, int data) {
    /* Calculate ROM page index */
    uint8 page = (data % cart.pages);

    /* Save frame control register data */
    sms.fcr[address] = data;

    switch (address) {
        case 0:
            if (data & 8) {
                sms.save = 1;
                /* Page in ROM */
                cpu_readmap[4] = &sms.sram[(data & 4) ? 0x4000 : 0x0000];
                cpu_readmap[5] = &sms.sram[(data & 4) ? 0x6000 : 0x2000];
                cpu_writemap[4] = &sms.sram[(data & 4) ? 0x4000 : 0x0000];
                cpu_writemap[5] = &sms.sram[(data & 4) ? 0x6000 : 0x2000];
            } else {
                /* Page in RAM */
                cpu_readmap[4] = &cart.rom[((sms.fcr[3] % cart.pages) << 14) + 0x0000];
                cpu_readmap[5] = &cart.rom[((sms.fcr[3] % cart.pages) << 14) + 0x2000];
                cpu_writemap[4] = sms.dummy;
                cpu_writemap[5] = sms.dummy;
            }
            break;

        case 1:
            cpu_readmap[0] = &cart.rom[(page << 14) + 0x0000];
            cpu_readmap[1] = &cart.rom[(page << 14) + 0x2000];
            break;

        case 2:
            cpu_readmap[2] = &cart.rom[(page << 14) + 0x0000];
            cpu_readmap[3] = &cart.rom[(page << 14) + 0x2000];
            break;

        case 3:
            if (!(sms.fcr[0] & 0x08)) {
                cpu_readmap[4] = &cart.rom[(page << 14) + 0x0000];
                cpu_readmap[5] = &cart.rom[(page << 14) + 0x2000];
            }
            break;
    }
}


int sms_irq_callback(int param) {
    return (0xFF);
}

