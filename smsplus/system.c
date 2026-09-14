/*
    Copyright (C) 1998, 1999, 2000  Charles Mac Donald

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "shared.h"
#include "ff.h"
#include <stdbool.h>

t_bitmap bitmap;
t_cart cart;
t_snd snd;
t_input input;

#if PICO_RP2350
OPLL *opll = NULL;
static uint8_t ym2413_regs[0x40] = {0}; /* shadow register file for savestate */
static int fm_active = 0;               /* set on first FM write; gates per-sample OPLL_calc */
static int ym2413_latch = 0;
/* OPLL runs at its native rate (clk/72 ≈ 49716 Hz at MASTER_CLOCK = 3579545)
   so the upstream rate converter is bypassed entirely — that path used
   software-emulated double-precision math (sinc + floor) which alone cost
   ~15% of the 252 MHz frame budget. We resample to 44100 here using a
   Q16 accumulator (zero-order hold; FM is band-limited enough to tolerate it). */
#define FM_NATIVE_RATE  (MASTER_CLOCK / 72)
#define FM_RESAMPLE_STEP_Q16 ((uint32_t)((((uint64_t)FM_NATIVE_RATE) << 16) / SMS_AUD_RATE))
static uint32_t fm_resample_acc = 0;
static int16_t fm_last_sample = 0;
#endif

struct {
    char reg[64];
} ym2413;

void *frens_f_malloc(size_t size);
void frens_f_free(void *ptr);
void ym2413_write(int chip, int offset, int data);
void system_init(int rate) {

    // initialize memory. cart.type is already set by load_rom().
    // The SG-1000 renderer does not use the Mode 4 tile cache, and its
    // cartridge RAM is 8 KB instead of the 32 KB SMS battery RAM.
    if (!IS_SG) {
        cachePtr = (int16 *)frens_f_malloc(512 * 4 * sizeof(int16));
        cacheStore = (uint8 *)frens_f_malloc(CACHEDTILES * 64 * sizeof(uint8));
        cacheStoreUsed = (uint8 *)frens_f_malloc(CACHEDTILES * sizeof(uint8));
        memset(cachePtr, 0, 512 * 4 * sizeof(int16));
        memset(cacheStore, 0, CACHEDTILES * 64 * sizeof(uint8));
        memset(cacheStoreUsed, 0, CACHEDTILES * sizeof(uint8));
    }
    sms.ram  = (uint8 *)frens_f_malloc(RAMSIZEBYTES);
    sms.sram = (uint8 *)frens_f_malloc(SRAM_BYTES);
    /* Initialize the VDP emulation */
    vdp_init();

    /* Initialize the SMS emulation */
    sms_init();

    /* Initialize the look-up tables and related data */
    render_init();

    /* Enable sound emulation if the sample rate was specified */
    system_audio_init(rate);

    /* Don't save SRAM by default */
    sms.save = 0;

    /* Clear emulated button state */
    __builtin_memset(&input, 0, sizeof(t_input));
}

void system_audio_init(int rate) {
    /* Clear sound context */
    __builtin_memset(&snd, 0, sizeof(t_snd));

    /* Reset logging data */
    snd.log = 0;
    snd.callback = NULL;

    /* Oops.. sound is disabled */
    if (!rate) return;

    /* Calculate buffer size in samples */
    snd.bufsize = (rate / 60);

    /* Sound output */
    snd.buffer[0] = (signed short int *) frens_f_malloc(snd.bufsize * 2);
    snd.buffer[1] = (signed short int *) frens_f_malloc(snd.bufsize * 2);
    if (!snd.buffer[0] || !snd.buffer[1]) return;
    __builtin_memset(snd.buffer[0], 0, snd.bufsize * 2);
    __builtin_memset(snd.buffer[1], 0, snd.bufsize * 2);

    /* Set up SN76489 emulation */
    SN76496_init(0, MASTER_CLOCK, 255, rate);

#if PICO_RP2350
    /* Set up YM2413 emulation (RP2350 only). Pass FM_NATIVE_RATE so upstream
       skips its expensive sinc rate converter; we resample in system_mix_fm. */
    opll = OPLL_new(MASTER_CLOCK, FM_NATIVE_RATE);
    if (opll) {
        OPLL_resetPatch(opll, OPLL_2413_TONE);
        OPLL_reset(opll);
    }
    fm_active = 0;
    ym2413_latch = 0;
    fm_resample_acc = 0;
    fm_last_sample = 0;
    __builtin_memset(ym2413_regs, 0, sizeof(ym2413_regs));
#endif

    /* Inform other functions that we can use sound */
    snd.enabled = 1;
}

