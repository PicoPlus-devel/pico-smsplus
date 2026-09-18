/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/divider.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/watchdog.h"
#include "util/work_meter.h"
#include "ff.h"
#include "tusb.h"
#include "gamepad.h"
#include "menu.h"
#include "nespad.h"
#include "wiipad.h"
#include "FrensHelpers.h"
#include "settings.h"
#include "FrensFonts.h"
#include "vumeter.h"
#include "shared.h"
#include "mytypes.h"
#include "PicoPlusPsram.h"
#include "menu_settings.h"
#include "state.h"
#include "soundrecorder.h"

bool isFatalError = false;
static FATFS fs;
char *romName;
bool showSettings = false;
bool loadSaveStateMenu = false;
SaveStateTypes quickSaveAction = SaveStateTypes::NONE;
static int cartType = TYPE_SMS; // TYPE_SMS, TYPE_GG or TYPE_SG
static uint32_t start_tick_us = 0;
static uint32_t fps = 0;
static uint8_t framesbeforeAutoStateIsLoaded = 0;
static char fpsString[3] = "00";
#if PICO_RP2350
extern const unsigned char EmuOverlay_444[];
extern const unsigned char EmuOverlay_555[];
#endif
// DVI Note: When using framebuffer or render audio per frame, AUDIOBUFFERSIZE must be increased to 1024
// #if PICO_RP2350
// #define AUDIOBUFFERSIZE 1024
// #else
// #define AUDIOBUFFERSIZE 512
// #endif
#define AUDIOBUFFERSIZE 1024

#define EMULATOR_CLOCKFREQ_KHZ 252000 //  Overclock frequency in kHz when using Emulator
static uint32_t CPUFreqKHz = EMULATOR_CLOCKFREQ_KHZ;
// Visibility configuration for options menu
// 1 = show option line, 0 = hide, -1 = always hidden.
// Designated initializers, like g_settings_descriptions in menu_settings.h, so
// each value stays bound to its enum tag; options not listed are hidden (0).
const int8_t g_settings_visibility_sms[MOPT_COUNT] = {
    [MOPT_EXIT_GAME]                 = 0,  // Always visible when in-game.
    [MOPT_RESET_GAME]                = 0,  // Always visible when in-game.
    [MOPT_REBOOT_TO_LOADER]          = BOOTLOADER_BUILD, // Only when built for the loader
    [MOPT_SAVE_RESTORE_STATE]        = 0,  // Always visible when in-game.
    [MOPT_SCREENMODE]                = 1,
    [MOPT_SCANLINES]                 = 0,  // Superseded by Screen Mode
    [MOPT_SCANLINE_TYPE]             = HSTX,
    [MOPT_FPS_OVERLAY]               = 1,
    [MOPT_AUDIO_ENABLE]              = 0,
    [MOPT_FRAMESKIP]                 = 0,
    [MOPT_DISPLAY_MODE]              = HSTX && ENABLEDVI, // non-HSTX builds always use HDMI
    [MOPT_EXTERNAL_AUDIO]            = (EXT_AUDIO_IS_ENABLED),
    [MOPT_FONT_COLOR]                = 1,
    [MOPT_FONT_BACK_COLOR]           = 1,
    [MOPT_FRUITJAM_VUMETER]          = ENABLE_VU_METER,
    [MOPT_FRUITJAM_VOLUME_CONTROL]   = (HW_CONFIG == 8),
    [MOPT_DMG_PALETTE]               = 0,  // Game Boy only
    [MOPT_BORDER_MODE]               = 0,  // Game Boy only
    [MOPT_RAPID_FIRE_ON_A]           = 0,
    [MOPT_RAPID_FIRE_ON_B]           = 0,
    [MOPT_AUTO_INSERT_FDS_DISK_A]    = 0,  // FDS (NES) only
    [MOPT_AUTO_SWAP_FDS_DISK]        = 0,  // FDS (NES) only
    [MOPT_FDS_DISK_SWAP]             = 0,  // FDS (NES) only
    [MOPT_OVERCLOCK]                 = 0,
    [MOPT_FM_AUDIO]                  = HSTX, // YM2413 FM, RP2350 with HSTX only
    [MOPT_ENTER_BOOTSEL_MODE]        = 1,
    [MOPT_CONTROLLER_TEST]           = 1,
    [MOPT_RECENT_GAMES]              = 1,  // Rom browser only; menu.cpp gates in-game
    [MOPT_USB_DRIVE_MODE]            = 0,  // USB drive mode (menu.cpp force-shows this in the rom browser)
};
const uint8_t g_available_screen_modes_sms[] = {
#if PICO_RP2350
    0, // SCANLINE_8_7, Does not work well with borders
#else
    1, // SCANLINE_8_7,
#endif
#if PICO_RP2350
    0, // NOSCANLINE_8_7, Does not work well with borders
#else
    1, // NOSCANLINE_8_7,
#endif
    1, // SCANLINE_1_1,
    1  // NOSCANLINE_1_1
};

#define fpsfgcolor 0;     // black
#define fpsbgcolor 0xFFF; // white

#define MARGINTOP 24
#define MARGINBOTTOM 4

#define FPSSTART (((MARGINTOP + 7) / 8) * 8)
#define FPSEND ((FPSSTART) + 8)

static bool reset = false;
static bool resetGame = false;

#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
// Cached Wii pad state updated once per frame in ProcessAfterFrameIsRendered()
static uint16_t wiipad_raw_cached = 0;
#endif

// The Sega Master system color palette converted to RGB444
// so it can be used with the DVI library.
// from https://segaretro.org/Palette
#if 0
#if !HSTX
// RGB444 format
WORD SMSPaletteRGB[64] = {
    0x0, 0x500, 0xA00, 0xF00, 0x50, 0x550, 0xA50, 0xF50,
    0xA0, 0x5A0, 0xAA0, 0xFA0, 0xF0, 0x5F0, 0xAF0, 0xFF0,
    0x5, 0x505, 0xA05, 0xF05, 0x55, 0x555, 0xA55, 0xF55,
    0xA5, 0x5A5, 0xAA5, 0xFA5, 0xF5, 0x5F5, 0xAF5, 0xFF5,
    0xA, 0x50A, 0xA0A, 0xF0A, 0x5A, 0x55A, 0xA5A, 0xF5A,
    0xAA, 0x5AA, 0xAAA, 0xFAA, 0xFA, 0x5FA, 0xAFA, 0xFFA,
    0xF, 0x50F, 0xA0F, 0xF0F, 0x5F, 0x55F, 0xA5F, 0xF5F,
    0xAF, 0x5AF, 0xAAF, 0xFAF, 0xFF, 0x5FF, 0xAFF, 0xFFF};
