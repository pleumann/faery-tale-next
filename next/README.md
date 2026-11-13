# The Faery Tale Adventure – ZX Spectrum Next port

A port of the 1987 Amiga game to the ZX Spectrum Next, built from the original sources
and data in [`../orig`](../orig). The game logic of `fmain.c`/`fmain2.c` is ported
largely line by line to C; graphics, sound, input and file access are rewritten for the
Next's hardware. How the port came about is described in [PORTING.md](PORTING.md).

## Status

The complete game is playable: intro, the three brothers, the whole world with all
regions and interiors, enemies and their AI, combat, objects, doors and keys, menus,
inventory, magic items, the magic map, the witch, the princess, day and night, hunger
and fatigue, music and sound effects, save/load and the ending.

Tested in CSpect and on a real Next. Some parts have not been seen working yet, e.g. the turtle, the swan, the dragon, locked doors and the
victory sequence; see [PORTING.md](PORTING.md) for details and known differences.

## Two builds

| Build | Files | Machine |
|---|---|---|
| `build/sd2m/fta.nex` | one file, all data inside (1.4 MB) | Next with 2 MB |
| `build/sd/fta.nex` + `build/sd/FTA/` | program and data folder | any Next (1 MB) |

The 2 MB build packs the data into RAM pages; nothing is loaded from the SD card during
the game, savegames go next to the NEX (`FTASAVEA.DAT` … `FTASAVEH.DAT`). The 1 MB build
loads from the `FTA` folder (savegames `FTA/SAVEA.DAT` …) and loads the tiles of the
neighbouring region ahead while the hero walks towards a region border.

## Building

Requirements: [z88dk](https://z88dk.org) (`zcc` in `PATH`) and Python 3 (standard library
only). For running on the host: CSpect with mono.

```
make -j8                         # both builds (about 15 s, incremental afterwards)
make -j8 build/sd2m/fta.nex      # only the 2 MB build
make run2m                       # start the 2 MB build in CSpect
make run                         # start the 1 MB build in CSpect
make music-preview               # render the songs as WAV (build/preview)
```

On the Next copy `build/sd2m/fta.nex` anywhere to the SD card (2 MB), or the contents of
`build/sd` – `fta.nex` together with the `FTA` folder – into one directory (1 MB).

The converter also writes PNG previews of all graphics (tile banks, the whole world map,
figures, font, panel) to `build/preview`.

## Controls

