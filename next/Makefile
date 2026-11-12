# The Faery Tale Adventure - ZX Spectrum Next port
#
#   make -j8       build assets and fta.nex into build/sd (only what changed)
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
ZFLAGS   := +zxn -vn -SO3 -clib=sdcc_iy -startup=31 --max-allocs-per-node20000 \
            -pragma-include:src/zpragma.inc -I$(GEN)

ASSETS   := $(SD)/FTA/TILES.DAT

# resident code (MMU slots 4-7, 0x8000-0xFCFF)
SRCS     := src/main.c src/platform/hw.c src/platform/layer2.c src/platform/files.c \
            src/platform/input.c src/platform/irq.asm \
            src/game/world.c src/game/view.c src/game/actors.c \
            src/game/state.c src/game/shapes.c src/game/glue.c src/game/ui.c \
            src/game/save.c
OBJS     := $(patsubst src/%,$(BUILD)/obj/%.o,$(basename $(SRCS)))

# banked modules: file -> 8K page (org 0x0000, run in MMU slot 0, see platform/bank.h)
BANKED   := logic:80 doors:81 objects:82 move:83 sprites:84 combat:85 menus:86 audio:87 placard:88 intro:89
BANK_OBJS := $(foreach b,$(BANKED),$(BUILD)/obj/bank/$(word 1,$(subst :, ,$(b))).o)
$(foreach b,$(BANKED),$(eval PAGE_$(word 1,$(subst :, ,$(b))) := $(word 2,$(subst :, ,$(b)))))

# every object depends on all headers (simple and safe; each file compiles in seconds)
HEADERS  := $(wildcard src/*.h src/*/*.h) src/zpragma.inc $(GEN)/narr.h $(ASSETS)

all: $(SD)/fta.nex

assets: $(ASSETS)

$(BUILD)/obj/%.o: src/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(ZCC) $(ZFLAGS) -c $< -o $@

$(BUILD)/obj/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(ZCC) $(ZFLAGS) -c $< -o $@

$(BUILD)/obj/bank/%.o: src/game/%.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(ZCC) $(ZFLAGS) -c $< -o $@ --codesegPAGE_$(PAGE_$*) --constsegPAGE_$(PAGE_$*)

$(SD)/fta.nex: $(OBJS) $(BANK_OBJS)
	@mkdir -p $(SD)
	$(ZCC) $(ZFLAGS) $(OBJS) $(BANK_OBJS) -o $(BUILD)/obj/fta -m -subtype=nex -create-app
	cp $(BUILD)/obj/fta.nex $@
	@grep -E "^__BSS_END_tail|^__code_user_tail" $(BUILD)/obj/fta.map

$(ASSETS): tools/*.py $(wildcard $(GAME)/*) ../narr.asm
	$(PYTHON) tools/convert.py $(GAME) $(SD) $(PREVIEW) $(GEN)

$(GEN)/narr.h: ../narr.asm tools/narr2c.py
	@mkdir -p $(SD)/FTA
	$(PYTHON) tools/narr2c.py $< $(GEN) $(SD)/FTA/NARR.DAT

run: all
	cd $(SD) && mono $(CSPECT) -w3 -zxnext -s28 -mmc=$(CURDIR)/$(SD)/ -map=$(CURDIR)/$(BUILD)/obj/fta.map fta.nex

music-preview: $(ASSETS)
	$(PYTHON) tools/music_preview.py $(SD)/FTA/MUSIC.DAT $(PREVIEW) 30

clean:
	rm -rf $(BUILD)

.PHONY: all assets run clean music-preview