#else
// RGB555 format
WORD SMSPaletteRGB[64] = {
    0x0, 0x500, 0xA00, 0xF00, 0x50, 0x550, 0xA50, 0xF50,
    0xA0, 0x5A0, 0xAA0, 0xFA0, 0xF0, 0x5F0, 0xAF0, 0xFF0,
    0x5, 0x505, 0xA05, 0xF05, 0x55, 0x555, 0xA55, 0xF55,
    0xA5, 0x5A5, 0xAA5, 0xFA5, 0xF5, 0x5F5, 0xAF5, 0xFF5,
    0xA, 0x50A, 0xA0A, 0xF0A, 0x5A, 0x55A, 0xA5A, 0xF5A,
    0xAA, 0x5AA, 0xAAA, 0xFAA, 0xFA, 0x5FA, 0xAFA, 0xFFA,
    0xF, 0x50F, 0xA0F, 0xF0F, 0x5F, 0x55F, 0xA5F, 0xF5F,
    0xAF, 0x5AF, 0xAAF, 0xFAF, 0xFF, 0x5FF, 0xAFF, 0xFFF};
#endif // HSTX
#endif

#define HEADER_OFFSET 0x7FF0
#define HEADER_SIZE 16

struct SegaHeader
{
    // 0x7FF0
    char signature[8];
    // 0x7FF8
    uint16_t reserverd;
    // 0x7FFA
    uint16_t checksum;
    // 0x7FFC
    uint8_t product_code[2];
    // 0x7FFE
    uint8_t ProductCodeAndVersion;
    // 0x7FFF
    uint8_t sizeAndRegion;
} *header;
bool detect_rom_type_from_memory(uintptr_t addr, int *size, bool *isGameGear)
{
    char *rom = (char *)addr;
    int actualSize = 0;
    header = (SegaHeader *)(rom + HEADER_OFFSET);
    if (strncmp(header->signature, "TMR SEGA", 8) == 0)
    {
        printf("Sega header found\n");

        uint8_t romsize = header->sizeAndRegion & 0b00001111;
        uint8_t region = (header->sizeAndRegion >> 4) & 0b00001111;
        // https://www.smspower.org/Development/ROMHeader
        switch (romsize)
        {
        case 0:                 // 256KB
            *size = 512 * 1024; // 512KB and 1MB Roms are reported in the header as 256KB.
                                // Setting Rom size to 512KB also works for 256KB roms.
            break;              // Setting rom size to 1MB for 256 or 512KB games does not work.
                                // Only a small set of roms are 1MB.
        case 1:
            *size = 512 * 1024;
            break;
        case 2:
            *size = 1024 * 1024;
            break;
        case 0xa:
            *size = 8 * 1024;
            break;
        case 0xb:
            *size = 16 * 1024;
            break;
        case 0xc:
            *size = 32 * 1024;
            break;
        case 0xd:
            *size = 48 * 1024;
            break;
        case 0xe:
            *size = 64 * 1024;
            break;
        case 0xf:
            *size = 128 * 1024;
            break;
        default:
            printf("Unknown romsize %x\n", romsize);
            *size = 0; // unknown size
            break;
        }
        printf("Romsize %x = %d bytes\n", romsize, *size);
#if PICO_RP2350 && PSRAM_CS_PIN
        // If PSRAM is used, get the actual size of the allocated block to determine the actual size of the rom.
        if (Frens::isPsramEnabled())
        {
            printf("PSRAM enabled, getting size of allocated block\n");
            PicoPlusPsram &psram_ = PicoPlusPsram::getInstance();
            printf("Size in rom: %d bytes, ", *size);
            *size = psram_.GetSize((void *)rom);
            printf("actual size in PSRAM: %d bytes\n", *size);
            printf("Setting rom size to %d bytes\n", *size);
        }
#endif

        *isGameGear = false;

        printf("Region: %x - ", region);
        switch (region)
        {
        case 3:
            printf("SMS Japan\n");
            *isGameGear = false;
            break;
        case 4:
            printf("SMS Export\n");
            *isGameGear = false;
            break;
        case 5:
            printf("GG USA\n");
            *isGameGear = true;
            break;
        case 6:
            printf("GG Export\n");
            *isGameGear = true;
            break;
        case 7:
            printf("GG International\n");
            *isGameGear = true;
            break;
        default:
            printf("Unknown\n");
            break;
        }
    }
    if (*size > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/* One-pole DC-blocker (R = 1 - 1/512, ~14 Hz cutoff). PSG-only output is
   unsigned 0..0x7FFF; PSG+FM mix is signed int16. Sign-extend raw_l/r
   directly — an unsigned cast wraps negative FM peaks to large positives
   and corrupts the filter state with spurious step inputs. The clamp at
   the end catches the rare PSG+FM peaks that exceed int16. */
static inline void psg_postprocess(short raw_l, short raw_r,
                                   short *out_l, short *out_r)
{
    static int32_t prev_l = 0, prev_r = 0;
    static int32_t dc_l   = 0, dc_r   = 0;
    int32_t xl = (int32_t)raw_l;
    int32_t xr = (int32_t)raw_r;
    int32_t yl = xl - prev_l + dc_l - (dc_l >> 9);
    int32_t yr = xr - prev_r + dc_r - (dc_r >> 9);
    prev_l = xl; prev_r = xr;
    dc_l   = yl; dc_r   = yr;
    /* +3.5 dB makeup gain (×1.5). PSG-only peaks land near -2.5 dBFS,
       FM-mixed peaks near -3.5 dBFS — comfortable below the int16 ceiling
       so the clamp rarely fires in practice. */
    yl = yl + (yl >> 1);
    yr = yr + (yr >> 1);
    if (yl > 32767) yl = 32767; else if (yl < -32768) yl = -32768;
    if (yr > 32767) yr = 32767; else if (yr < -32768) yr = -32768;
    *out_l = (short)yl;
    *out_r = (short)yr;
}

#if !HSTX
#define DVILOGDROPPEDSAMPLES 0
static void inline processaudioPerFrameDVI()
{
    auto &ring = dvi_->getAudioRingBuffer();
    int totalSamples = snd.bufsize;
    int written = 0;

    while (written < totalSamples)
    {
        int n = std::min<int>(totalSamples - written, ring.getWritableSize());
        if (n == 0)
        {
#if DVILOGDROPPEDSAMPLES
            static int dropped = 0;
            dropped += (totalSamples - written);
            if (dropped % 100 == 0)
            {
                printf("DVI audio buffer full, dropping samples! Total dropped: %d\n", dropped);
            }
#endif
            break; // Buffer full, can't write more
        }
        auto p = ring.getWritePointer();
        for (int i = 0; i < n; ++i)
        {
            short l = snd.buffer[0][written + i];
            short r = snd.buffer[1][written + i];
            short ol, or_;
            psg_postprocess(l, r, &ol, &or_);
            *p++ = {ol, or_};
        }
        ring.advanceWritePointer(n);
        written += n;
    }
}
#else // !HSTX
static void inline processaudioPerFrameHSTX() {
     for (int i = 0; i < snd.bufsize; i++)
    {
        short l = snd.buffer[0][i];
        short r = snd.buffer[1][i];
        short ol, or_;
        psg_postprocess(l, r, &ol, &or_);
        hstx_push_audio_sample(ol, or_);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(ol);
        }
#endif
    }
}
#endif
static void inline processaudioPerFrameI2S()
{
    for (int i = 0; i < snd.bufsize; i++)
    {
        short l = snd.buffer[0][i];
        short r = snd.buffer[1][i];
        short ol, or_;
        psg_postprocess(l, r, &ol, &or_);
        /* I2S DAC feeds an amp/headphone directly with no downstream volume
           control, and the SMS+FM mix peaks much hotter than the NES mixer
           pico_shared was tuned for — cut -6 dB here. */
        ol = (short)(ol >> 1);
        or_ = (short)(or_ >> 1);
        EXT_AUDIO_ENQUEUE_SAMPLE(ol, or_);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(ol);
        }
#endif
    }
}
// Don't process audio per line, but per frame
#if 0
int sampleIndex = 0;
void in_ram(processaudio)(int offset)
{
    int samples = 4; // 735/192 = 3.828125 192*4=768 735/3=245
    if (offset == (IS_GG ? 24 : 0))
    {
        sampleIndex = 0;
    }
    else
    {
        sampleIndex += samples;
        if (sampleIndex >= 735)
        {
            return;
        }
    }
    short *p1 = snd.buffer[0] + sampleIndex;
    short *p2 = snd.buffer[1] + sampleIndex;
#if !HSTX
    while (samples)
    {
        auto &ring = dvi_->getAudioRingBuffer();
        auto n = std::min<int>(samples, ring.getWritableSize());
        if (!n)
        {
            return;
        }
        auto p = ring.getWritePointer();
        int ct = n;
        while (ct--)
        {
            int l = *p1++; // (*p1++ << 16) + *p2++;
            // works also : int l = (*p1++ + *p2++) / 2;
            int r = *p2++;
            // int l = *wave1++;
#if EXT_AUDIO_IS_ENABLED
            if (settings.flags.useExtAudio)
            {
                // uint32_t sample32 = (l << 16) | (r & 0xFFFF);
                EXT_AUDIO_ENQUEUE_SAMPLE(l, r);
            }
            else
            {
                *p++ = {static_cast<short>(l), static_cast<short>(r)};
            }
#else
            *p++ = {static_cast<short>(l), static_cast<short>(r)};
#endif
        }
#if EXT_AUDIO_IS_ENABLED
        if (!settings.flags.useExtAudio)
        {
            ring.advanceWritePointer(n);
        }
#else
        ring.advanceWritePointer(n);
#endif
        samples -= n;
    }
#else
    for (int i = 0; i < samples; i++)
    {
        int l = *p1++; // (*p1++ << 16) + *p2++;
        // works also : int l = (*p1++ + *p2++) / 2;
        int r = *p2++;
        // int l = *wave1++;
        // int r = l;
        EXT_AUDIO_ENQUEUE_SAMPLE(l, r);
#if ENABLE_VU_METER
        if (settings.flags.enableVUMeter)
        {
            addSampleToVUMeter(l);
        }
#endif
    }
#endif
}
#endif

