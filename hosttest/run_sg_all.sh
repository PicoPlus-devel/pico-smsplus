#!/usr/bin/env bash
# Run every .sg ROM under a directory through sms_host, in parallel, then
# write hosttest/out/sg/summary.txt and index.html (thumbnail grid of the
# last frame of each ROM).
#
# Usage: hosttest/run_sg_all.sh [romdir] [frames] [input-script] [state-frame]
# Each run saves and reloads a state at state-frame (default 600, 0 = off).
# HOST_HEAP_LIMIT is passed through to sms_host.
set -uo pipefail
cd "$(dirname "$0")/.."   # repo root

ROMDIR=${1:-/home/frank/roms/SMS/SG}
FRAMES=${2:-900}
SCRIPT=${3:-B1:300,B1:420,B1:540,B1:700}
STATE=${4:-600}
OUT=hosttest/out/sg

[ -x hosttest/sms_host ] || hosttest/build.sh
rm -rf "$OUT"
mkdir -p "$OUT/log"
export FRAMES SCRIPT STATE OUT

find "$ROMDIR" -type f -name '*.sg' -print0 | sort -z |
xargs -0 -P "$(nproc)" -I{} bash -c '
	rom="$1"
	id=$(printf "%s" "$rom" | md5sum | cut -c1-10)
	state=()
	[ "$STATE" -gt 0 ] && state=(-s "$STATE")
	mkdir -p "$OUT/log/$id"
	{
		echo "rom: $rom"
		./hosttest/sms_host "$rom" "$FRAMES" 0 -o "$OUT/log/$id" -l "$OUT/$id.ppm" -i "$SCRIPT" "${state[@]}"
		echo "exit: $?"
	} > "$OUT/log/$id.txt" 2>&1
	rm -rf "$OUT/log/$id"
' _ {}

python3 hosttest/ppm2png.py "$OUT/*.ppm" > /dev/null && rm -f "$OUT"/*.ppm
python3 hosttest/sg_report.py "$OUT"
