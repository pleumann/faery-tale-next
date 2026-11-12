#!/bin/sh
# Start CSpect with the SD root, wait, capture only the CSpect window, quit.
# Never captures the whole screen: without a window id nothing is captured.
# Usage: tools/cspect_shot.sh <sd-root> <out.png> [seconds]
SD="$1"; OUT="$2"; WAIT="${3:-6}"
CSPECT="${CSPECT:-$HOME/Retro/CSpect4_5_1_0/CSpect.exe}"
TOOLS="$(cd "$(dirname "$0")" && pwd)"
OUT_ABS="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"
SD_ABS="$(cd "$SD" && pwd)/"; cd "$SD" || exit 1
mono "$CSPECT" -w2 -zxnext -s28 -mmc="$SD_ABS" -nodelay fta.nex >/dev/null 2>&1 &
PID=$!
sleep "$WAIT"
WID=$(swift "$TOOLS/winid.swift" "$PID" 2>/dev/null)
if [ -n "$WID" ]; then
    screencapture -x -o -l"$WID" "$OUT_ABS"
else
    echo "CSpect window not found - no screenshot taken" >&2
fi
kill "$PID" 2>/dev/null