extern "C" void in_ram(sms_palette_syncGG)(int index)
{
    // The GG has a different palette format
    int r = ((vdp.cram[(index << 1) | 0] >> 1) & 7) << 5;
    int g = ((vdp.cram[(index << 1) | 0] >> 5) & 7) << 5;
    int b = ((vdp.cram[(index << 1) | 1] >> 1) & 7) << 5;
#if !HSTX
    int r444 = ((r << 4) + 127) >> 8; // equivalent to (r888 * 15 + 127) / 255
    int g444 = ((g << 4) + 127) >> 8; // equivalent to (g888 * 15 + 127) / 255
    int b444 = ((b << 4) + 127) >> 8;
    palette444[index] = (r444 << 8) | (g444 << 4) | b444;
#else
    int r555 = ((r << 5) + 127) >> 8; // equivalent to (r888 * 31 + 127) / 255
    int g555 = ((g << 5) + 127) >> 8; // equivalent to (g888 * 31 + 127) / 255
    int b555 = ((b << 5) + 127) >> 8;

    palette444[index] = (r555 << 10) | (g555 << 5) | b555;
#endif
    return;
}

extern "C" void in_ram(sms_palette_sync)(int index)
{
#if 0
    // Get SMS palette color index from CRAM
    WORD r = ((vdp.cram[index] >> 0) & 3);
    WORD g = ((vdp.cram[index] >> 2) & 3);
    WORD b = ((vdp.cram[index] >> 4) & 3);
    WORD tableIndex = b << 4 | g << 2 | r;
    // Get the RGB444 color from the SMS RGB444 palette
    palette444[index] = SMSPaletteRGB[tableIndex];
#endif
#if 1
    // Alternative color rendering below
    WORD r = ((vdp.cram[index] >> 0) & 3) << 6;
    WORD g = ((vdp.cram[index] >> 2) & 3) << 6;
    WORD b = ((vdp.cram[index] >> 4) & 3) << 6;

#if !HSTX
    int r444 = ((r << 4) + 127) >> 8; // equivalent to (r888 * 15 + 127) / 255
    int g444 = ((g << 4) + 127) >> 8; // equivalent to (g888 * 15 + 127) / 255
    int b444 = ((b << 4) + 127) >> 8;
    palette444[index] = (r444 << 8) | (g444 << 4) | b444;
#else
    int r555 = ((r << 5) + 127) >> 8; // equivalent to (r888 * 31 + 127) / 255
    int g555 = ((g << 5) + 127) >> 8; // equivalent to (g888 * 31 + 127) / 255
    int b555 = ((b << 5) + 127) >> 8;
    palette444[index] = (r555 << 10) | (g555 << 5) | b555;
#endif
#endif
    return;
}

