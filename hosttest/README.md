# hosttest — Linux host harness for the smsplus core

Runs the unmodified `smsplus/` core (Master System, Game Gear and SG-1000)
headless on a Linux PC and dumps frames as images. Useful for checking
rendering, memory mapping, save states and heap use without flashing a board.
HDMI/DVI output, SD card, PSRAM timing and audio still need the device.

## Layout

| File | Purpose |
|---|---|
| `host_main.c` | main loop; implements the callbacks `main.cpp` normally provides (`sms_render_line`, palette sync, `frens_f_malloc/free`); PPM dumps, input script, save-state round trip, heap accounting |
| `shim/ff.h`, `fatfs_host.c` | the part of FatFs the core uses for save states, on top of stdio |
| `build.sh` | builds `hosttest/sms_host` with AddressSanitizer and the core defines from `CMakeLists.txt` |
| `run_sg_all.sh` | runs every `.sg` ROM under a directory in parallel |
| `sg_report.py` | writes `summary.txt` and an `index.html` thumbnail grid for a `run_sg_all.sh` run |
| `ppm2png.py` | PPM → PNG converter, Python standard library only |

The build defines neither `PICO_BOARD` nor `PICO_RP2350`, so `in_ram` expands to
nothing and the YM2413 is left out — the same core configuration as RP2040.

## Build

From the repository root:

```sh
hosttest/build.sh   # produces hosttest/sms_host
```

## Run one ROM

```sh
./hosttest/sms_host <rom> <frames> <dump-every> [-o dir] [-b frame] [-l last.ppm] [-i script] [-s frame] [-r] [-v vram.bin] [-V]
python3 hosttest/ppm2png.py "hosttest/out/*.ppm"
```

- The cart type follows the file extension, case-sensitive as in the firmware:
  `.gg` Game Gear, `.sg` SG-1000, anything else Master System.
- `dump-every` writes `frame_NNNNN.ppm` to the `-o` directory (default
  `hosttest/out`); 0 disables it. `-b` delays the first periodic dump to the
  given frame. `-l` writes the last frame.
- `-i` holds inputs for player 1, e.g. `B1:300-320,PAUSE:900`. Names: `UP DOWN
  LEFT RIGHT B1 B2 PAUSE START`. A single frame number holds for 6 frames.
- `-s F` saves a state at frame F, tears the emulator down, initialises it again
  and loads the state, as `Emulator_LoadState` in `state.cpp` does.
- `-r` prints the VDP registers and status after the last frame.
- `-v FILE` writes the 16 KB VRAM after the last frame; `-V` also writes
  `vram_NNNNN.bin` next to every periodic frame dump, e.g. to check a frame's
  sprite attribute table against what was drawn. VRAM dumped after frame N is
  what frame N+1 is drawn from.

The core's allocations go through a counting `frens_f_malloc`, which aborts on
failure like the Pico SDK. The peak is printed at exit. With
`HOST_HEAP_LIMIT=<bytes>` it aborts as soon as the limit would be exceeded.

SG-1000 RAM-adaptor detections are logged with the triggering write and frame.

## Run all SG-1000 ROMs

```sh
HOST_HEAP_LIMIT=81340 hosttest/run_sg_all.sh [romdir] [frames] [input-script] [state-frame]
```

Defaults: `/home/frank/roms/SMS/SG`, 900 frames, button 1 pressed at frames
300, 420, 540 and 700, and a state round trip at frame 600. 81340 bytes is the
heap peak of a Master System session. Results are in `hosttest/out/sg/`:
`summary.txt` lists failures (non-zero exit, ASan report, heap panic, state
round-trip failure) and RAM-adaptor detections, and `index.html` shows the last
frame of every ROM.
