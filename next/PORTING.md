# Porting The Faery Tale Adventure to the ZX Spectrum Next

This is a report on how the Next port in this directory came about: the approach, the
technical decisions, the milestones, what went wrong and what is worth knowing if you
attempt something similar.

**Who did what.** The port was written by Claude, Anthropic's AI model, working in Claude
Code over the course of two days: an afternoon for the port itself, a morning for
documentation and polish, including finding the music bug. The owner of this repository set the goal, made the decisions where
there were real alternatives, tested on CSpect and a real Next, and reported what looked
or sounded wrong. Everything in `next/` – code, tools, build, documentation – was
written by the AI; the text below is its own account, so "I" refers to Claude.

## Starting point

The original is about 5,300 lines of Aztec C (`fmain.c`, `fmain2.c`) and 3,000 lines of
68000 assembler (`fsubs.asm`, `gdriver.asm`, `narr.asm`) from 1986/87, written for a
512K Amiga. Talin describes it himself as written in a hurry, with few comments. It is
deeply tied to the hardware: five-bitplane playfield with blitter scrolling, software
sprites with background save/restore, a hires status panel in its own viewport,
copper-driven palettes, Paula playing sampled waveforms, an input.device handler, and
the game data read as raw blocks from the floppy.

The Next has a 28 MHz Z80N with a 64K address space (eight MMU slots of 8K), 1 or 2 MB
of RAM, Layer 2 bitmaps, 128 hardware sprites, a tilemap, a copper, three AY chips, DACs
and a DMA. On paper that is enough for the game; the work is in mapping one onto the
other.

## Decisions

The repository owner chose a 1 MB Next as the baseline, graphics as faithful as
possible, music on the AY first, and the port in a separate directory. From there:

- **Playfield: Layer 2, 320×256, 8 bpp, used as a torus.** The Amiga viewport is
  288×140 with 32 colours; the 16×32 tiles are converted to 8 bpp. Layer 2 has
  hardware scrolling in both directions and is 20×8 tiles large, one more column and two
  more rows than visible. So scrolling is just the scroll registers plus drawing the new
  column or row in the part that is off screen – no copying, no tearing, no double
  buffer.