// SG-1000: fixed TMS9918A palette, set once at reset.
// The TMS palette uses channel values up to 255, so scale by 15 (31) rather than
// 16 (32): the conversion used for SMS/GG above overflows for values >= 249
// (>= 253 for 555) and would turn white into near black.
extern "C" void sms_palette_syncSG(int index)
{
    const uint8 *rgb = tms_palette_rgb[index & 15];
    int r = rgb[0];
    int g = rgb[1];
    int b = rgb[2];
#if !HSTX
    int r444 = ((r << 4) - r + 127) >> 8; // (r888 * 15 + 127) / 256, max 15
    int g444 = ((g << 4) - g + 127) >> 8;
    int b444 = ((b << 4) - b + 127) >> 8;
    palette444[index] = (r444 << 8) | (g444 << 4) | b444;
#else
    int r555 = ((r << 5) - r + 127) >> 8; // (r888 * 31 + 127) / 256, max 31
    int g555 = ((g << 5) - g + 127) >> 8;
    int b555 = ((b << 5) - b + 127) >> 8;
    palette444[index] = (r555 << 10) | (g555 << 5) | b555;
#endif
}

extern "C" void in_ram(sms_render_line)(int line, const uint8_t *buffer)
{
    // DVI top margin has #MARGINTOP lines
    // DVI bottom margin has #MARGINBOTTOM lines
    // DVI usable screen estate: MARGINTOP .. (240 - #MARGINBOTTOM)
    // SMS has 192 lines
    // GG  has 144 lines
    // gg : Line starts at line 24
    // sms: Line starts at line 0
    // Emulator loops from scanline 0 to 261
    // Audio processing is per frame, not per line
#if 0
#if !HSTX
#if EXT_AUDIO_IS_ENABLED
    // i2s_audio will be output per frame if external audio is enabled
    if (settings.flags.useExtAudio == 0)
    {
        processaudio(line);
    }
#else
    processaudio(line);
#endif
#endif
#endif
    // Adjust line number to center the emulator display
    line += MARGINTOP;
    // Only render lines that are visible on the screen, keeping into account top and bottom margins
    if (line < MARGINTOP || line >= 240 - MARGINBOTTOM)
        return;
    uint16_t *sbuffer;
    uint16_t *currentLineBuf = nullptr;
#if !HSTX
    dvi::DVI::LineBuffer *b{};
#if FRAMEBUFFERISPOSSIBLE
    if (Frens::isFrameBufferUsed())
    {
        currentLineBuf = &Frens::framebuffer[line * 320];
        sbuffer = currentLineBuf + 32 + (IS_GG ? 48 : 0);
        if (buffer)
        {
            for (int i = screenCropX ; i < BMP_WIDTH - screenCropX; i++)
            {
                sbuffer[i - screenCropX] = palette444[(buffer[i + BMP_X_OFFSET]) & 31];
            }
        }
    }
    else
    {
#endif
        b = dvi_->getLineBuffer();
        currentLineBuf = b->data();
        if (buffer)
        {
            uint16_t *sbuffer = b->data() + 32 + (IS_GG ? 48 : 0);
            for (int i = screenCropX; i < BMP_WIDTH - screenCropX; i++)
            {
                sbuffer[i - screenCropX] = palette444[(buffer[i + BMP_X_OFFSET]) & 31];
            }
        }
        else
        {
            sbuffer = b->data() + 32;
            __builtin_memset(sbuffer, 0, 512);
        }
#else
    currentLineBuf = hstx_getlineFromFramebuffer(line);
    sbuffer = currentLineBuf + 32 + (IS_GG ? 48 : 0);
    if (buffer)
    {
        for (int i = screenCropX ; i < BMP_WIDTH - screenCropX; i++)
        {
            sbuffer[i - screenCropX] = palette444[(buffer[i + BMP_X_OFFSET]) & 31];
        }
    }
#endif
#if !HSTX
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#endif
    // Display frame rate
    if (settings.flags.displayFrameRate && line >= FPSSTART && line < FPSEND)
    {
        WORD *fpsBuffer = currentLineBuf + 40;
        int rowInChar = line % 8;
        for (auto i = 0; i < 2; i++)
        {
            char firstFpsDigit = fpsString[i];
            char fontSlice = getcharslicefrom8x8font(firstFpsDigit, rowInChar);
            for (auto bit = 0; bit < 8; bit++)
            {
                if (fontSlice & 1)
                {
                    *fpsBuffer++ = fpsfgcolor;
                }
                else
                {
                    *fpsBuffer++ = fpsbgcolor;
                }
                fontSlice >>= 1;
            }
        }
    }

#if !HSTX
#if FRAMEBUFFERISPOSSIBLE
    if (!Frens::isFrameBufferUsed())
    {
#endif
        assert(currentLineBuffer_);
        dvi_->setLineBuffer(line, b);
#if FRAMEBUFFERISPOSSIBLE
    }
#endif
#endif
    return;
}

void system_load_sram(void)
{
    char pad[FF_MAX_LFN];
    FILINFO fno;
    char fileName[FF_MAX_LFN];
    strcpy(fileName, Frens::GetfileNameFromFullPath(romName));
    Frens::stripextensionfromfilename(fileName);

    snprintf(pad, FF_MAX_LFN, "%s/%s.SAV", GAMESAVEDIR, fileName);

    FIL fil;
    FRESULT fr;

    size_t bytesRead;
    if (!sms.sram)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "sms.sram is NULL, cannot load SRAM");
        printf("%s\n", ErrorMessage);
        return;
    }
    printf("Load SRAM from %s\n", pad);
    fr = f_stat(pad, &fno);
    if (fr == FR_NO_FILE)
    {
        printf("No save file found, skipping load.\n");
        return;
    }
    if (fr != FR_OK)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "f_stat() failed on save file: %d", fr);
        printf("%s\n", ErrorMessage);
        return;
    }
    if (fno.fsize != SRAMSIZEBYTES)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Save file size mismatch: %d != %d", fno.fsize, SRAMSIZEBYTES);
        printf("%s\n", ErrorMessage);
        return;
    }

    printf("Loading save file %s\n", pad);
    fr = f_open(&fil, pad, FA_READ);
    if (fr == FR_OK)
    {
        fr = f_read(&fil, sms.sram, SRAMSIZEBYTES, &bytesRead);
        if (fr == FR_OK)
        {
            printf("Savefile read from disk %d bytes\n", bytesRead);
            sms.save = 1;
        }
        else
        {
            snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot read save file: %d %d/%d read", fr, bytesRead, SRAMSIZEBYTES);
            printf("%s\n", ErrorMessage);
        }
    }
    else
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot open save file: %d", fr);
        printf("%s\n", ErrorMessage);
    }
    f_close(&fil);
}

