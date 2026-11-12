# The Faery Tale Adventure - ZX Spectrum Next port
#
#   make           build assets and fta.nex into build/sd
#   make run       start in CSpect with build/sd as SD card root
#   make clean

GAME     := ../game
BUILD    := build
SD       := $(BUILD)/sd
PREVIEW  := $(BUILD)/preview

CSPECT   ?= $(HOME)/Retro/CSpect4_5_1_0/CSpect.exe
PYTHON   ?= python3 -I

ZCC      := zcc
ZFLAGS   := +zxn -vn -SO3 -clib=sdcc_iy -startup=31 --max-allocs-per-node200000 \
            -pragma-include:src/zpragma.inc

SRCS     := src/main.c src/platform/hw.c src/platform/layer2.c src/platform/files.c \
            src/platform/irq.asm

ASSETS   := $(SD)/FTA/PAGE0.L2

all: $(SD)/fta.nex $(ASSETS)

$(SD)/fta.nex: $(SRCS) $(wildcard src/*.h src/platform/*.h) src/zpragma.inc
	@mkdir -p $(BUILD)/obj $(SD)
	$(ZCC) $(ZFLAGS) $(SRCS) -o $(BUILD)/obj/fta -m -subtype=nex -create-app
	cp $(BUILD)/obj/fta.nex $@

$(ASSETS): tools/*.py
	$(PYTHON) tools/convert.py $(GAME) $(SD) $(PREVIEW)

run: all
	cd $(SD) && mono $(CSPECT) -w3 -zxnext -s28 -mmc=$(CURDIR)/$(SD)/ -map=$(CURDIR)/$(BUILD)/obj/fta.map fta.nex

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
