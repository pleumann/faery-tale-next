"""Tables copied from the original sources, needed by the converter."""

# fmain2.c:367 - playfield palette (RGB4); colour 31 is changed per region at runtime
PAGECOLORS = [
    0x0000, 0x0FFF, 0x0E96, 0x0B63, 0x0631, 0x07BF, 0x0333, 0x0DB8,
    0x0223, 0x0445, 0x0889, 0x0BBC, 0x0521, 0x0941, 0x0F82, 0x0FC7,
    0x0040, 0x0070, 0x00B0, 0x06F6, 0x0005, 0x0009, 0x000D, 0x037F,
    0x0C00, 0x0F50, 0x0FA0, 0x0FF6, 0x0EB6, 0x0EA5, 0x000F, 0x0BDF,
]

# fmain.c:476 - status panel palette (16 colours + 4 for the pointer sprite)
TEXTCOLORS = [
    0x000, 0xFFF, 0xC00, 0xF60, 0x00f, 0xc0f, 0x090, 0xFF0,
    0xf90, 0xf0c, 0xA50, 0xFDB, 0xEB7, 0xCCC, 0x888, 0x444,
    0x000, 0xDB0, 0x740, 0xC70,
]

# fmain.c:484 - palette of the intro pages
INTROCOLORS = [
    0x000, 0xFFF, 0xE00, 0xA00, 0xD80, 0xEC0, 0x390, 0x021,
    0xEEB, 0xEDA, 0xEEA, 0xCB8, 0xA95, 0x973, 0x840, 0x620,
    0xA52, 0xC74, 0xD96, 0xFCA, 0x449, 0x444, 0xDC9, 0x668,
    0x33F, 0x888, 0xA60, 0xAAF, 0xBBB, 0xCCF, 0xDDD, 0xEEE,
]

# fmain.c:615 - struct need { image[4], terra1, terra2, sector, region, setchar }
FILE_INDEX = [
    ((320, 480, 520, 560), 0, 1, 32, 160, 22),   # F1 - snowy region
    ((320, 360, 400, 440), 2, 3, 32, 160, 21),   # F2 - witch wood
    ((320, 360, 520, 560), 2, 1, 32, 168, 22),   # F3 - swampy region
    ((320, 360, 400, 440), 2, 3, 32, 168, 21),   # F4 - plains and rocks
    ((320, 480, 520, 600), 0, 4, 32, 176, 0),    # F5 - desert area
    ((320, 280, 240, 200), 5, 6, 32, 176, 23),   # F6 - bay / city / farms
    ((320, 640, 520, 600), 7, 4, 32, 184, 0),    # F7 - volcanic
    ((320, 280, 240, 200), 5, 6, 32, 184, 24),   # F8 - forest and wilderness
    ((680, 720, 800, 840), 8, 9, 96, 192, 0),    # F9 - inside of buildings
    ((680, 760, 800, 840), 10, 9, 96, 192, 0),   # F10 - dungeons and caves
]

TERRA_BLOCK = 149
TILE_BANK_FIRST = 200       # 17 banks of 40 blocks: 200..879
TILE_BANKS = 17

# fmain2.c:643 - struct cfile_info { width (words), height, count, numblocks, seq_num, file_id }
CFILES = [
    ("julian",   1, 32, 67, 42, "PHIL", 1376),
    ("phillip",  1, 32, 67, 42, "PHIL", 1418),
    ("kevin",    1, 32, 67, 42, "PHIL", 1460),
    ("objects",  1, 16, 116, 36, "OBJECTS", 1312),
    ("raft",     2, 32, 2, 3, "RAFT", 1348),
    ("turtle",   2, 32, 16, 20, "CARRIER", 1351),
    ("ogre",     1, 32, 64, 40, "ENEMY", 960),
    ("ghost",    1, 32, 64, 40, "ENEMY", 1080),
    ("dknight",  1, 32, 64, 40, "ENEMY", 1000),
    ("necro",    1, 32, 64, 40, "ENEMY", 1040),
    ("dragon",   3, 40, 5, 12, "DRAGON", 1160),
    ("bird",     4, 64, 8, 40, "CARRIER", 1120),
    ("snake",    1, 32, 64, 40, "ENEMY", 1376),
    ("wizard",   1, 32, 8, 5, "SETFIG", 936),
    ("royal",    1, 32, 8, 5, "SETFIG", 931),
    ("bartender", 1, 32, 8, 5, "SETFIG", 941),
    ("witch",    1, 32, 8, 5, "SETFIG", 946),
    ("ranger",   1, 32, 8, 5, "SETFIG", 951),
]

SHADOW_BLOCK, SHADOW_BLOCKS = 896, 24
SAMPLE_BLOCK, SAMPLE_BLOCKS = 920, 11

# gdriver.asm:202 - (period, offset in words) per note; sample length = (32-offset)*2 bytes
PTABLE = (
    [(p, 0) for p in (1440, 1356, 1280, 1208, 1140, 1076)] +
    [(p, 0) for p in (1016, 960, 906, 856, 808, 762, 720, 678, 640, 604, 570, 538)] +
    [(p, 0) for p in (508, 480, 453, 428, 404, 381, 360, 339, 320, 302, 285, 269)] +
    [(p, 16) for p in (508, 480, 453, 428, 404, 381, 360, 339, 320, 302, 285, 269)] +
    [(p, 24) for p in (508, 480, 453, 428, 404, 381, 360, 339, 320, 302, 285, 269)] +
    [(p, 28) for p in (508, 480, 453, 428, 404, 381, 360, 339, 320, 302, 285, 269)] +
    [(p, 28) for p in (254, 240, 226, 214, 202, 190, 180, 170, 160, 151, 143, 135)]
)

# gdriver.asm:227 - SMUS note durations in tempo counts
NOTEVALS = [
    26880, 13440, 6720, 3360, 1680, 840, 420, 210,
    40320, 20160, 10080, 5040, 2520, 1260, 630, 315,
    17920, 8960, 4480, 2240, 1120, 560, 280, 140,
    26880, 13440, 6720, 3360, 1680, 840, 420, 210,
    21504, 10752, 5376, 2688, 1344, 672, 336, 168,
    32256, 16128, 8064, 4032, 2016, 1008, 504, 252,
    23040, 11520, 5760, 2880, 1440, 720, 360, 180,
    34560, 17280, 8640, 4320, 2160, 1080, 540, 270,
]

PAL_CLOCK = 3546895
AY_CLOCK = 1750000          # Next AY clock (28 MHz / 16)

SAFE_START = (19036, 15755)  # fmain.c:2853