void system_save_sram()
{
    char pad[FF_MAX_LFN];
    char fileName[FF_MAX_LFN];
    printf("system_save_sram: Saving SRAM\n");
    strcpy(fileName, Frens::GetfileNameFromFullPath(romName));
    Frens::stripextensionfromfilename(fileName);
    // SG-1000 carts have no battery RAM; sms.sram is smaller working RAM there
    if (!sms.save || IS_SG)
    {
        printf("SRAM not updated.\n");
        return;
    }
    snprintf(pad, FF_MAX_LFN, "%s/%s.SAV", GAMESAVEDIR, fileName);
    printf("Save SRAM to %s\n", pad);
    FIL fil;
    FRESULT fr;
    fr = f_open(&fil, pad, FA_CREATE_ALWAYS | FA_WRITE);
    if (fr != FR_OK)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Cannot open save file: %d", fr);
        printf("%s\n", ErrorMessage);
        return;
    }
    size_t bytesWritten;
    fr = f_write(&fil, sms.sram, SRAMSIZEBYTES, &bytesWritten);
    if (bytesWritten < SRAMSIZEBYTES)
    {
        snprintf(ErrorMessage, ERRORMESSAGESIZE, "Error writing save: %d %d/%d written", fr, bytesWritten, SRAMSIZEBYTES);
        printf("%s\n", ErrorMessage);
    }
    f_close(&fil);
    printf("done\n");
}

void loadoverlay()
{
#if PICO_RP2350
    if (!Frens::isFrameBufferUsed())
    {
        return;
    }
    char CRC[9];
    static const char *borderdirs = "ABCDEFGHIJKLMNOPQRSTUVWY";
    static char PATH[FF_MAX_LFN + 1];
    static char CHOSEN[FF_MAX_LFN + 1];
    // only Game Gear has default overlay
    char *overlay = (cartType == TYPE_GG) ?
#if !HSTX
                               (char *)EmuOverlay_444
                               :
#else
                               (char *)EmuOverlay_555
                               :
#endif
                               nullptr;
    // int fldIndex;
    // if (settings.flags.borderMode == DEFAULTBORDER)
    // {

    //     Frens::loadOverLay(nullptr, overlay);
    //     return;
    // }

    // if (settings.flags.borderMode == THEMEDBORDER)
    // {
    snprintf(CRC, sizeof(CRC), "%08X", Frens::getCrcOfLoadedRom());
    snprintf(CHOSEN, (FF_MAX_LFN + 1) * sizeof(char), "/metadata/SMS/Images/Bezels/%c/%s%s", CRC[0], CRC, FILEXTFORSEARCH);
    printf("Loading bezel: %s\n", CHOSEN);
    //}
    // else
    // {
    //     fldIndex = (rand() % strlen(borderdirs));
    //     snprintf(PATH, (FF_MAX_LFN + 1) * sizeof(char), "/metadata/SMS/Images/Borders/%c", borderdirs[fldIndex]);
    //     printf("Scanning random folder: %s\n", PATH);
    //     FRESULT fr = Frens::pick_random_file_fullpath(PATH, CHOSEN, (FF_MAX_LFN + 1) * sizeof(char));
    //     if (fr != FR_OK)
    //     {
    //         printf("Failed to pick random file from %s: %d\n", PATH, fr);
    //         Frens::loadOverLay(nullptr, overlay);
    //         return;
    //     }
    // }
    Frens::loadOverLay(CHOSEN, overlay);
#endif
}

