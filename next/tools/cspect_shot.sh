#!/bin/sh
# Start CSpect with the SD root, wait, take a screenshot of the CSpect window, quit.
# Usage: tools/cspect_shot.sh <sd-root> <out.png> [seconds]
SD="$1"; OUT="$2"; WAIT="${3:-6}"
CSPECT="${CSPECT:-$HOME/Retro/CSpect4_5_1_0/CSpect.exe}"
OUT_ABS="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"
SD_ABS="$(cd "$SD" && pwd)/"; cd "$SD" || exit 1
mono "$CSPECT" -w2 -zxnext -s28 -mmc="$SD_ABS" -nodelay fta.nex >/dev/null 2>&1 &
PID=$!
sleep "$WAIT"
screencapture -x -o "$OUT_ABS"
kill "$PID" 2>/dev/null