void system_shutdown(void)
{
    // free memory
    printf("Freeing memory allocated to emulator.\n");
    frens_f_free(snd.buffer[1]);
    frens_f_free(snd.buffer[0]);
    frens_f_free(cacheStoreUsed);
    frens_f_free(cacheStore);
    frens_f_free(cachePtr);
    frens_f_free(sms.sram);
    frens_f_free(sms.ram);
    snd.buffer[0] = NULL;
    snd.buffer[1] = NULL;
    cacheStoreUsed = NULL;
    cacheStore = NULL;
    cachePtr = NULL;
    sms.sram = NULL;
    sms.ram = NULL;

    if (snd.enabled)
    {
#if PICO_RP2350
        if (opll) {
            OPLL_delete(opll);
            opll = NULL;
        }
        fm_active = 0;
#endif
    }
}

void system_reset(void) {
   
    
    cpu_reset();
    vdp_reset();
    sms_reset();
    render_reset();
    if (!IS_SG) system_load_sram();
    if (snd.enabled) {
#if PICO_RP2350
        if (opll) {
            OPLL_reset(opll);
            OPLL_resetPatch(opll, OPLL_2413_TONE);
        }
        fm_active = 0;
        ym2413_latch = 0;
        fm_resample_acc = 0;
        fm_last_sample = 0;
        __builtin_memset(ym2413_regs, 0, sizeof(ym2413_regs));
#endif
    }
}