static inline int ProcessAfterFrameIsRendered()
{
    Frens::pollHeadPhoneJack();
    Frens::PaceFrames60fps(false);
    //Frens::waitForVSync();
#if NES_PIN_CLK != -1
    nespad_read_start();
#endif
    auto count =
#if !HSTX
        dvi_->getFrameCounter();
#else
        hstx_getframecounter();
#endif
    auto onOff = hw_divider_s32_quotient_inlined(count, 60) & 1;
    Frens::blinkLed(onOff);
#if NES_PIN_CLK != -1
    nespad_read_finish(); // Sets global nespad_state var
#endif
    // nespad_read_finish(); // Sets global nespad_state var
    tuh_task();
    // Frame rate calculation
    if (settings.flags.displayFrameRate)
    {
        // calculate fps and round to nearest value (instead of truncating/floor)
        uint32_t tick_us = Frens::time_us() - start_tick_us;
        fps = (1000000 - 1) / tick_us + 1;
        start_tick_us = Frens::time_us();
        fpsString[0] = '0' + (fps / 10);
        fpsString[1] = '0' + (fps % 10);
    }
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
    // Poll Wii pad once per frame (function called once per rendered frame)
    wiipad_raw_cached = wiipad_read();
#endif
#if ENABLE_VU_METER
    if (isVUMeterToggleButtonPressed())
    {
        settings.flags.enableVUMeter = !settings.flags.enableVUMeter;
        // FrensSettings::savesettings();
        // printf("VU Meter %s\n", settings.flags.enableVUMeter ? "enabled" : "disabled");
        turnOffAllLeds();
    }
#endif
    if (showSettings)
    {
        showSettings = false;
        int rval = showSettingsMenu(true);
        if (rval == 3)
        {
            reset = true;
            if (isAutoSaveStateConfigured())
            {
                loadSaveStateMenu = true;
                quickSaveAction = SaveStateTypes::SAVE_AND_EXIT;
            }
        }
        if (rval == 4)
        {
            loadSaveStateMenu = true;
            quickSaveAction = SaveStateTypes::NONE;
        }
        if ( rval == 5)
        {
            reset = resetGame = true;
        }
        loadoverlay();
    }
    if (loadSaveStateMenu)
    {
        if (quickSaveAction == SaveStateTypes::LOAD_AND_START)
        {
            if (framesbeforeAutoStateIsLoaded > 0)
            {
                --framesbeforeAutoStateIsLoaded; // let the emulator run for a few frames before loading state
            }
        }
        else
        {
            framesbeforeAutoStateIsLoaded = 0;
        }
        if (framesbeforeAutoStateIsLoaded == 0)
        {

            char msg[24];
            snprintf(msg, sizeof(msg), "CRC %08X", Frens::getCrcOfLoadedRom());
            if (showSaveStateMenu(Emulator_SaveState, Emulator_LoadState, msg, quickSaveAction) == false)
            {
                reset = true;
            };
            loadSaveStateMenu = false;
             loadoverlay();
        }
       
    }
    return count;
}

static DWORD prevButtons[2]{};
static DWORD prevButtonssystem[2]{};
static DWORD prevOtherButtons[2]{};

#define OTHER_BUTTON1 (0b1)
#define OTHER_BUTTON2 (0b10)

#define NESPAD_SELECT (0x04)
#define NESPAD_START (0x08)
#define NESPAD_UP (0x10)
#define NESPAD_DOWN (0x20)
#define NESPAD_LEFT (0x40)
#define NESPAD_RIGHT (0x80)
#define NESPAD_A (0x01)
#define NESPAD_B (0x02)

