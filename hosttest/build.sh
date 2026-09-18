#!/usr/bin/env bash
# Build the host harness: the unmodified smsplus core plus the FatFs shim.
# Uses the same core defines as CMakeLists.txt (SMS_FLAGS). Without PICO_BOARD
# and PICO_RP2350, in_ram expands to nothing and emu2413 is left out, which
# matches the RP2040 build. Drop -fsanitize=address for faster runs.
set -euo pipefail
cd "$(dirname "$0")/.."   # repo root

cc -O1 -g -fsanitize=address -fno-omit-frame-pointer \
  -DMB_SMS -DLSB_FIRST=0 -DNDEBUG \
  -I hosttest/shim -I smsplus \
  -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable \
  -Wno-pointer-sign -Wno-sign-compare \
  smsplus/*.c hosttest/host_main.c hosttest/fatfs_host.c \
  -lm -o hosttest/sms_host

echo "built hosttest/sms_host"