bool system_save_state(FIL *fd) {
    UINT bw;
    FRESULT fr;
    /* Save VDP context */
    fr = f_write(fd, &vdp, sizeof(t_vdp), &bw);
    if (fr != FR_OK || bw != sizeof(t_vdp)) {
        printf("Error writing VDP context: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)sizeof(t_vdp));
        return false;
    }

    /* Save SMS context */
    fr = f_write(fd, &sms, sizeof(t_sms), &bw);
    if (fr != FR_OK || bw != sizeof(t_sms)) {
        printf("Error writing SMS context: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)sizeof(t_sms));
        return false;
    }

    /* Save SMS work RAM contents (sms.ram is a pointer; struct above only stores the address) */
    fr = f_write(fd, sms.ram, RAMSIZEBYTES, &bw);
    if (fr != FR_OK || bw != RAMSIZEBYTES) {
        printf("Error writing SMS RAM: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)RAMSIZEBYTES);
        return false;
    }

    /* Save SMS SRAM contents (SG-1000: the smaller cartridge RAM) */
    fr = f_write(fd, sms.sram, SRAM_BYTES, &bw);
    if (fr != FR_OK || bw != SRAM_BYTES) {
        printf("Error writing SMS SRAM: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)SRAM_BYTES);
        return false;
    }

    /* Save Z80 context */
    fr = f_write(fd, Z80_Context, sizeof(Z80_Regs), &bw);
    if (fr != FR_OK || bw != sizeof(Z80_Regs)) {
        printf("Error writing Z80 regs: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)sizeof(Z80_Regs));
        return false;
    }
    fr = f_write(fd, &after_EI, sizeof(int), &bw);
    if (fr != FR_OK || bw != sizeof(int)) {
        printf("Error writing after_EI: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)sizeof(int));
        return false;
    }

    /* Save YM2413 registers: on RP2350 we save the shadow register file; on
       RP2040 we write 64 zero bytes so the format stays binary-compatible. */
#if PICO_RP2350
    fr = f_write(fd, ym2413_regs, 0x40, &bw);
#else
    fr = f_write(fd, &ym2413.reg[0], 0x40, &bw);
#endif
    if (fr != FR_OK || bw != 0x40) {
        printf("Error writing YM2413 regs: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)0x40);
        return false;
    }

    /* Save SN76489 context */
    fr = f_write(fd, &sn[0], sizeof(t_SN76496), &bw);
    if (fr != FR_OK || bw != sizeof(t_SN76496)) {
        printf("Error writing SN76489 context: fr=%d wrote=%u expected=%u\n", fr, bw, (unsigned)sizeof(t_SN76496));
        return false;
    }

    return true;
}


bool system_load_state(FIL *fd) {
    int i;

    uint8 *reg = (uint8 *)frens_f_malloc(0x40);

    UINT br;
    FRESULT fr;

    /* Initialize everything */
    // cpu_reset();
    system_reset();

    /* Load VDP context */
    fr = f_read(fd, &vdp, sizeof(t_vdp), &br);
    if (fr != FR_OK || br != sizeof(t_vdp)) {
        printf("Error reading VDP context: fr=%d read=%u expected=%u\n", fr, br, (unsigned)sizeof(t_vdp));
        return false;
    }

    /* Load SMS context — preserve runtime pointers across the struct read.
       sms.dummy/ram/sram are pointers into local heap/static memory; the values
       written to the save file at save time are stale and must not be used. */
    uint8 *saved_dummy = sms.dummy;
    uint8 *saved_ram   = sms.ram;
    uint8 *saved_sram  = sms.sram;
    fr = f_read(fd, &sms, sizeof(t_sms), &br);
    sms.dummy = saved_dummy;
    sms.ram   = saved_ram;
    sms.sram  = saved_sram;
    if (fr != FR_OK || br != sizeof(t_sms)) {
        printf("Error reading SMS context: fr=%d read=%u expected=%u\n", fr, br, (unsigned)sizeof(t_sms));
        return false;
    }

    /* Load SMS work RAM contents */
    fr = f_read(fd, sms.ram, RAMSIZEBYTES, &br);
    if (fr != FR_OK || br != RAMSIZEBYTES) {
        printf("Error reading SMS RAM: fr=%d read=%u expected=%u\n", fr, br, (unsigned)RAMSIZEBYTES);
        return false;
    }

    /* Load SMS SRAM contents (SG-1000: the smaller cartridge RAM) */
    fr = f_read(fd, sms.sram, SRAM_BYTES, &br);
    if (fr != FR_OK || br != SRAM_BYTES) {
        printf("Error reading SMS SRAM: fr=%d read=%u expected=%u\n", fr, br, (unsigned)SRAM_BYTES);
        return false;
    }

    /* Load Z80 context */
    fr = f_read(fd, Z80_Context, sizeof(Z80_Regs), &br);
    if (fr != FR_OK || br != sizeof(Z80_Regs)) {
        printf("Error reading Z80 regs: fr=%d read=%u expected=%u\n", fr, br, (unsigned)sizeof(Z80_Regs));
        return false;
    }
    fr = f_read(fd, &after_EI, sizeof(int), &br);
    if (fr != FR_OK || br != sizeof(int)) {
        printf("Error reading after_EI: fr=%d read=%u expected=%u\n", fr, br, (unsigned)sizeof(int));
        return false;
    }

    // not used
    /* Load YM2413 registers */
    fr = f_read(fd, reg, 0x40, &br);
    if (fr != FR_OK || br != 0x40) {
        printf("Error reading YM2413 regs: fr=%d read=%u expected=%u\n", fr, br, (unsigned)0x40);
        return false;
    }
    
    /* Load SN76489 context */
    fr = f_read(fd, &sn[0], sizeof(t_SN76496), &br);
    if (fr != FR_OK || br != sizeof(t_SN76496)) {
        printf("Error reading SN76489 context: fr=%d read=%u expected=%u\n", fr, br, (unsigned)sizeof(t_SN76496));
        return false;
    }

    /* Restore callbacks */
    z80_set_irq_callback(sms_irq_callback);

    if (IS_SG) {
        /* Rebuilds ROM/RAM pages, detected RAM adaptor and mapper paging */
        sg_memory_map();
    } else {
        cpu_readmap[0] = cart.rom + 0x0000; /* 0000-3FFF */
        cpu_readmap[1] = cart.rom + 0x2000;
        cpu_readmap[2] = cart.rom + 0x4000; /* 4000-7FFF */
        cpu_readmap[3] = cart.rom + 0x6000;
        cpu_readmap[4] = cart.rom + 0x0000; /* 0000-3FFF */
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

        sms_mapper_w(3, sms.fcr[3]);
        sms_mapper_w(2, sms.fcr[2]);
        sms_mapper_w(1, sms.fcr[1]);
        sms_mapper_w(0, sms.fcr[0]);
    }

    /* Force full pattern cache update */
//    is_vram_dirty = 1;
//    memset(vram_dirty, 1, 0x200);

    /* Restore palette */
    for (i = 0; i < PALETTE_SIZE; i += 1)
        palette_sync(i);

    /* Restore sound state */
    if (snd.enabled) {
#if PICO_RP2350
        if (opll) {
            OPLL_reset(opll);
            OPLL_resetPatch(opll, OPLL_2413_TONE);

            /* Replay register file in the order the chip expects:
               rhythm flags first, then user instrument, frequency,
               key-on/block, instrument+volume. */
            ym2413_write(0, 0, 0x0E);
            ym2413_write(0, 1, reg[0x0E]);
            for (i = 0x00; i <= 0x07; i += 1) {
                ym2413_write(0, 0, i);
                ym2413_write(0, 1, reg[i]);
            }
            for (i = 0x10; i <= 0x18; i += 1) {
                ym2413_write(0, 0, i);
                ym2413_write(0, 1, reg[i]);
            }
            for (i = 0x20; i <= 0x28; i += 1) {
                ym2413_write(0, 0, i);
                ym2413_write(0, 1, reg[i]);
            }
            for (i = 0x30; i <= 0x38; i += 1) {
                ym2413_write(0, 0, i);
                ym2413_write(0, 1, reg[i]);
            }
        }
#endif
    }
    frens_f_free(reg);
    return true;
}

void in_ram(system_mix_fm)(signed short *left, signed short *right, int n) {
#if PICO_RP2350
    if (!sms.use_fm || !fm_active || !opll) return;
    uint32_t acc = fm_resample_acc;
    int16_t last = fm_last_sample;
    for (int i = 0; i < n; ++i) {
        /* Advance FM by one output sample's worth of native ticks.
           Step is ~49716/44100 ≈ 1.127 in Q16; sometimes 1 call, sometimes 2. */
        acc += FM_RESAMPLE_STEP_Q16;
        while (acc >= (1u << 16)) {
            last = OPLL_calc(opll);
            acc -= (1u << 16);
        }
        int32_t fm = (int32_t)last;
        int32_t l = (int32_t)left[i]  + fm;
        int32_t r = (int32_t)right[i] + fm;
        if (l > 32767) l = 32767; else if (l < -32768) l = -32768;
        if (r > 32767) r = 32767; else if (r < -32768) r = -32768;
        left[i]  = (signed short)l;
        right[i] = (signed short)r;
    }
    fm_resample_acc = acc;
    fm_last_sample = last;
#else
    (void)left; (void)right; (void)n;
#endif
}

void ym2413_write(int chip, int offset, int data) {
#if PICO_RP2350
    if (!opll) return;
    if (offset & 1) {
        /* data write */
        if (ym2413_latch < 0x40) {
            ym2413_regs[ym2413_latch] = (uint8_t)data;
        }
        OPLL_writeReg(opll, (uint32_t)ym2413_latch, (uint8_t)data);
        fm_active = 1;
    } else {
        /* address latch */
        ym2413_latch = data & 0x3F;
    }
#else
    (void)chip; (void)offset; (void)data;
#endif
}