void processinput(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem, bool ignorepushed, char *gamepadType)
{
    // pwdPad1 and pwdPad2 are only used in menu and are only set on first push
    *pdwPad1 = *pdwPad2 = *pdwSystem = 0;

    int smssystem[2]{};
    unsigned long pushed, pushedsystem, pushedother;
    bool usbConnected = false;
    for (int i = 0; i < 2; i++)
    {
        int nespadbuttons = 0;
        auto &dst = (i == 0) ? *pdwPad1 : *pdwPad2;
        auto &gp = io::getCurrentGamePadState(i);
        if (i == 0)
        {
            usbConnected = gp.isConnected();
            if (gamepadType)
            {
                strcpy(gamepadType, gp.GamePadName);
            }
        }
        int smsbuttons = (gp.buttons & io::GamePadState::Button::LEFT ? INPUT_LEFT : 0) |
                         (gp.buttons & io::GamePadState::Button::RIGHT ? INPUT_RIGHT : 0) |
                         (gp.buttons & io::GamePadState::Button::UP ? INPUT_UP : 0) |
                         (gp.buttons & io::GamePadState::Button::DOWN ? INPUT_DOWN : 0) |
                         (gp.buttons & io::GamePadState::Button::A ? INPUT_BUTTON1 : 0) |
                         (gp.buttons & io::GamePadState::Button::B ? INPUT_BUTTON2 : 0) | 0;
        int otherButtons = (gp.buttons & io::GamePadState::Button::X ? OTHER_BUTTON1 : 0) |
                           (gp.buttons & io::GamePadState::Button::Y ? OTHER_BUTTON2 : 0) | 0;
        smssystem[i] =
            (gp.buttons & io::GamePadState::Button::SELECT ? INPUT_PAUSE : 0) |
            (gp.buttons & io::GamePadState::Button::START ? INPUT_START : 0) |
            0;

#if NES_PIN_CLK != -1
        // When USB controller is connected both NES ports act as controller 2
        if (usbConnected)
        {
            if (i == 1)
            {
                nespadbuttons = nespadbuttons | nespad_states[1] | nespad_states[0];
            }
        }
        else
        {
            nespadbuttons |= nespad_states[i];
        }
#endif
// When USB controller is connected  wiipad acts as controller 2
#if WII_PIN_SDA >= 0 and WII_PIN_SCL >= 0
        if (usbConnected)
        {
            if (i == 1)
            {
                nespadbuttons |= wiipad_raw_cached;
            }
        }
        else // if no USB controller is connected, wiipad acts as controller 1
        {
            if (i == 0)
            {
                nespadbuttons |= wiipad_raw_cached;
            }
        }
#endif
        if (nespadbuttons > 0)
        {
            smsbuttons |= ((nespadbuttons & NESPAD_UP ? INPUT_UP : 0) |
                           (nespadbuttons & NESPAD_DOWN ? INPUT_DOWN : 0) |
                           (nespadbuttons & NESPAD_LEFT ? INPUT_LEFT : 0) |
                           (nespadbuttons & NESPAD_RIGHT ? INPUT_RIGHT : 0) |
                           (nespadbuttons & NESPAD_A ? INPUT_BUTTON1 : 0) |
                           (nespadbuttons & NESPAD_B ? INPUT_BUTTON2 : 0) | 0);
            smssystem[i] |= ((nespadbuttons & NESPAD_SELECT ? INPUT_PAUSE : 0) |
                             (nespadbuttons & NESPAD_START ? INPUT_START : 0) | 0);
        }

        // if (gp.buttons & io::GamePadState::Button::SELECT) printf("SELECT\n");
        // if (gp.buttons & io::GamePadState::Button::START) printf("START\n");
        input.pad[i] = smsbuttons;
        auto p1 = smssystem[i];
        if (ignorepushed == false)
        {
            pushed = smsbuttons & ~prevButtons[i];
            pushedsystem = smssystem[i] & ~prevButtonssystem[i];
            pushedother = otherButtons & ~prevOtherButtons[i];
        }
        else
        {
            pushed = smsbuttons;
            pushedsystem = smssystem[i];
            pushedother = otherButtons;
        }
        if (p1 & INPUT_PAUSE)
        {
            if (pushedsystem & INPUT_START)
            {
                // system_save_sram();
                // reset = true;
                // printf("Reset pressed\n");
                FrensSettings::savesettings();
                showSettings = true;
            }
            else if (pushed & INPUT_LEFT)
            {
                // Toggle audio output, ignore if HSTX is enabled, because HSTX must use external audio
#if EXT_AUDIO_IS_ENABLED && !HSTX
                settings.flags.useExtAudio = !settings.flags.useExtAudio;
                if (settings.flags.useExtAudio)
                {
                    printf("Using I2S Audio\n");
                }
                else
                {
                    printf("Using DVIAudio\n");
                }

#else
                settings.flags.useExtAudio = 0;
#endif
                //FrensSettings::savesettings();
            }
            else if (pushed & INPUT_UP)
            {
#if !HSTX
                scaleMode8_7_ = Frens::screenMode(-1);
#else
                Frens::toggleScanLines();
#endif
            }
            else if (pushed & INPUT_DOWN)
            {
#if !HSTX
                scaleMode8_7_ = Frens::screenMode(+1);
#else
                Frens::toggleScanLines();
#endif
            }
#if ENABLE_VU_METER
            else if (pushed & INPUT_RIGHT)
            {
                settings.flags.enableVUMeter = !settings.flags.enableVUMeter;
                //FrensSettings::savesettings();
                // printf("VU Meter %s\n", settings.flags.enableVUMeter ? "enabled" : "disabled");
                turnOffAllLeds();
            }
#endif
        }
        if (p1 & INPUT_START)
        {
            // Toggle frame rate display
            if (pushed & INPUT_BUTTON1)
            {
                settings.flags.displayFrameRate = !settings.flags.displayFrameRate;
                printf("FPS: %s\n", settings.flags.displayFrameRate ? "ON" : "OFF");
                // FrensSettings::savesettings();
            }
            else if (pushed & INPUT_BUTTON2)
            {
#if PICO_RP2350
                if (Frens::isPsramEnabled() && !SoundRecorder::isRecording())
                {
                    SoundRecorder::startRecording();
                }
#endif
            }
            else if (pushed & INPUT_UP)
            {
                loadSaveStateMenu = true;
                quickSaveAction = SaveStateTypes::LOAD;
            }
            else if (pushed & INPUT_DOWN)
            {
                loadSaveStateMenu = true;
                quickSaveAction = SaveStateTypes::SAVE;
            }
            else if (pushed & INPUT_LEFT)
            {
#if HW_CONFIG == 8
                settings.fruitjamVolumeLevel = std::max(-63, settings.fruitjamVolumeLevel - 1);
                EXT_AUDIO_SETVOLUME(settings.fruitjamVolumeLevel);
#endif
            }
            else if (pushed & INPUT_RIGHT)
            {
#if HW_CONFIG == 8
                settings.fruitjamVolumeLevel = std::min(23, settings.fruitjamVolumeLevel + 1);
                EXT_AUDIO_SETVOLUME(settings.fruitjamVolumeLevel);
#endif
            }
        }
        prevButtons[i] = smsbuttons;
        prevButtonssystem[i] = smssystem[i];
        prevOtherButtons[i] = otherButtons;
        // return only on first push
        if (pushed)
        {
            dst = smsbuttons;
        }
        if (pushedother)
        {
            if (pushedother & OTHER_BUTTON1)
            {
                printf("Other 1\n");
            }
            if (pushedother & OTHER_BUTTON2)
            {
                printf("Other 2\n");
            }
        }
    }
    input.system = *pdwSystem = smssystem[0] | smssystem[1];
    // return only on first push
    if (reset)
    {
        system_save_sram();
    }
    if (pushedsystem)
    {
        *pdwSystem = smssystem[0] | smssystem[1];
    }
}
//static DWORD *OLD_Pad1 = nullptr;
void in_ram(process)(void)
{

    DWORD pdwPad1, pdwPad2, pdwSystem; // have only meaning in menu
    // print address of pwdPad1 for debugging purposes
    //printf("pwdPad1 address: %p\n", (void *)&pdwPad1);
    // if (OLD_Pad1 != nullptr)
    // {
    //     // calculate offset between old and new address
    //     ptrdiff_t offset = (char *)&pdwPad1 - (char *)OLD_Pad1;
    //     printf("Offset between old and new pwdPad1: %ld bytes\n",offset);
    // }
    // OLD_Pad1 = &pdwPad1;
    while (reset == false)
    {
        processinput(&pdwPad1, &pdwPad2, &pdwSystem, false, nullptr);
#if PICO_RP2350
        sms.use_fm = settings.flags.useFM;
#endif
        sms_frame(0);
#if EXT_AUDIO_IS_ENABLED
        if (settings.flags.useExtAudio == 1 || Frens::isHeadPhoneJackConnected())
        {
            processaudioPerFrameI2S();
        } else
#endif
        {
#if !HSTX
            processaudioPerFrameDVI();
#else
            processaudioPerFrameHSTX();
#endif
        }
        ProcessAfterFrameIsRendered();
    }
}

