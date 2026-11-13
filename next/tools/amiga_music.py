#!/usr/bin/env python3
"""Render the songs as the Amiga plays them (gdriver.asm on Paula), as a reference.

The original waveforms of v6 are played as Paula does: 8-bit samples without
interpolation, period = clock ticks per sample, waveform offset/length per octave from
ptable, volume 0..64, hard stereo (voices 0 and 3 left, 1 and 2 right). The score is
driven once per vertical blank, so the tempo depends on the video standard.

Usage: amiga_music.py <MUSIC.DAT> <out-dir> [seconds] [pal|ntsc]
"""
import os
import struct
import sys
import wave

RATE = 44100
CLOCKS = {"pal": (3546895, 50), "ntsc": (3579545, 60)}
NEW_WAVE = [0x0000, 0x0000, 0x0000, 0x0000, 0x0005, 0x0202, 0x0101, 0x0103, 0x0004, 0x0504, 0x0100, 0x0500]
SONGS = ["day", "battle", "night", "intro", "astral", "indoor", "death"]
OFF_PERIODS, OFF_NOTEVALS, OFF_WAVES, OFF_ENV = 56, 224, 352, 1376

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gamedata as gd   # noqa: E402

PAN = [0, 1, 1, 0]      # 0 = left, 1 = right


def render(data, off, seconds, std, indoor_dungeon=False, speed=1):
    clock_hz, vbl = CLOCKS[std]
    w = lambda o: struct.unpack_from("<H", data, o)[0]
    waves = data[OFF_WAVES:OFF_WAVES + 1024]
    nw = list(NEW_WAVE)
    if off == 20:
        nw[10] = 0x0307 if indoor_dungeon else 0x0100
    voices = []
    for v in range(4):
        start = w((off + v) * 2)
        voices.append(dict(ptr=start, beg=start, es=0, est=0, vl=0, wave=nw[v] >> 8,
                           env=nw[v] & 0xFF, delay=-1, vol=0,
                           # Paula channel: current buffer and the one latched for the next loop
                           cur=None, nxt=None, pos=0.0, period=0))
    clock, tempo = 0, 150
    tempos = set()
    left, right = [], []
    per_frame = RATE / vbl
    acc = 0.0
    for frame in range(int(seconds * vbl)):
        clock += tempo * speed
        for v in voices:
            if not v["ptr"]:
                continue
            if clock - v["es"] >= 0:
                while True:
                    cmd, val = data[v["ptr"]], data[v["ptr"] + 1]
                    v["ptr"] += 2
                    if cmd & 0x80 and cmd != 0x80:
                        if cmd == 0x81:
                            x = nw[val & 15]
                            v["wave"], v["env"] = x >> 8, x & 0xFF
                        elif cmd == 0x90:
                            tempo = val
                            tempos.add(val)
                        elif cmd == 0xFF:
                            if val:
                                v["ptr"] = v["beg"]
                            else:
                                v["ptr"] = 0
                                v["vol"] = 0
                                break
                        continue
                    ln = w(OFF_NOTEVALS + (val & 0x3F) * 2)
                    v["est"] = v["es"] + (ln - 300 if ln > 300 else ln)
                    v["es"] += ln
                    v["vl"] = OFF_ENV + v["env"] * 256
                    vol = data[v["vl"]]
                    v["vl"] += 1
                    v["delay"] = 0
                    if cmd == 0x80:
                        v["vol"] = 0
                        v["est"] = clock
                        break
                    period, offset = gd.PTABLE[min(cmd, 83)]
                    start = v["wave"] * 128 + offset * 4
                    buf = (start, (32 - offset) * 2)
                    if v["cur"] is None:
                        v["cur"] = buf
                        v["pos"] = 0.0
                    else:
                        v["nxt"] = buf          # Paula takes it at the end of the loop
                    v["period"] = period
                    v["vol"] = vol
                    break
            elif clock - v["est"] >= 0:
                v["vol"] = 0
            elif v["delay"] == 0:
                vol = data[v["vl"]]
                if vol < 128:
                    v["vl"] += 1
                    v["vol"] = vol
        acc += per_frame
        n = int(acc)
        acc -= n
        for _ in range(n):
            mix = [0, 0]
            for i, v in enumerate(voices):
                if v["cur"] is None or not v["period"]:
                    continue
                s, ln = v["cur"]
                b = waves[s + int(v["pos"])]
                if b > 127:
                    b -= 256
                mix[PAN[i]] += b * min(v["vol"], 64)
                v["pos"] += clock_hz / v["period"] / RATE
                while v["pos"] >= ln:
                    v["pos"] -= ln
                    if v["nxt"]:
                        v["cur"], v["nxt"] = v["nxt"], None
                        s, ln = v["cur"]
            left.append(mix[0])
            right.append(mix[1])
    peak = max(1, max(map(abs, left)), max(map(abs, right)))
    out = bytearray()
    for a, b in zip(left, right):
        out += struct.pack("<hh", int(a * 30000 / peak), int(b * 30000 / peak))
    return out, tempos


def main():
    data = open(sys.argv[1], "rb").read()
    outdir = sys.argv[2]
    seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 40
    std = sys.argv[4] if len(sys.argv) > 4 else "ntsc"
    os.makedirs(outdir, exist_ok=True)
    for i, name in enumerate(SONGS):
        pcm, tempos = render(data, i * 4, seconds, std)
        path = os.path.join(outdir, f"amiga_{std}_{name}.wav")
        with wave.open(path, "wb") as f:
            f.setnchannels(2)
            f.setsampwidth(2)
            f.setframerate(RATE)
            f.writeframes(pcm)
        print(f"{path}: tempo events {sorted(tempos) or '-'}")


if __name__ == "__main__":
    main()