- **Figures: hardware sprites with the occlusion computed into them.** The Amiga draws
  figures with the blitter and masks them with per-tile "shadow" masks so they disappear
  behind trees and walls. The port composes each figure in a buffer, applies the same
  masks with the original rules (seven mask modes, e.g. "only the right column", "only if
  the feet are below the tile", "bridge"), and uploads the result as 16×16 sprite
  patterns. Patterns are double buffered.
- **Status panel: the 80-column tilemap.** The panel is 640 pixels wide; the tilemap in
  80-column mode covers exactly that. The panel graphics are deduplicated into 171 tiles
  (with mirrored variants), the compass highlights are precomputed per direction. Text
  uses the Spectrum ROM font – the repository owner's suggestion – with one character
  per tile and the colours selected through tilemap palette offsets, so 96 glyph tiles
  serve every colour combination of the menus.
- **Game logic: ported, not rewritten.** `fmain.c` and `fmain2.c` are translated into
  ANSI C keeping the structure, names and quirks, with comments pointing to the original
  lines. Original bugs that could crash or write past arrays are fixed; the others are
  kept (and marked).
- **Code size: banking.** The ported code does not fit into the 32K above 0x8000. Ten
  modules (logic, doors, objects, actor loop, sprites, combat, menus, audio, placards,
  intro) are linked into 8K pages at address 0 and run in MMU slot 0, called through
  small resident wrappers.
- **Data: converted at build time.** A Python converter reads the Amiga disk image
  block by block and the IFF pictures, and writes Next formats plus PNG previews. Nothing
  derived is checked in.

## Milestones

**M0 – skeleton.** NEX build with z88dk, IM2 frame interrupt, Layer 2 in 320×256, palette
fades from Amiga RGB4 values, loading from the SD card. First result: the intro picture
fading in.

**M1 – converter.** Disk image reader, planar-to-chunky conversion, IFF/ILBM with
ByteRun1, the hunk format of the Amber font, the texts of `narr.asm` (a small 68000
assembler data parser), music and samples. The PNG previews made it possible to check
everything before writing Next code: a 2048×1024 overview of the whole world, the start
village at full resolution, all figure sets, the font, the panel as round trip.

**M2 – map viewer.** Region loading as in `load_new_region`, the tile lookup of
`_genmini` including its clamping at region borders, the torus scrolling. Verified by
rendering reference views in Python from the disk data and comparing them with CSpect
screenshots.

**M3 – the hero.** Hardware sprites, walking animation, collision (`prox`, `proxcheck`
with the original's deflection around obstacles), terrain effects, occlusion, the camera
of `map_adjust`.

**M4 – game logic.** The actor loop for all types, enemy AI (goals and tactics), melee
and missiles, encounters, objects and set figures, doors and keys, day/night, hunger,
fatigue, death and revival. Banking was introduced here.

**M5 – user interface.** Tilemap panel, messages, status line, menus with `do_option`,
inventory picture, compass, keyboard scanner.

**M6 – music and sound.** The score player of `gdriver.asm` on the AY chips, the moods of
`setmood`, a WAV preview tool to listen to the conversion on the host.

**M7 – the rest of the game.** Save/load in an own format (the original wrote raw memory
blocks whose layout depended on the compiler), placards with the meander border and the
Amber font, intro with the book pages, the princess, the victory, the magic map, the
witch, Kempston mouse.

**M8 and later – polish.** Profiling and speed-up, music in the interrupt, loading the
next region ahead, the single-file 2 MB build, sound effects through the DAC.

## How I tested

I could not play the game – no keyboard or joystick input into the emulator, and no
sound. Everything had to be verified indirectly:

- **Reference renders from the original data.** For the map and tiles, Python renders
  views straight from the disk image; CSpect screenshots of the same coordinates must
  match pixel for pixel.
- **Screenshots of the CSpect window**, taken by a script with a fixed delay after start.
  Test builds (`-DTEST_…`) put the hero at specific places or press keys by script, so a
  screenshot shows a defined situation: a door threshold, a region border, the princess's
  cell, the bridge.
- **State dumps.** A debug build writes the actor list, AY registers and time stamps in
  scan lines to a file on the emulated SD card, which a Python script then evaluates.
  This is how the flickering enemies, the music and the frame budget were analysed.
- **The repository owner** tested in CSpect (including the mouse) and on a real Next,
  and listened. Several of the problems below were found that way.

## Problems worth knowing

**SDCC and masks.** Enemy files hold two races in alternating frames (ogre/orc, …); the
original picks the parity per enemy with `inum &= 0xfffe`. On an `int16_t` this had no
effect in the generated code, so enemies flickered between two costumes. Found from the
owner's remark that the robbers looked "flickery", confirmed with a state dump, fixed
with an 8-bit value.

**A 200-second compile.** The actor loop is one huge function with many gotos in the
original. SDCC's register allocator needed over three minutes for it regardless of
optimisation settings. Split into step functions – the original jump targets became
return codes – it compiles in five seconds; together with a proper per-file Makefile a
full build takes 15 s.

**Strings across banks.** Banked code in slot 0 passes string literals that live in its
own page. When a file call maps the ROM into slot 0 for esxDOS, or another bank is
mapped, the string is gone. File names are therefore copied to resident memory first;
text output is resident.

**The tilemap and the border.** In CSpect the tilemap was only shown over the 256×192
paper area, not over the border, until the ULA was switched on again with a fully
transparent palette – a hint found in CSpect's change log.

**The DMA that wrote into memory.** The first DMA program for the sound effects made
CSpect write into memory instead of to the DAC, destroying the panel font during fights.
Using the reset and timing-reset sequence of CSpect's own DMA example fixed it. Found by
building variants with and without the DMA and comparing screenshots.

**A lost variable.** `_genmini` also computes the sector the hero stands in. The port
lost that while restructuring, and nothing seemed wrong – until the owner noticed the
hero being covered by a bridge: the bridge's occlusion rule depends on that sector. Place
names, drowning in a particular sector and the stone ring teleport depended on it too.

**The music.** For a long time the music sounded slow and somewhat off, and the bass
in particular seemed out of tune. Attempts with an envelope-generator "buzzer bass" and a
bass one octave up only treated the symptoms. The cause turned up when the owner of this
repository compared the port with a longplay recording of the Amiga version and found it
at least five times too slow: the pitch table in `MUSIC.DAT` had 78 entries, the layout
reserved 84, so everything after it (note durations, waveforms, envelopes, track
offsets) was read 12 bytes off. Durations came from the wrong table row, and each track
lost its tempo command, its instrument and its first two notes. The converter now pads
the table and checks the layout. `tools/amiga_music.py` renders the songs the way Paula
plays them and served as the reference. The game was made for NTSC Amigas, so at 50 Hz
the player advances the score at 6/5 of the tempo.

**Frame rate.** With several enemies on screen the game stuttered. Profiling in scan
lines showed the sprite composition taking two thirds of the budget; uploading patterns
with `OTIR`, applying masks through row pointers, 16-bit distance math and caching the
day/night palette brought about 90 % of the loops to the full 25 Hz.

**Interrupts and the memory map.** Moving the music player into the interrupt meant the
interrupt has to map pages. Every Next register access became a nesting critical section
(selecting a register and writing it are two port accesses), the interrupt saves and
restores the slots, and it stays out of the way during esxDOS calls, when the ROM and
DivMMC must stay mapped.

**Screenshots and privacy.** The first screenshot script captured the whole screen and
once caught an unrelated window of the owner's desktop. It was deleted at once; since
then only the CSpect window is captured, found by its title, and nothing is captured if
the window is not found.

## Differences from the original

- Fixed: writes past the inventory arrays (`stuff[ARROWBASE]`), an out-of-range read in
  the AI (`EVADE`), the never-seeded random number generator (seeded from the frame
  counter when the intro ends).
- Removed: the copy protection questions and disk checks, disk swapping.
- Different by necessity: the main loop runs at a fixed 25 Hz; text lines in the message
  area are 8 instead of 10 pixels apart; the title uses the Amber font instead of the
  ROM's topaz; the witch's gaze is drawn as an outline, not filled; the page turn in the
  intro is a cut, not the animated flip.
- Music on the AY with square waves; sound effects are the original
  samples on the DAC.
- Controls: cursor keys and `0` instead of the numeric keypad, SYMBOL SHIFT + 1–7
  instead of F1–F7, digit keys in submenus that were mouse-only on the Amiga, and – as an
  addition – click to walk with the mouse.
- A 2 MB build that holds everything in memory, and prefetching of map data in the 1 MB
  build.

## Open ends

- Not yet seen working: turtle, swan, dragon, locked doors with keys, buying and giving,
  the victory sequence.
- A side-by-side test against the original in a scriptable Amiga emulator: record seed,
  input and actor state per main loop iteration on the Amiga, replay them in the port
  and report the first difference.

## Credits

The Faery Tale Adventure was written by David Joiner (Talin) and published by
MicroIllusions in 1987; see [`../orig/README.md`](../orig/README.md) for his notes on the
code and its copyright status.