static char selectedRom[FF_MAX_LFN];
/// @brief
/// Start emulator. Emulator does not run well in DEBUG mode, lots of red screen flicker. In order to keep it running fast enough, we need to run it in release mode or in
/// RelWithDebugInfo mode.
/// @return
int main()
{

    romName = selectedRom;
    ErrorMessage[0] = selectedRom[0] = 0;

    int fileSize = 0;
    cartType = TYPE_SMS;

    vreg_voltage voltage = VREG_VOLTAGE_1_20;
#if HSTX
    Frens::FlashParams *flashParams;
    // assign flashParams to point to flash location
    bool freqOverruled = false;
    flashParams = (Frens::FlashParams *)FLASHPARAM_ADDRESS;
    if ( Frens::validateFlashParams(*flashParams) ) {
        CPUFreqKHz = flashParams->cpuFreqKHz;
        voltage = flashParams->voltage;
        freqOverruled = true;
    }
#endif
    Frens::setClocksAndStartStdio(CPUFreqKHz, voltage);


    printf("==========================================================================================\n");
    printf("Pico-SMS+ %s\n", SWVERSION);
    printf("Build date: %s\n", __DATE__);
    printf("Build time: %s\n", __TIME__);
    printf("CPU freq: %d kHz\n", clock_get_hz(clk_sys) / 1000);
    printf("Stack size: %d bytes\n", PICO_STACK_SIZE);
    printf("==========================================================================================\n");
    printf("Starting up...\n");
    FrensSettings::initSettings(FrensSettings::emulators::SMS);
    // Note:
    //     - When using framebuffer, AUDIOBUFFERSIZE must be increased to 1024
    //     - Top and bottom margins are reset to zero
    isFatalError = !Frens::initAll(selectedRom, CPUFreqKHz, MARGINTOP, MARGINBOTTOM, AUDIOBUFFERSIZE, false, true);
    // Must go through applyScreenMode on both display paths: it is the only
    // place that pushes scanline *type* and aspect ratio to the HSTX driver as
    // well as the on/off bit. Calling hstx_setScanLines() directly here left
    // type/aspect at their power-on defaults until the first menu exit, so a
    // game started straight from boot (non-PSRAM builds reboot into the rom,
    // skipping the menu entirely) rendered with the wrong scanline settings.
    // It also read flags.scanlineOn, the legacy toggle this port hides and
    // nothing writes any more - screenMode owns scanlines now.
    scaleMode8_7_ = Frens::applyScreenMode(settings.screenMode);
    bool showSplash = true;
    g_settings_visibility = g_settings_visibility_sms;
    g_available_screen_modes = g_available_screen_modes_sms;
    while (true)
    {
        #if 1
        if (strlen(selectedRom) == 0 || reset == true )
        {
            menu("Pico-SMS+", ErrorMessage, isFatalError, showSplash, ".sms .gg .sg", selectedRom);
            // returns only when PSRAM is enabled,
            printf("Selected rom from menu: %s\n", selectedRom);
        }
        #endif
        reset = false;
        fileSize = 0;
        cartType = TYPE_SMS;
        //EXT_AUDIO_MUTE_INTERNAL_SPEAKER(settings.flags.fruitJamEnableInternalSpeaker == 0);
        if (Frens::isPsramEnabled())
        {
            if (Frens::cstr_endswith(selectedRom, ".sg"))
            {
                // SG-1000 roms have no "TMR SEGA" header to take the size from.
                // In PSRAM mode selectedRom holds the full path of the loaded rom.
                FILINFO fno;
                cartType = TYPE_SG;
                if (f_stat(selectedRom, &fno) == FR_OK)
                {
                    fileSize = fno.fsize;
                }
            }
            else
            {
                // Detect rom type from memory
                // This is used to determine the rom size and type (SMS or GG)
                bool isGameGear = false;
                detect_rom_type_from_memory(ROM_FILE_ADDR, &fileSize, &isGameGear);
                cartType = isGameGear ? TYPE_GG : TYPE_SMS;
            }
            if (fileSize == 0)
            {
                // No rom loaded, continue to menu
                printf("No rom loaded, continuing to menu\n");
                selectedRom[0] = 0;
                continue;
            }
        }
        else
        {
            FRESULT fr;
            FIL file;
            fr = f_open(&file, selectedRom, FA_READ);
            if (fr != FR_OK)
            {
                snprintf(ErrorMessage, 40, "Cannot open rom %d", fr);
                printf("%s\n", ErrorMessage);
                selectedRom[0] = 0;
                continue;
            }
            fileSize = f_size(&file);
            f_close(&file);
            if (Frens::cstr_endswith(selectedRom, ".gg"))
            {
                cartType = TYPE_GG;
            }
            else if (Frens::cstr_endswith(selectedRom, ".sg"))
            {
                cartType = TYPE_SG;
            }
            printf("Now playing: %s (%d bytes)\n", selectedRom, fileSize);
        }
        if (cartType == TYPE_GG)
        {
            printf("Game Gear rom detected\n");
        }
        else if (cartType == TYPE_SG)
        {
            printf("SG-1000 rom detected\n");
        }
        else
        {
            printf("Master System rom detected\n");
        }
        if (isAutoSaveStateConfigured())
        {
            char tmpPath[40];
            getAutoSaveStatePath(tmpPath, sizeof(tmpPath));
            printf("Auto-save is configured found for this ROM (%s)\n", tmpPath);
            if (Frens::fileExists(tmpPath))
            {
                printf("Auto-save state found for this ROM (%s)\n", tmpPath);
                printf("Loading auto-save state...\n");
                loadSaveStateMenu = true;
                quickSaveAction = SaveStateTypes::LOAD_AND_START;
                framesbeforeAutoStateIsLoaded = 120; // wait 2 seconds before loading auto state
            }
            else
            {
                printf("No auto-save state found for this ROM.\n");
            }
        }
        else
        {
            printf("No auto-save configured for this ROM.\n");
        }
        do {
            reset = resetGame = false;
          
            load_rom(ROM_FILE_ADDR, fileSize, cartType);
            // Initialize all systems and power on
            system_init(SMS_AUD_RATE);
            // load state if any
            // system_load_state();
            system_reset();
            printf("Starting game\n");
                        // After a non-PSRAM reboot the monitor needs time to sync with the
            // fresh HDMI signal.  Without a delay the FDS BIOS intro animation
            // plays while the display is still dark.  Only needed on the very
            // first launch (showSplash is true); resets keep the link up.
            // This also benefits RP2040/RP2350: .nsf files don't clip sound at the start, roms that 
            // start with sound also don't clip sound.
            if (showSplash && !Frens::isPsramEnabled())
            {
                showSplash = false;
                printf("Feeding blank frames for display sync...\n");
                menuPumpBlankFrames(180);
            }
            loadoverlay();
            Frens::PaceFrames60fps(true); 
            process();
            system_shutdown();
        } while (resetGame);
        selectedRom[0] = 0;
        showSplash = false;
#if ENABLE_VU_METER
        turnOffAllLeds();
#endif
    }

    return 0;
}
