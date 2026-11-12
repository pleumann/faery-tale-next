#!/usr/bin/env python3
"""Render the songs of MUSIC.DAT as WAV files, using the same rules as src/game/audio.c
(AY square tones, envelopes, tempo clock), to listen to the conversion on the host.

Usage: music_preview.py <MUSIC.DAT> <out-dir> [seconds]
"""
import os
import struct
import sys
import wave

RATE = 22050
AY_CLOCK = 1750000
NEW_WAVE = [0x0000, 0x0000, 0x0000, 0x0000, 0x0005, 0x0202, 0x0101, 0x0103, 0x0004, 0x0504, 0x0100, 0x0500]
VOL_AY = [0, 3, 5, 6, 7, 8, 8, 9, 9, 10, 10, 10, 10, 11, 11, 11,
          11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
          13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14,
          14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15]
SONGS = ["day", "battle", "night", "intro", "astral", "indoor", "death"]
OFF_PERIODS, OFF_NOTEVALS, OFF_ENV = 56, 224, 1376


def ay_amp(level):
    return 0 if level == 0 else 2 ** ((level - 15) / 2)


def render(data, off, seconds, indoor_dungeon=False):
    w = lambda o: struct.unpack_from("<H", data, o)[0]
    nw = list(NEW_WAVE)
    if off == 20:
        nw[10] = 0x0307 if indoor_dungeon else 0x0100
    voices = []
    for v in range(4):
        start = w((off + v) * 2)
        voices.append(dict(ptr=start, beg=start, es=0, est=0, vl=0, wave=nw[v] >> 8,
                           env=nw[v] & 0xFF, delay=-1, period=0, vol=0))
    clock, tempo = 0, 150
    out = bytearray()
    phases = [0.0] * 4
    for frame in range(seconds * 50):
        clock += tempo
        for v, vc in enumerate(voices):
            if not vc["ptr"]:
                continue
            if clock - vc["es"] >= 0:
                while True:
                    cmd, val = data[vc["ptr"]], data[vc["ptr"] + 1]
                    vc["ptr"] += 2
                    if cmd & 0x80 and cmd != 0x80:
                        if cmd == 0x81:
                            vc["wave"], vc["env"] = nw[val & 15] >> 8, nw[val & 15] & 0xFF
                        elif cmd == 0x90:
                            tempo = val
                        elif cmd == 0xFF:
                            if val:
                                vc["ptr"] = vc["beg"]
                            else:
                                vc["ptr"] = 0
                                vc["vol"] = 0
                                break
                        continue
                    ln = w(OFF_NOTEVALS + (val & 0x3F) * 2)
                    vc["est"] = vc["es"] + (ln - 300 if ln > 300 else ln)
                    vc["es"] += ln
                    vc["vl"] = OFF_ENV + vc["env"] * 256
                    vol = data[vc["vl"]]
                    vc["vl"] += 1
                    vc["delay"] = 0
                    if cmd == 0x80:
                        vc["vol"] = 0
                        vc["est"] = clock
                    else:
                        vc["period"] = w(OFF_PERIODS + min(cmd, 83) * 2)
                        vc["vol"] = min(vol, 64)
                    break
            elif clock - vc["est"] >= 0:
                vc["vol"] = 0
            elif vc["delay"] == 0:
                vol = data[vc["vl"]]
                if vol < 0x80:
                    vc["vl"] += 1
                    vc["vol"] = min(vol, 64)
        for _ in range(RATE // 50):
            s = 0.0
            for v, vc in enumerate(voices):
                if vc["vol"] and vc["period"]:
                    f = AY_CLOCK / (16 * vc["period"])
                    phases[v] = (phases[v] + f / RATE) % 1.0
                    s += ay_amp(VOL_AY[vc["vol"]]) * (1 if phases[v] < 0.5 else -1)
            out += struct.pack("<h", int(max(-1, min(1, s / 3)) * 30000))
    return out


def main():
    data = open(sys.argv[1], "rb").read()
    outdir = sys.argv[2]
    seconds = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    os.makedirs(outdir, exist_ok=True)
    for i, name in enumerate(SONGS):
        pcm = render(data, i * 4, seconds)
        with wave.open(os.path.join(outdir, f"music_{name}.wav"), "wb") as f:
            f.setnchannels(1)
            f.setsampwidth(2)
            f.setframerate(RATE)
            f.writeframes(pcm)
        print(f"  music_{name}.wav")


if __name__ == "__main__":
    main()