| Key | Action |
|---|---|
| cursor keys (CAPS SHIFT + 5–8), Kempston joystick | walk (8 directions) |
| `0`, fire | fight (bow: aim while held, shoot on release) |
| `I` / `T` / SYMBOL SHIFT + `Z` | inventory / take (also search bodies and chests) / look (`?`) |
| `U`, `G` | use menu, give menu (in it `1`–`4`: gold, book, writ, bone) |
| `1`–`7`, `K` | use: dirk, mace, sword, bow, wand, shell, lasso; key menu (`1`–`6`) |
| `8` / `9` in the use menu | sunstone / book |
| `Y` / `S` / `A` | yell / say / ask |
| `O` `R` `8` `C` `W` `B` `E` | buy food, arrows, vial, mace, sword, bow, totem (at the inn) |
| SYMBOL SHIFT + `1`–`7` | magic items (the Amiga's F1–F7) |
| SPACE / `M` / `F` | pause / music / sound effects (SPACE also skips the intro) |
| `Q`, then `V` / `X` | save / quit (quitting resets the Next into the menu) |
| `L` | load; in the file menu `1`–`8` select slot A–H |
| SYMBOL SHIFT + `D` | summon a band of ogres (only in the `DEMO` and test builds) |

Kempston mouse: a left click on menu entries works as on the Amiga; holding the left
button on the compass walks, the right button fights. As an addition to the original, a
left click or hold on the playfield walks there.

## Layout

- `tools/` – host tools in Python: Amiga disk image and IFF readers, the asset converter
  (`convert.py`), `narr2c.py` for the texts of `narr.asm`, `memfs.py` for the 2 MB build,
  the music preview and the CSpect screenshot helper used for testing
- `src/platform/` – Next hardware: MMU, banking, IM2 interrupt, Layer 2, files (esxDOS or
  RAM), keyboard, joystick, mouse
- `src/game/` – the game: world and map, view (scrolling), actors and AI, combat, sprites,
  objects, doors, menus, status panel and texts, placards, intro, audio, save/load
- `src/main.c` – start-up and the main loop

## Memory map

| Slot | Address | Use |
|---|---|---|
| MMU0 | 0000–1FFF | banked code (pages 80–89); the ROM during esxDOS calls |
| MMU1 | 2000–3FFF | short-term data windows (map, texts, music, font, samples) |
| MMU2–3 | 4000–7FFF | bank 5: tilemap (map at 4000, tiles at 4600); short-term data windows |
| MMU4–7 | 8000–FCFF | resident code and data, stack below FD00 |
| | FD00–FEFF | IM2 vector table and interrupt jump |

Banked modules (8K pages at 0x0000, called through resident wrappers in `glue.c`):
80 logic (world tick, AI, encounters, revival), 81 doors, 82 objects, 83 actor loop,
84 sprites, 85 combat and depth sort, 86 menus, 87 audio, 88 placards and map,
89 intro, rescue and victory.

| Pages | Use |
|---|---|
| 18–27 / 28–37 | Layer 2 buffer A (game) / B (intro double buffering) |
| 56–60 | sectors (writable copy, doors change them), region map, terrain |
| 61–62 | shadow masks |
| 73 | sprite compose buffer |
| 74–78 | texts, music, Amber font |
| 1 MB build | tile bank cache 40–55, 2–3, 6–9, 12–13, 16–17, 90–95; figure sets 28–39, 64–72; samples 63 |
| 2 MB build | data files packed by `tools/memfs.py` into the free pages, tiles and figures used in place |

Sprites: two halves of the pattern memory (0–30, 32–62) alternate per frame; pattern 63
and sprite 127 are the mouse pointer.

## Asset files

Generated by `tools/convert.py` from `../orig/game` (all multi-byte values little-endian):

| File | Content |
|---|---|
| TILES.DAT | 17 banks × 64 tiles, 16×32 px, 8 bpp column-major (512 bytes per tile) |
| SECTORS.DAT | sector tile bytes: outdoor and indoor (2 × 32K) |
| MAPS.DAT | region maps, 128×32 sector numbers: 4 outdoor bands and indoors |
| TERRA.DAT | 11 terrain tables, 4 bytes per tile |
| SHADOW.DAT | 192 occlusion masks 16×32×1 |
| SHAPES.DAT | index + figure sets as 16×16 cells, 8 bpp, colour 31 transparent |
| SAMPLES.DAT | 6 sound effects, unsigned 8-bit PCM |
| MUSIC.DAT | track offsets, AY periods, note durations, waveforms, envelopes, tracks |
| NARR.DAT | all texts of `narr.asm` (offsets in the generated `narr.h`) |
| FONT.DAT | the Amber/9 font |
| PAGE0, WINPIC .L2/.PAL | intro and victory pictures, 320×256 Layer 2 |
| P1A … P3B.BIN | intro book pages |
| HISCR .TIL/.MAP/.CMP/.PAL | status panel as 8×8 4bpp tiles, map, compass variants, palette |

## Test builds

| Define | What it does |
|---|---|
| `DEMO` | no intro; the hero fights summoned ogres at the stone ring |
| `TEST_FEATURES` | shows the magic map, then visits the witch |
| `TEST_RESCUE` | walks into the princess's cell |
| `TEST_REGION` | walks across a region border (prefetching) |
| `TEST_BRIDGE` | starts on the bridge west of Tambry |
| `TEST_DOOR` | starts on a door threshold |
| `TEST_KEYS` | presses keys by script (inventory) |
| `DEBUG_DUMP` | writes actor state, timing in scan lines and AY registers to `DEBUG.BIN` |
| `SHOT_X`, `SHOT_Y`, `SHOT_REGION` | starts at that world position and region (used for the screenshots) |

If a file cannot be loaded, the screen turns into the ULA display with a coloured border
and the esxDOS error number as 8 blocks (white = 1, MSB left).

## License

The port's code is under the MIT license, see [`LICENSE`](LICENSE). The original code and
game data in `../orig/` are not covered; see Talin's statement in
[`../orig/README.md`](../orig/README.md).
