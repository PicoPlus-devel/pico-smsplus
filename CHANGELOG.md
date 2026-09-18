# CHANGELOG

Adds support for **Sega SG-1000** games.

# General Info


[Binaries for each configuration and PCB design are at the end of this page](#downloads___).

For board-by-board wiring, supported display modes and more refer to the [pico-infonesPlus documentation](https://github.com/fhoedemakers/pico-infonesPlus#setup). The set of supported boards and their pinouts is identical between the two projects.

# v0.29 Release notes

## What's new

- **Sega SG-1000 games.** Roms with the `.sg` extension can now be played. Copy them to the SD card next to your Master System and Game Gear roms. Save states work as usual.
- The extension must be lowercase: rename files ending in `.SG` to `.sg`.
- **Metadata for SG-1000 games.** The [metadata pack](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip) now also contains box art, game information and themed bezels for SG-1000 games. The bezels are shown in-game on RP2350 boards. Download it again and extract it to the root of the SD card. Thanks again **Gavin Knight** ([DynaMight1124](https://github.com/DynaMight1124))
- **USB drive mode**: the settings menu can now show the SD card on your computer over USB, so you can add or remove games without taking the card out. Open the menu with SELECT from the game list, pick *USB drive mode*, then eject the drive on your computer when you are done. It is not available while a game is running. See [USB drive mode](https://github.com/fhoedemakers/pico-smsplus#usb-drive-mode).

# v0.28 Release notes

A maintenance release. It brings the shared menu and support code up to date; the emulator itself is unchanged. Upgrading is only a matter of flashing the new `.uf2` — your settings, saves and save states on the SD card are untouched.

## Fixes

- **DVI-only monitors show a picture again.** With **Display Mode** set to DVI, some older screens that accept DVI but not HDMI stayed black. They work again.
- **Clone Pico boards no longer crash at start-up.** Boards fitted with a cheaper flash chip than a genuine Raspberry Pi Pico could hang on power-on. The chip is now recognised and driven at a speed it can handle. Genuine Picos are unaffected.
- **Steadier start-up.** The board lets its power settle before switching to the higher clock speed.

# v0.27 Release notes

This release adds a recently played list, avoids re-flashing a rom that is
already in flash, adds SNES controller support on the GPIO controller port, and
updates the PicoNES PCB design. It also contains a number of display, settings
and menu fixes.

## What's new

### Recently played

The menu keeps a list of the last 20 games that were started, newest first. It is
opened with Button3 in the rom browser — X on a SNES controller, Y on XInput,
Triangle on PlayStation, C on Genesis, X on a Wii Classic pad — or from the new
**Recently played** entry at the top of the settings menu.

In the list, Button2 starts the selected game, SELECT removes it from the list,
START shows its artwork, and Button1 closes the list. The settings menu offers
the entry only when it is opened from the rom browser, not from inside a running
game. That is also the route for pads without a Button3, such as a NES pad on the
controller port.

The list is stored as plain text in `/recent_SMS.txt` in the SD card root, one
line per game, and can be edited or deleted on a PC. Master System and Game Gear
roms share a single list. A game that is no longer present on the card is
reported as missing when it is started, and can be removed with SELECT. An
unreadable list is treated as empty; no other settings are affected.

On boards without PSRAM, the entry whose rom is currently in flash is marked
`[READY]`.

### Roms already in flash are no longer re-flashed

On boards without PSRAM the rom was written to flash on every launch, including
when the image already in flash was the game being started. The emulator now
detects this case and skips the write, which removes the delay when restarting a
game that was just played. The image is verified before the write is skipped;
anything that does not match is flashed as before.

### SNES controllers on the GPIO controller port

The controller port now reads all 12 buttons of a SNES pad, and the menu maps its
face buttons by name: Button2 selects, Button1 goes back, and X opens the
recently played list, as on USB and Wii Classic pads. Previously a SNES pad was
read as a NES pad, so these buttons were mapped incorrectly. NES pads are
unaffected.

The Controller Test screen now reports the detected pad type, names the buttons
according to that type, and shows the raw data received from the pad on the GPIO
ports.

### PicoNES PCB revision v2.6

The PicoNES PCB design has been updated to v2.6 (`pico_nesPCB_v2.6.zip`),
replacing v2.1 as the current release. It adds through-holes, allowing a Pico to
be mounted on male headers instead of soldered flat, which makes the
[Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107)
usable on this PCB. It also corrects the silkscreen labelling of the D3/D4 pads
on controller port 2, which was reversed on v2.1. Only the labelling differed;
the routing is identical on both revisions and no firmware change is required.

The README now documents all three PCB designs —
[PicoNES, PicoNES Mini and PicoNES Micro](https://github.com/fhoedemakers/pico-smsplus#custom-pcbs).

## Fixes

- Fixed the scanline setting being ignored when a game starts. Games started
  without the selected scanline effect and only picked it up after the settings
  menu had been opened and closed. Starting a game now applies the selected
  screen mode, scanline style and aspect ratio immediately. This mainly affected
  boards that boot directly into a game, which is every board without PSRAM.
- Resetting the settings to their defaults no longer leaves an unused internal
  scanline switch at the value held by the settings file.
- Fixed the settings menu discarding its result. A screen opened from the
  settings menu — the recently played list or the controller test — could return
  to the screensaver instead of applying what was selected.
- Reduced stack usage while browsing the rom list.
- Debug builds now report HDMI audio underruns per interval in addition to the
  total counted since boot.

# v0.26 Release notes

This release is mostly about **sound**. Japanese Master System games gain
their proper FM voice, Game Gear games with sampled speech finally sound
right, and the audio path itself has been cleaned up so the emulator no
longer thumps on boot and FM peaks no longer distort.

## What's new

### YM2413 FM sound for Japanese SMS games (RP2350 only in certain board config)

Japanese Master System cartridges that use the YM2413 (OPLL) FM sound
chip — *Phantasy Star*, *Wonder Boy III: The Dragon's Trap*, *Ys*,
*After Burner* and many others — now play with their original FM
instruments instead of the PSG fallback. Built on the vendored
[emu2413](https://github.com/digital-sound-antiques/emu2413) core by
Mitsutaka Okazaki.

FM can be toggled in the settings menu. RP2040 builds are unchanged
(FM is not enabled there for performance reasons).

### Works with pico-bootLoader

There is a new companion project,
[pico-bootLoader](https://github.com/fhoedemakers/pico-bootLoader), that lets
one RP2350 board hold several emulators (and a native *Doom* port) at the same
time. Every power-on brings up a menu where you pick which one to run — no more
plugging the board into a PC and copying a `.uf2` over just to switch systems.

This release makes the Master System / Game Gear emulator one of those
selectable entries. When it has been started from the bootloader, the in-game
settings menu gains a **Return to emulator selection** entry so you can hop
straight back to the picker.

Nothing changes if you don't use it: the normal `.uf2` downloads below are
still stand-alone and install exactly as before. The bootloader-ready builds
come with the [pico-bootLoader
release](https://github.com/fhoedemakers/pico-bootLoader/releases/latest).

### Game Gear digitized speech fixed

Game Gear games such as *Sonic the Hedgehog 2* stream digitized speech
by rapidly changing PSG channel volume mid-frame. The old once-per-frame
audio renderer collapsed all of those writes into a single sample and
reduced the famous "Segaaaa" intro to a crackle. Audio is now rendered
per scanline (~15.7 kHz effective update rate), so sampled speech and
sound effects come through cleanly.

### Cleaner audio output

- **No more boot thump.** A DC offset that produced a loud pop on
  startup has been removed by a small one-pole high-pass (DC blocker)
  on the DVI/HSTX/I2S audio outputs.
- **Recovered headroom.** With the DC offset gone, the previous safety
  attenuation has been halved, so the audio is noticeably louder
  without clipping.
- **FM peaks no longer distort.** A signed/unsigned bug in the DC
  blocker would wrap loud FM notes into broadband noise; fixed so the
  FM and PSG channels mix cleanly.

### Under the hood

- **Automatic overclock when FM is on (RP2350).** The default clock
  stays at 252 MHz; enabling YM2413 FM (or the existing overclock
  setting) bumps the chip to 378 MHz on HSTX boards and 324 MHz on
  PicoDVI boards to cover the extra DSP work. The HSTX value of
  378 MHz is a multiple of 126 MHz, so the HSTX clock stays on its
  clean path and HDMI audio timing is preserved.
- **128 KB RAM reclaimed on RP2350.** The FM core's largest lookup
  table is now pre-computed at build time and stored in flash instead
  of being built in RAM at startup.
- **Audio frame pacing on RP2040.** A small number of blank frames are
  now pumped out around mode changes to keep audio in lock-step with
  the display.

## Fixes

- Internal settings-visibility table cleaned up so menu entries appear
  on exactly the boards that support them.
- The border overlay now shows up reliably when starting a game.
- **Random crash fixed.** The screen buffer could end up at an address the
  processor doesn't like, which showed up as an occasional lock-up while
  playing. It is now always placed correctly.
- **Steadier overclocking.** Boards running at the higher clock speeds could
  fault a few seconds into a game. The flash memory timing now follows the
  chosen speed instead of being fixed, so those settings stay stable.
- Builds work again with the newest version of Raspberry Pi's `picotool`.

## Credits

Mitsutaka Okazaki for the [emu2413](https://github.com/digital-sound-antiques/emu2413)


# v0.25 Release notes

This release replaces PicoDVI with HSTX on more boards, HSTX now also carries audio over HDMI,
adds smoother on-screen motion, a few new in-game conveniences, and a
handful of fixes that make the emulator more reliable in everyday use.

A huge thank you to [@fliperama86](https://github.com/fliperama86) for the
excellent [pico_hdmi](https://github.com/fliperama86/pico_hdmi) driver that
made the new HDMI output possible, and for all the help along the way.

## What's new

### HSTX Video and sound over HDMI

On the technical side, several RP2350 board configurations have switched
from the **PicoDVI** software-driven video output to **HSTX**, the
RP2350's dedicated High-Speed Serial Transmit hardware (GPIO 12 – 19).
HSTX has been used for video on some boards before, but in this release
it also carries **audio embedded in the HDMI stream** for the first
time — that's the new capability HSTX gains here. (PicoDVI has always
been able to embed audio; HSTX is just catching up on that front while
offloading the work from the CPU to dedicated hardware.)

In practice, on these boards picture and sound now travel together over a
single HDMI cable — no separate audio jack needed:

- Adafruit Fruit Jam
- Murmulator M2

These RP2350 boards have also been switched from PicoDVI to HSTX. (Video and sound):

- [Breadboard build](https://github.com/fhoedemakers/pico-infonesPlus?tab=readme-ov-file#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard)
- [PCB build](https://github.com/fhoedemakers/pico-infonesPlus?tab=readme-ov-file#pcb-with-raspberry-pi-pico-or-pico-2)
- [Adafruit Metro RP2350](https://github.com/fhoedemakers/pico-infonesPlus?tab=readme-ov-file#adafruit-metro-rp2350)

All other boards continue to use PicoDVI and work as before.

### Smoother gameplay

Frame timing now follows the display's own refresh signal, which removes
small stutters and gives the picture a more consistent, smooth feel.

### New options and conveniences

- **Reset the running game** from the in-game menu — no need to power-cycle
  the device to restart.
- **Enter flashing mode from the settings menu**, so you can update the
  firmware without having to unplug the device and hold the BOOTSEL button.
- **Scanline effect for HDMI boards** — turn on the classic CRT scanline
  look from the settings menu when running on an HDMI-capable board.

### Adafruit Fruit Jam

- **Automatic headphone detection.** Plug headphones in and the built-in
  speaker mutes itself; unplug them and the speaker comes back. The old
  manual mute setting and pushbutton-1 mute shortcut have been removed —
  they are no longer needed.

## Fixes

- **Save states work again.** A bug introduced in v0.24 could crash the
  emulator when loading a saved game. This is now fixed. Note: save state
  files created by earlier v0.25 development builds are not compatible
  with this fix and will need to be re-created.
- **Picture-loss recovery.** On the new HSTX output when set to
  video-only (DVI) mode, the monitor could occasionally lose the picture.
  The emulator now detects this and automatically restores the signal
  without needing a restart. (Not observed in full HDMI mode, but the
  same safety net is enabled there too just in case.)
- **Screen cropping fix.** A small visual glitch where part of the game
  picture was being cut off incorrectly has been corrected.

## Credits

Updated to acknowledge new contributors — see the splash screen on
startup.

# previous changes

See [HISTORY.md](https://github.com/fhoedemakers/pico-smsplus/blob/main/HISTORY.md)

<a name="downloads___"></a>
## Downloads by configuration

Binaries for each configuration are listed below. Binaries for Pico(2) also work for Pico(2)-w. No blinking led however on the -w boards.
For some configurations risc-v binaries are available. It is recommended however to use the arm binaries. 

### Standalone boards

| Board | Binary | Readme | |
|:--|:--|:--|:--|
| Adafruit Metro RP2350 | [picosmsPlus_AdafruitMetroRP2350_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitMetroRP2350_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-metro-rp2350) | |
| Adafruit Fruit Jam | [picosmsPlus_AdafruitFruitJam_arm_piousb.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitFruitJam_arm_piousb.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-fruit-jam)| |
| Waveshare RP2040-PiZero | [picosmsPlus_WaveShareRP2040PiZero_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_WaveShareRP2040PiZero_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board)| [3-D Printed case](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#3d-printed-case-for-rp2040rp2350-pizero) |
| Waveshare RP2350-PiZero | [picosmsPlus_WaveShareRP2350PiZero_arm_piousb.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_WaveShareRP2350PiZero_arm_piousb.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#waveshare-rp2040rp2350-pizero-development-board)| [3-D Printed case](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#3d-printed-case-for-rp2040rp2350-pizero) |

### Breadboard

| Board | Binary | Readme |
|:--|:--|:--|
| Pico| [picosmsPlus_AdafruitDVISD_pico_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico W | [picosmsPlus_AdafruitDVISD_pico_w_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico 2 | [picosmsPlus_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Pico 2 W | [picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |
| Adafruit feather rp2040 DVI | [picosmsPlus_AdafruitFeatherDVI_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitFeatherDVI_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#adafruit-feather-rp2040-with-dvi-hdmi-output-port-setup) |
| Pimoroni Pico Plus 2 | [picosmsPlus_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-with-adafruit-hardware-and-breadboard) |


### PCB Pico/Pico2

| Board | Binary | Readme |
|:--|:--|:--|
| Pico| [picosmsPlus_AdafruitDVISD_pico_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico W| [picosmsPlus_AdafruitDVISD_pico_w_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico 2 | [picosmsPlus_AdafruitDVISD_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |
| Pico 2 W | [picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#pcb-with-raspberry-pi-pico-or-pico-2) |

PCB: [pico_nesPCB_v2.6.zip](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/pico_nesPCB_v2.6.zip) (new in this release, replaces v2.1). [Readme](https://github.com/fhoedemakers/pico-smsplus#picones-pcb)

3D-printed case designs for PCB:

[https://www.thingiverse.com/thing:6689537](https://www.thingiverse.com/thing:6689537). 
For the latest two player PCB 2.0, you need:

- Top_v2.0_with_Bootsel_Button.stl. This allows for software upgrades without removing the cover. (*)
- Base_v2.0.stl
- Power_Switch.stl.
(*) in case you don't want to access the bootsel button on the Pico, you can choose Top_v2.0.stl

When the Pico is mounted with male headers on the v2.6 PCB, use the latest top cover. The older covers assume a Pico soldered flat and leave no room for the USB cable.

### PCB WS2XX0-Zero (PCB required)

| Board | Binary | Readme |
|:--|:--|:--|
| Waveshare RP2040-Zero | [picosmsPlus_WaveShareRP2040ZeroWithPCB_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_WaveShareRP2040ZeroWithPCB_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-smsplus#picones-mini-pcb) |
| Waveshare RP2350-Zero | [picosmsPlus_WaveShareRP2350ZeroWithPCB_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_WaveShareRP2350ZeroWithPCB_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-smsplus#picones-mini-pcb) |

PCB: [Gerber_PicoNES_Mini_PCB_v2.0.zip](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/Gerber_PicoNES_Mini_PCB_v2.0.zip)

3D-printed case designs for PCB WS2XX0-Zero:
[https://www.thingiverse.com/thing:7041536](https://www.thingiverse.com/thing:7041536)

### PCB Waveshare RP2350-USBA with PCB
[Binary](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_WaveShare2350USBA_arm_piousb.uf2)

PCB: [Gerber_PicoNES_Micro_v1.2.zip](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/Gerber_PicoNES_Micro_v1.2.zip)

[Readme](https://github.com/fhoedemakers/pico-smsplus#picones-micro-pcb)

[Build guide](https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/)


### Pimoroni Pico DV

| Board | Binary | Readme |
|:--|:--| :--|
| Pico/Pico w | [picosmsPlus_PimoroniDVI_pico_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_PimoroniDVI_pico_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |
| Pico 2/Pico 2 w | [picosmsPlus_PimoroniDVI_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_PimoroniDVI_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |
| Pimoroni Pico Plus 2 | [picosmsPlus_PimoroniDVI_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_PimoroniDVI_pico2_arm.uf2) | [Readme](https://github.com/fhoedemakers/pico-infonesPlus/blob/main/README.md#raspberry-pi-pico-or-pico-2-setup-for-pimoroni-pico-dv-demo-base) |

> [!NOTE]
> On Pico W and Pico2 W, the CYW43 driver (used only for blinking the onboard LED) causes a DMA conflict with I2S audio on the Pimoroni Pico DV Demo Base, leading to emulator lock-ups. For now, no Pico W or Pico2 W binaries are provided; please use the Pico or Pico2 binaries instead. (#132)

### Murmulator M1

For more info about the Murmulator see this website: https://murmulator.ru/ and [#150](https://github.com/fhoedemakers/pico-infonesPlus/issues/150)

| Board | Binary |
|:--|:--|
| Pico/Pico w | [picosmsPlus_MurmulatorM1_pico_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_MurmulatorM1_pico_arm.uf2) |
| Pico 2/Pico 2 w | [picosmsPlus_MurmulatorM1_pico2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_MurmulatorM1_pico2_arm.uf2) |

### Murmulator M2

For more info about the Murmulator see this website: https://murmulator.ru/ and [#150](https://github.com/fhoedemakers/pico-infonesPlus/issues/150)

| Board | Binary |
|:--|:--|
| Pico/Pico w | [picosmsPlus_MurmulatorM2_arm.uf2](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/picosmsPlus_MurmulatorM2_arm.uf2) |

### Other downloads

- Metadata: [SMSPlusMetadata.zip](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip)


Extract the zip file to the root folder of the SD card. Select a game in the menu and press START to show more information and box art. Works for most official released games. Screensaver shows floating random cover art. Works for RP2040 and RP2350.
Extra on RP2350: when a game is loaded a themed bezel/border is shown.



























