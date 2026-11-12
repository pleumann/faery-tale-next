# The Faery Tale Adventure – ZX Spectrum Next port

Work in progress. The original Amiga sources in the parent directory are left untouched;
this directory holds the port.

## Build

Requirements: z88dk (`zcc` in `PATH`), Python 3 (no extra modules), for running: CSpect + mono.

```
make          # converts ../game data into build/sd/FTA and builds build/sd/fta.nex
make run      # starts CSpect with build/sd as SD card root
```

On real hardware copy the contents of `build/sd` (fta.nex and the FTA folder) to the same
directory on the SD card and start fta.nex. Data paths are relative to that directory.

## Layout

- `tools/` – host tools: IFF reader, asset converter, PNG previews (`build/preview`)
- `src/platform/` – Next hardware layer (MMU, IM2 interrupt, Layer 2, palette, esxDOS files)
- `src/` – game code

## Memory map

| Slot | Address | Use |
|---|---|---|
| MMU0–1 | 0000–3FFF | ROM (needed for esxDOS calls); later banked code |
| MMU2 | 4000–5FFF | data window for paged assets |
| MMU4–7 | 8000–FCFF | resident code + data, stack below FD00 |
| | FD00–FEFF | IM2 vector table and interrupt jump |

Layer 2 buffers: 16K banks 9–13 (pages 18–27) and 14–18 (pages 28–37).

## Error codes

If a file cannot be loaded, the screen turns into the ULA display with a coloured border
and the esxDOS error number as 8 blocks (white = 1, MSB left).
