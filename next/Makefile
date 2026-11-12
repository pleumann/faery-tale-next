# The Faery Tale Adventure - ZX Spectrum Next port
#
#   make           build assets and fta.nex into build/sd
#   make run       start in CSpect with build/sd as SD card root
#   make clean

GAME     := ../game
BUILD    := build
SD       := $(BUILD)/sd
PREVIEW  := $(BUILD)/preview
GEN      := $(BUILD)/gen

CSPECT   ?= $(HOME)/Retro/CSpect4_5_1_0/CSpect.exe
PYTHON   ?= python3 -I

ZCC      := zcc
ZFLAGS   := +zxn -vn -SO3 -clib=sdcc_iy -startup=31 --max-allocs-per-node200000 \
            -pragma-include:src/zpragma.inc -I$(GEN)

SRCS     := src/main.c src/platform/hw.c src/platform/layer2.c src/platform/files.c \
            src/platform/input.c src/platform/irq.asm \
            src/game/world.c src/game/view.c src/game/actors.c src/game/sprites.c

ASSETS   := $(SD)/FTA/TILES.DAT

all: $(SD)/fta.nex

assets: $(ASSETS)

$(SD)/fta.nex: $(SRCS) $(ASSETS) $(wildcard src/*.h src/*/*.h) src/zpragma.inc
	@mkdir -p $(BUILD)/obj $(SD)
	$(ZCC) $(ZFLAGS) $(SRCS) -o $(BUILD)/obj/fta -m -subtype=nex -create-app
	cp $(BUILD)/obj/fta.nex $@

$(ASSETS): tools/*.py $(wildcard $(GAME)/*) ../narr.asm
	$(PYTHON) tools/convert.py $(GAME) $(SD) $(PREVIEW) $(GEN)

$(GEN)/narr.c: ../narr.asm tools/narr2c.py
	$(PYTHON) tools/narr2c.py $< $(GEN)

run: all
	cd $(SD) && mono $(CSPECT) -w3 -zxnext -s28 -mmc=$(CURDIR)/$(SD)/ -map=$(CURDIR)/$(BUILD)/obj/fta.map fta.nex

clean:
	rm -rf $(BUILD)

.PHONY: all assets run clean
