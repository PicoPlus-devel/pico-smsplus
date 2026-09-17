
# pico-smsplus, A Sega Master System, Game Gear and SG-1000 emulator for RP2040/RP2350 based boards.

This software is a port of [SmsPlus](https://segaretro.org/SMS_Plus), a Sega Master System and Game Gear emulator for RP2040/RP2350 based microcontroller boards like the RaspberryPi Pico and Pico 2. Sound and video are ouput over HDMI. Support for the Sega SG-1000 has been added to this port; see [SG-1000 support](#sg-1000-support).
The code for HDMI output is based on [Shuichi Takano's Pico-InfoNes project](https://github.com/shuichitakano/pico-infones) which in turn is based on [PicoDVI](https://github.com/Wren6991/PicoDVI).

Create a FAT32 (recommended. see [#29](https://github.com/fhoedemakers/pico-smsplus/issues/29)) or exFAT formatted SD card and copy your Master System (.sms), Game Gear (.gg) and/or SG-1000 (.sg) roms and optional [metadata](#using-metadata) on to it.
You can organize the roms in directories. A menu is displayed on which you can select the rom to play. The last 20 games you started are kept in a [recently played list](#recently-played-games), one button press away in the menu.

Supports two controllers for two player Master System and SG-1000 games. [See "about two player games" below for specifics and limitations](#about-two-player-games) 

Save and load state possible.

Battery backed saves stored on SD for games that support this. 

See the [releases](https://github.com/fhoedemakers/pico-smsplus/releases/latest) page for the supported RP2040/RP2350 boards.

***

## Video
Click on image below to see a demo video.

[![Video](https://img.youtube.com/vi/__E8h2Ay3g8/0.jpg)](https://www.youtube.com/watch?v=__E8h2Ay3g8)

***
## RP2040 notes

**The emulator works very well on the Pico 2/RP2350. On the Pico/RP2040, some games may not run at full speed or at all. (like afterburner).**

>[!WARNING] 
> On Pico/RP2040, some games show red flashing between screens. This can be occasionally or severe depending on the game. If you are sensitive for this, or experience health issues while playing those games, please stop playing immediately.

***

## SG-1000 Support

Sega SG-1000 cartridge images with the `.sg` file extension can be played. The extension must be written in lowercase: files ending in `.SG` are listed in the menu, but are started as Master System roms and must be renamed first.

- The RAM expansion used by several Taiwanese releases (for example Knightmare and TwinBee) and the extra cartridge RAM of titles such as The Castle are detected automatically.
- Save states are supported. SG-1000 cartridges have no battery-backed saves.
- The SG-1000 has a fixed palette and no FM sound; the YM2413 FM setting has no effect for these games.
- The [metadata pack](#using-metadata) contains box art, game information and themed bezels for SG-1000 games.

***

## Save Game Support for SMS Games

[For Sega Master System (SMS) games that support it](https://consolemods.org/wiki/Master_System:List_of_Master_System_Games_with_Save_Batteries), progress can be saved to SD card.

### How It Works

- **Saving**: To ensure game data is written to the SD card, you must return to the main menu by pressing **Select + Start**.
- **Save Location**: Save files are stored in the `/SAVES` directory with a `.SAV` extension.
- **Auto-Loading**: When you start a game, its corresponding `.SAV` file will be loaded automatically (if available).

>[!NOTE]
>Make sure to return to the menu before powering off, or the save data may be lost.

***

## YM2413 FM sound

A number of Japanese Master System games can use the **YM2413 (OPLL) FM sound chip**, an add-on that was sold with the Japanese Mark III and built into the Japanese Master System. On those games the FM chip replaces the standard PSG soundtrack with a much richer one. Games that support it include:

- *Phantasy Star* (Japanese release)
- *Wonder Boy III: The Dragon's Trap*
- *Ys*
- *After Burner*

and a number of others. Games that do not support FM are unaffected by the setting — they never address the chip and sound exactly the same either way.

FM emulation is **off by default**. Turn it on with the **YM2413 FM** entry in the settings menu.

> [!IMPORTANT]
> Enabling YM2413 FM raises the processor clock from 252 MHz to **378 MHz** to cover the extra work of emulating the chip. The board reboots to apply the new clock, and reboots again when the setting is turned back off. For that reason it is worth leaving off unless you are actually playing a game that uses FM.

The setting is only present on **HSTX-based RP2350 configurations**, which is where the emulator has the headroom for it:

| Board / configuration | Binary |
|:--|:--|
| Breadboard or [PicoNES PCB](#picones-pcb) with a Pico 2 / Pico 2 W / Pimoroni Pico Plus 2 | `picosmsPlus_AdafruitDVISD_pico2_arm.uf2`, `picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2` |
| Adafruit Metro RP2350 | `picosmsPlus_AdafruitMetroRP2350_arm.uf2` |
| Adafruit Fruit Jam | `picosmsPlus_AdafruitFruitJam_arm_piousb.uf2` |
| Murmulator M2 | `picosmsPlus_MurmulatorM2_arm.uf2` |

On every other configuration — all RP2040 boards, and the RP2350 boards that use the PicoDVI video driver instead of HSTX — the entry does not appear in the settings menu and those games fall back to their PSG soundtrack.

***

## System requirements and setup - What do yo need?

The binary specific for your config can be downloaded from the [releases](https://github.com/fhoedemakers/pico-smsplus/releases/latest) page.

You need a FAT32 or exFAT formatted SD card to put your .sms, .gg and .sg roms on, preferably in /roms/SMS (subdirectory organization is supported). It is highly recommended to use FAT32, see https://github.com/fhoedemakers/pico-smsplus/issues/29

>[!NOTE]
> For detailed instructions how to setup specific configurations, see the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus).

*** 

## For Raspberry Pi Pico / Pico W / Pico 2

- Raspberry Pi Pico and [Pimoroni Pico DV Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base?variant=39494203998291)
- [PicoNES custom printed circuit board](#picones-pcb)
- Breadboard with components.

*** 

## Other RP2040 based boards

These boards already contain an RP2040 cpu, a separate Raspberry Pi Pico is not needed.

- [Adafruit Feather RP2040 with DVI Output](https://www.adafruit.com/product/5710) and [FeatherWing - RTC + SD](https://www.adafruit.com/product/2922).
- [Waveshare RP2040-PiZero Development Board](https://www.waveshare.com/rp2040-pizero.htm)

## Other RP2350 based boards


- [Adafruit Fruit Jam](https://www.adafruit.com/product/6200)
- [Adafruit Metro RP2350](https://www.adafruit.com/product/6003) or [Adafruit Metro RP2350 with PSRAM](https://www.adafruit.com/product/6267)
- [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107)
  Use the breadboard config, the Pimoroni Pico DV Demo base, or the [PicoNES PCB](#picones-pcb) from design v2.6 onwards — that revision added through-holes, so the board can be plugged in on male headers instead of lying flat against the PCB, which the SP/CE connector on its back prevents.
  The PSRAM on the board is used in stead of flash to load the roms from SD.
- [Waveshare RP2350-PiZero](https://www.waveshare.com/rp2350-pizero.htm)
- [Waveshare RP2350-Zero](https://www.waveshare.com/rp2350-zero.htm) and [Waveshare RP2350-USB-A](https://www.waveshare.com/rp2350-usba.htm), each on their own PCB — see [Custom PCBs](#custom-pcbs).


***

# Custom PCBs

Three community PCB designs turn a supported board and its breakouts into a finished little console, each with an optional 3D-printed case. They are simply a neater way to build hardware this emulator already supports, so nothing changes in the firmware: flash the binary for that configuration and you are done.

| Design | Board it carries | Build | Gerber archive | Designed by |
| --- | --- | --- | --- | --- |
| [PicoNES](#picones-pcb) | Pico 2, Pico 2 W, Pimoroni Pico Plus 2 or an original Pico | `-c2` | `pico_nesPCB_v2.6.zip` | John Edgar Park |
| [PicoNES Mini](#picones-mini-pcb) | Waveshare RP2350-Zero | `-c6` | `Gerber_PicoNES_Mini_PCB_v2.0.zip` | Gavin Knight |
| [PicoNES Micro](#picones-micro-pcb) | Waveshare RP2350-USB-A | `-c9` | `Gerber_PicoNES_Micro_v1.2.zip` | Gavin Knight |

All three archives are attached to every [release](https://github.com/fhoedemakers/pico-smsplus/releases/latest) of this project and also live in [pico_shared/PCB](https://github.com/fhoedemakers/pico_shared/tree/main/PCB). Upload the zip as-is to a PCB manufacturer of your choice; [PCBWay](https://www.pcbway.com/) and JLCPCB are both good options.

The designs come from [pico-infonesPlus](https://github.com/fhoedemakers/pico-infonesPlus) and kept their NES-flavoured names, but there is nothing NES-specific about them — they are DVI, microSD and controller wiring, and this emulator runs on them just as well. The PicoNES and PicoNES Mini both have two controller ports, so [two player Master System games](#about-two-player-games) work on them; the PicoNES Micro is a single-controller design.

> [!NOTE]
> Sellers on AliExpress have copied the PicoNES design and sell ready-made boards. For questions about those, contact the seller.

## PicoNES PCB

The original design, by [@johnedgarpark](https://twitter.com/johnedgarpark). It carries the Pico, the DVI and microSD breakouts and up to two NES controller ports. It is also the only one of the three that takes an interchangeable Pico-format board, which is what makes a Pimoroni Pico Plus 2 — and with it PSRAM — an option, and the only one that can carry an original RP2040 Pico. The current design is **v2.6**.

<img width="480" alt="Populated PCB with a Pico plugged into the through-holes" src="https://github.com/user-attachments/assets/2bbc846d-56b1-4528-9899-01bc9b32ce11" />

### Mounting the Pico

Design v2.6 added through-holes, so there are now two ways to fit the board:

| Mounting | Boards | Design version |
| --- | --- | --- |
| Soldered flat onto the PCB, no headers | Pico 2, Pico 2 W, Pico | any |
| Male headers plugged into the through-holes | Pico 2, Pico 2 W, Pico, Pimoroni Pico Plus 2 | v2.6 or later |

> [!IMPORTANT]
> A [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107) needs v2.6 **and** male headers. On v2.1 and older designs the board has to lie flat against the PCB, which the SP/CE connector on the back of the Pimoroni Pico Plus 2 prevents.

> [!NOTE]
> Soldering skills are required. Solder every connection from the Pico to the PCB, including the ones on the short right-hand side of the board — those are ground.

### What you need

- One of the following, mounted as described above:
  * Raspberry Pi Pico 2 or Pico 2 W **without headers**, soldered flat.
  * Raspberry Pi Pico 2, Pico 2 W or [Pimoroni Pico Plus 2](https://shop.pimoroni.com/products/pimoroni-pico-plus-2?variant=42092668289107) **with male headers** soldered on ([these](https://a.co/d/dSNPuyo) fit), plugged into the through-holes.
  * An original Raspberry Pi Pico, either way — see [RP2040 notes](#rp2040-notes) for what the RP2040 gives up.
- [Adafruit DVI Breakout Board — For HDMI Source Devices](https://www.adafruit.com/product/4984)
- [Adafruit Micro SD SPI or SDIO Card Breakout Board — 3V ONLY!](https://www.adafruit.com/product/4682)
- For controllers on the GPIO ports:
  * [one or two NES controller ports](https://www.zedlabz.com/products/controller-connector-port-for-nintendo-nes-console-7-pin-90-degree-replacement-2-pack-black-zedlabz) — populate both if you want to play two player Master System games
  * NES or SNES controllers
- [Micro USB to OTG Y-cable](https://a.co/d/b9t11rl) if you want to use a USB game controller — it powers the board and connects the controller at the same time.
- Micro USB power supply.
- Optional: an on/off switch, such as [this one](https://www.kiwi-electronics.com/en/spdt-slide-switch-410?search=KW-2467).

> [!NOTE]
> A plain NES controller has no Button3, so it cannot open the [recently played list](#recently-played-games) directly — use the settings menu entry instead. The sockets speak the SNES protocol as well, so an SNES pad with a [SNES-to-NES adapter cable](https://nl.aliexpress.com/item/1005007923169070.html) — [or one you make yourself](http://www.neshq.com/hardmods/snes_to_nes_controller.txt) — is the better choice.

### Which binary to flash

- Pico 2 **and** Pimoroni Pico Plus 2 — `picosmsPlus_AdafruitDVISD_pico2_arm.uf2`
- Pico 2 W — `picosmsPlus_AdafruitDVISD_pico2_w_arm.uf2`
- Original Pico — `picosmsPlus_AdafruitDVISD_pico_arm.uf2`
- Pico W — `picosmsPlus_AdafruitDVISD_pico_w_arm.uf2`

The Pimoroni Pico Plus 2 needs no separate build. The emulator reads the real flash size from the chip at boot and detects PSRAM at runtime, so the same `pico2` image adapts to whichever board is plugged in.

### What the Pimoroni Pico Plus 2 adds

The Pimoroni Pico Plus 2 brings 8 MB of PSRAM and 16 MB of flash. The PSRAM is what you notice: roms are loaded into it and a game starts the moment you select it, instead of after the wait a plain Pico 2 needs to write the rom to its flash.

### 3D printed case

Gavin Knight ([DynaMight1124](https://github.com/DynaMight1124)) designed an NES-like enclosure for this PCB: [thingiverse.com/thing:6689537](https://www.thingiverse.com/thing:6689537). The v2.0 design has a base, a power-switch part and a choice of two top covers — one with a button that reaches the BOOTSEL button so firmware can be updated without opening the case, one without. Print the files that match the PCB version you own; Gavin's Thingiverse page has the details.

> [!IMPORTANT]
> If the Pico is mounted with male headers, download the **latest** top cover. Headers raise the Pico, and only the newest cover leaves room for the USB cable — the older ones assume a Pico soldered flat onto the PCB.

<img width="480" alt="Top cover with a button for BOOTSEL" src="https://github.com/user-attachments/assets/3c8f8990-51b9-4873-9054-64bb2cd6c300" />

For the full photo gallery and assembly detail, see the [PCB section of the pico-infonesPlus documentation](https://github.com/fhoedemakers/pico-infonesPlus#pcb-with-raspberry-pi-pico-or-pico-2-and-pimoroni-pico-plus-2).

## PicoNES Mini PCB

A smaller take on the same idea by Gavin Knight ([DynaMight1124](https://github.com/DynaMight1124)), built around a Waveshare RP2350-Zero and two NES controller ports. It uses cheaper but considerably harder to solder parts, so it is a more advanced project than the PicoNES — if you are unsure of your soldering, start with that one instead. The current design is **v2.0** (`Gerber_PicoNES_Mini_PCB_v2.0.zip`), which improved the SD slot and the components around the HDMI port.

Flash `picosmsPlus_WaveShareRP2350ZeroWithPCB_arm.uf2`. The design also exists in an RP2040-Zero flavour; flash `picosmsPlus_WaveShareRP2040ZeroWithPCB_arm.uf2` for that one, but an RP2350-Zero is the better choice — see [RP2040 notes](#rp2040-notes).

> [!NOTE]
> Good soldering skills are required, especially around the HDMI portion: plenty of flux, a fine tip and solder wick. The recommended order is the resistor arrays first, then the HDMI port, then the Pico or the microSD adaptor, and the NES ports last — they can be hard to push into the PCB.

The build guide and the full component list are on Instructables: <https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/>

<img width="480" alt="Soldered PicoNES Mini PCB" src="https://github.com/user-attachments/assets/13933b1d-af00-402e-a0a0-8456de4a82da" />

### 3D printed case for the Mini

Also by Gavin Knight: [thingiverse.com/thing:7041536](https://www.thingiverse.com/thing:7041536). The same page still carries the older v1.0 PCB design files, gerber and BOM. Without a printer of your own, a local printing service or a professional one such as PCBWay or JLCPCB will produce it — the professional finishes are excellent.

<img width="480" alt="PicoNES Mini in its 3D-printed case" src="https://github.com/user-attachments/assets/732384bd-062d-43ca-97cb-a16a39607c41" />

## PicoNES Micro PCB

The smallest of the three, again by Gavin Knight: a Waveshare RP2350-USB-A board on a PCB barely larger than the USB port itself, with a single player controlling the console over USB. The current design is **v1.2** (`Gerber_PicoNES_Micro_v1.2.zip`).

Flash `picosmsPlus_WaveShare2350USBA_arm_piousb.uf2`. The game controller plugs into the USB-A port; the USB-C port is for power and for flashing the firmware.

> [!NOTE]
> There are no NES controller ports on this design, so [two player Master System games](#about-two-player-games) cannot be played on it — the driver recognises only one USB controller.

> [!NOTE]
> Because of the size, micro-soldering skills are required — the design uses 0603 SMD components. This is the most demanding of the three builds.

The build guide is on Instructables: <https://www.instructables.com/PicoNES-RaspberryPi-Pico-Based-NES-Emulator/>

<img width="480" alt="PicoNES Micro populated PCB, NES controller shown for scale" src="https://github.com/user-attachments/assets/59c8a31b-dc3e-47b0-8ffb-89e1eab2a75b" />

<img width="480" alt="PicoNES Micro in its 3D-printed case" src="https://github.com/user-attachments/assets/1d6051f2-1393-40e1-aad0-e39ffb7717a0" />

***

## Supported USB controllers
The following controllers are supported.

- BUFFALO BGC-FC801
- SONY DUALSHOCK 4
- SONY DualSense
- Xinput type controllers (Xbox)
- [Retro-bit 8 button Genesis-USB](https://www.retro-bit.com/controllers/genesis/#usb). 
- and more...

Also original NES and WII-classic controllers are supported in some configurations. 

See the [Pico-InfonesPlus sister project](https://github.com/fhoedemakers/pico-infonesPlus?tab=readme-ov-file#gamecontroller-support) for more info.

***

## About two player games

The emulator supports two player games using two NES controllers or an USB gamecontroller and a NES controller.

> [!NOTE]
> You cannot use two USB controllers for two player games.
> At the moment only one USB controller is recognized by the driver. In this case the USB controller is always player 1. Player 2 must be a NES controller.


| | Player 1 | Player 2 |
| --- | -------- | -------- |
| USB controller connected | USB | NES port 1 or NES port 2 |
| No usb controller connected | NES port 1| NES port 2 |

***

# Gamepad and keyboard usage
Below the button mapping for different controllers. You can also use a USB-keyboard.
|     | (S)NES | Genesis | XInput | Dual Shock/Sense | Wii Classic |
| --- | ------ | ------- | ------ | ---------------- | ----------- |
| Button1 | B (*) |    A    |   A    |    X             |   A (**)    |
| Button2 | A  |    B    |   B    |   Circle         |   B (**)    |
| Button3 | X (SNES only) | C | Y | Triangle    |   X         |
| Select  | select | Mode or C | Select | Select     |   Select    |

(*) On SNES USB-controller press Y once to activate the B-button.

(**) In a game, a Wii Classic pad follows the controller-port convention: A is Button1 and B is Button2. In the menu it follows the menu convention, where A chooses and B goes back — the same two buttons, labelled the other way round.

Button3 is a menu-only button — it opens the [recently played list](#recently-played-games) and does nothing in a game, so the Master System's two buttons are unaffected.

> [!NOTE]
> An original NES controller has no Button3. Everything reachable with it can also be reached from the settings menu.

## In menu
Gamepad buttons:
- UP/DOWN: Next/previous item in the menu.
- LEFT/RIGHT: next/previous page.
- Button2 : Open folder/flash and start game.
- Button1 : Back to parent folder.
- Button3 : Open the [recently played list](#recently-played-games).
- START: Show metadata and box art (when available). 
- SELECT: Opens a setting menu. Here you can change settings like screen mode, scanlines, framerate display, menu colors and other board specific settings. Settings can also be changed in-game by pressing some button combinations as explained below. The settings menu can also be opened in-game.

## Recently played games

The menu keeps a list of the **last 20 games you started**, most recent first. Open it with **Button3** in the menu, or with the **Recently played** entry at the top of the settings menu. That entry is only there when the settings menu is opened from the menu — a game cannot be started from inside a running game.

> [!NOTE]
> On an original 3-button Genesis Mini controller, C acts as SELECT and opens the settings menu instead. Take the **Recently played** entry there.

In the list:

| Button | Action |
| ------ | ------ |
| UP/DOWN | Select a game. |
| Button2 | Start the highlighted game. |
| Button1 | Close the list and return to the menu. |
| SELECT | Remove the highlighted game from the list. Asks for confirmation first. This only removes the entry, the rom on the SD card is left alone. |
| START | Show [metadata](#using-metadata) and box art (when available). |

Games are added to the list automatically when you start them, so nothing has to be enabled. Starting a game that is already in the list moves it back to the top. The list closes by itself after a minute without input.

The list is kept in **`/recent_SMS.txt`** in the root of the SD card, as plain text with one game per line. Master System, Game Gear and SG-1000 roms share the one list. It survives a reboot and can be read, edited or deleted on a PC. Deleting the file simply empties the list, and a damaged file is treated as an empty list — unlike the settings file, nothing else is reset. Each emulator running under [pico-bootLoader](https://github.com/fhoedemakers/pico-bootLoader) keeps its own list.

If a game was moved, renamed or deleted on the SD card in the meantime, the list says so instead of starting it. Use SELECT to remove such an entry.

On boards **without** PSRAM, one entry can be tagged **[READY]**. That is the game whose rom is currently written to flash, which is the one that starts without waiting for the flashing step.

## Emulator (in game)
Gamepad buttons:
- SELECT + START, Xbox button: opens the settings menu. From there, you can:
  - Quit the game and return to the SD card menu
  - Adjust settings and resume your game.
- SELECT + UP/SELECT + DOWN: switches screen modes.
- START + Button2 : Toggle framerate display
- START + DOWN : (quick) Save state. (slot 5)
- START + UP : (quick) Load state. (slot 5)
- **Pimoroni Pico DV Demo Base only**: SELECT + LEFT: Switch audio output to the connected speakers on the line-out jack of the Pimoroni Pico DV Demo Base. The speaker setting will be remembered when the emulator is restarted.
- **Fruit Jam Only** 
  - SELECT + UP: Toggle scanlines.
  - pushbutton 1 (on board): Mute audio of built-in speaker. Audio is still outputted to the audio jack. 
  - pushbutton 2 (on board) or SELECT + RIGHT: Toggles the VU meter on or off. (NeoPixel LEDs light up in sync with the music rhythm)
  - START + LEFT/RIGHT: Adjust volume of built-in speaker and external audio jack.
- **RP2350 with PSRAM only**: Record about 30 seconds of audio by pressing START to pause the game and then START + BUTTON1. Audio is recorded to **/soundrecorder.wav** on the SD-card.
- **Genesis Mini Controller**: When using a Genesis Mini 3 button controller, press C for SELECT. On the 8-button Genesis controllers, MODE acts as SELECT.
- **USB-keyboard**: When using an USB-Keyboard
  - Cursor keys: up, down, left, right
  - A: Select
  - S: Start
  - Z: Button2
  - X: Button1
  - C: Button3. In the menu this opens the [recently played list](#recently-played-games).

# Music Playback in menu (RP2350 Only)

The menu allows you to play music files. Files must meet the following requirements:

- **Format:** WAV  
- **Bit depth:** 16-bit  
- **Sample rate:** 44.1 kHz  
- **Channels:** Stereo  
- **File extension:** `.wav`  

## How to Play
1. Select a music file from the menu.
2. Press **Button2** or **START** to start playback.
3. Press **Button2** or **START** again to stop playback.

## Converting MP3 to WAV
You can easily convert MP3 files to WAV using [Audacity](https://www.audacityteam.org/):

1. Open the MP3 file in Audacity.
2. Go to **File → Export → Export Audio**.
3. Choose the following settings:
   - **Format:** WAV (Microsoft)
   - **Channels:** Stereo
   - **Sample rate:** 44,100 Hz
   - **Encoding:** Signed 16-bit PCM
4. Copy the exported WAV file to the SD card.

## Using metadata.

<img alt="Screenshot 2025-10-19 14-57-45" src="https://github.com/user-attachments/assets/da91016f-093b-4b96-8d8e-5a0f37cf2506" />

Download the metadata pack from the [releases page](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip) It contains box art, game info and themed borders/bezels for many Master System, Game Gear and SG-1000 games. The metadata is used in the menu to show box art and game info when a rom is selected.  When the screensaver is started, random box art is shown. Extra on RP2350 boards: When in-game, themed borders/bezels are shown around the game screen.

- Download pack [here](https://github.com/fhoedemakers/pico-smsplus/releases/latest/download/SMSPlusMetadata.zip).  
  - Extract the zip contents to the **root of the SD card**.  
  - In the menu:  
    - Highlight a game and press **START** → show cover art and metadata.  
    - Press **SELECT** → show full game description.  
    - Press **Button1** → return to menu.  
    - Press **START** or **Button2** → start the game.
  - **RP2350-only**: In-game: A themed bezel is shown:

| Master System | Game Gear | 
| ------------- | --------  |
| <img width="320" alt="Screenshot 2025-10-19 14-57-33" src="https://github.com/user-attachments/assets/19db1e45-0cda-442f-ac1a-3b54b7ccfa84" /> | <img width="320" alt="Screenshot 2025-10-21 17-04-42" src="https://github.com/user-attachments/assets/8f9bed95-fea8-4ce8-aa1c-edcbeacb1945" /> |


***

## Building from source

Use the bld.sh script to build the project. Build using Ubuntu Linux or WSL on Windows. See the Pico SDK installation instructions on how to set up the build environment.

Use ./bld.sh --h for options.

The resulting .uf2 file will be in the releases/ folder. Copy it to the Pico when in bootloader mode.

The emulator core can also be run on Linux without a board, for testing and debugging. See [hosttest/README.md](hosttest/README.md).

***

## Credits

This emulator is other people's work brought together on a Pico.

**Emulation**

- [SMS Plus](https://segaretro.org/SMS_Plus) by **Charles MacDonald** — the Sega Master System and Game Gear emulator core this project is built on.
- The SG-1000 (TMS9918A) video modes are ported from **SMS Plus GX** by **Charles MacDonald** and **Eke-Eke**, as included in [retro-go](https://github.com/ducalex/retro-go).
- Detection of the Taiwanese SG-1000 RAM expansion is based on [PicoDrive](https://github.com/notaz/picodrive).
- The Z80 CPU core is **Juergen Buchmueller**'s portable Z80 emulator.
- [emu2413](https://github.com/digital-sound-antiques/emu2413) by **Mitsutaka Okazaki** — YM2413 (OPLL) FM sound for the Japanese Master System games that use it.

**Video and sound output**

- [pico_hdmi](https://github.com/fliperama86/pico_hdmi) by [@fliperama86](https://github.com/fliperama86) — the HSTX driver that carries picture and sound over HDMI on the RP2350 boards, and a great deal of help along the way.
- [pico-infones](https://github.com/shuichitakano/pico-infones) by **Shuichi Takano** — the PicoDVI video path and the USB HID gamepad handling this project inherited.
- [PicoDVI](https://github.com/Wren6991/PicoDVI) by **Luke Wren** ([@Wren6991](https://github.com/Wren6991)) — the DVI-over-HDMI implementation underneath it.

**Libraries and drivers**

- [pico_fatfs](https://github.com/elehobica/pico_fatfs) by [@elehobica](https://github.com/elehobica), wrapping [FatFs](http://elm-chan.org/fsw/ff/00index_e.html) by **ChaN** — SD card access.
- [tusb_xinput](https://github.com/Ryzee119/tusb_xinput) by **Ryan Wendland** ([@Ryzee119](https://github.com/Ryzee119)) — Xbox controller support.
- [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) by [@sekigon-gonnoc](https://github.com/sekigon-gonnoc) — the second USB port on the boards that have one.
- [lwmem](https://github.com/MaJerle/lwmem) by **Tilen Majerle** ([@MaJerle](https://github.com/MaJerle)) — allocator used for the PSRAM heap.
- [PicoPlusPsram](https://github.com/AndrewCapon/PicoPlusPsram) by **Andrew Capon** ([@AndrewCapon](https://github.com/AndrewCapon)) — PSRAM detection and setup.
- The (S)NES and Wii Classic controller support goes back to work by **Phil Burgess** ([@PaintYourDragon](https://github.com/PaintYourDragon)) and **Adafruit**.

**Hardware**

- The **PicoNES PCB** was designed by **John Edgar Park** ([@johnedgarpark](https://twitter.com/johnedgarpark)).
- The **PicoNES Mini** and **PicoNES Micro** PCBs, and the 3D-printed cases for all of them, were designed by **Gavin Knight** ([DynaMight1124](https://github.com/DynaMight1124)).
- The [metadata pack](#using-metadata) — the box art, game info and themed borders/bezels on the SD card — was put together by **Gavin Knight** ([DynaMight1124](https://github.com/DynaMight1124)).
- **Murmulator M1 and M2** support was contributed by [@javavi](https://github.com/javavi).
- Thanks to [@Layer812](https://github.com/Layer812) for testing the I2S audio output and providing feedback.

**This project**

- **pico-smsplus** — the port to the Pico, the menu, the settings screen and the board support — is by **Frank Hoedemakers** ([@fhoedemakers](https://github.com/fhoedemakers)).
- The menu, settings, controller handling and board configurations are shared with [pico-infonesPlus](https://github.com/fhoedemakers/pico-infonesPlus) and the other emulators in the family through [pico_shared](https://github.com/fhoedemakers/pico_shared).
- Part of the code and documentation was written with the assistance of **[Claude Code](https://claude.com/claude-code)**, Anthropic's agentic coding tool.

***


