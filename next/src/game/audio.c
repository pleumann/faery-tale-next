/* Banked module (PAGE_AUDIO): music and sound effects on the Next's AY chips.

   The four-voice score player of gdriver.asm, driven once per 50 Hz frame:
   a tempo clock, SMUS-like note/rest/instrument/tempo events per voice and a volume
   envelope per note. Voices 0-2 play on AY 1 (channels A-C), voice 3 on AY 2 channel A.
   The Amiga's waveforms become plain AY square tones. Sound effects (Amiga: samples on
   voice 2) are noise bursts on AY 2 channel C, as long as the original sample. */

#include <stdint.h>
#include <z80.h>
#include "../platform/hw.h"
#include "assets.h"
#include "world.h"
#include "state.h"
#include "actors.h"
#include "audio.h"

#define MUSIC_PAGE  76                  /* MUSIC.DAT, 2 pages */
#define SLOT1       ((uint8_t *)0x2000)

/* octaves the bass voice is raised on the AY (0 = original pitch, 40-100 Hz hums) */
#ifndef BASS_OCTAVE
#define BASS_OCTAVE 1
#endif

#define AY_REG      0xFFFD
#define AY_DATA     0xBFFD

struct voice {
    uint16_t trak_ptr, trak_beg;        /* offsets in MUSIC.DAT, 0 = no track */
    uint32_t event_start, event_stop;
    uint16_t vol_list;                  /* envelope read position */
    uint8_t wave_num, vol_num;
    int8_t vol_delay;                   /* -1: no envelope changes */
};

static struct voice voices[4];
static uint32_t timeclock;
static uint16_t tempo = 150;
static uint8_t playing;
static uint8_t music_on = 1, sound_on = 1;
static uint8_t cur_ay;

/* fmain.c:669 - instrument per slot: high byte waveform, low byte envelope */
static int16_t new_wave[12] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0005,
    0x0202, 0x0101, 0x0103, 0x0004, 0x0504,
    0x0100, 0x0500 };

/* Amiga volume 0..64 (linear) -> AY level 0..15 (about 3 dB per step) */
static const uint8_t vol_ay[65] = {
     0, 3, 5, 6, 7, 8, 8, 9, 9,10,10,10,10,11,11,11,
    11,11,11,12,12,12,12,12,12,12,12,13,13,13,13,13,
    13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,14,
    14,14,14,14,14,14,14,14,15,15,15,15,15,15,15,15,15 };

/* sample lengths in bytes (SAMPLES.DAT) for the effect durations */
static const uint16_t sample_size[6] = { 1012, 684, 786, 1604, 952, 550 };

static uint8_t fx_frames, fx_left;

/* Octaves to shift per Amiga waveform: waveform 5 is dominated by its 2nd/3rd harmonic,
   waveform 3 by the 4th, so on the Amiga they sound higher than their fundamental, which
   a plain AY square wave would play as a low hum. */
static const uint8_t wave_octave[8] = { 0, 0, 0, 2, 0, 1, 0, 0 };

static uint8_t mb(uint16_t off)
{
    mmu_set(1, MUSIC_PAGE + (off >> 13));
    return SLOT1[off & 0x1FFF];
}

static uint16_t mw(uint16_t off)
{
    return mb(off) | ((uint16_t)mb(off + 1) << 8);
}

static void ay_select(uint8_t chip)     /* 0 = AY 1, 1 = AY 2 */
{
    if (cur_ay != chip) {
        z80_outp(AY_REG, chip ? 0xFE : 0xFF);
        cur_ay = chip;
    }
}

static void ay(uint8_t reg, uint8_t val)
{
    z80_outp(AY_REG, reg);
    z80_outp(AY_DATA, val);
}

/* volume of voice v (0..64) */
static void set_volume(uint8_t v, uint8_t vol)
{
    if (vol > 64) vol = 64;
    if (v < 3) { ay_select(0); ay(8 + v, vol_ay[vol]); }
    else { ay_select(1); ay(8, vol_ay[vol]); }
}

static void set_period(uint8_t v, uint16_t period)
{
    uint8_t ch = (v < 3) ? v : 0;
    ay_select(v < 3 ? 0 : 1);
    ay(ch * 2, (uint8_t)period);
    ay(ch * 2 + 1, (uint8_t)(period >> 8) & 15);
}

void audio_init_b(void)
{
    uint8_t i;
    nextreg(0x08, nextreg_read(0x08) | 0x02);  /* turbosound: three AYs */
    cur_ay = 0xFF;
    for (i = 0; i < 2; i++) {
        ay_select(i);
        ay(7, i ? 0x1E : 0x38);         /* AY 1: tones A-C; AY 2: tone A, noise C */
        ay(8, 0); ay(9, 0); ay(10, 0);
    }
}

static void silence(void)
{
    uint8_t v;
    for (v = 0; v < 4; v++) set_volume(v, 0);
}

/* gdriver.asm _playscore: start tracks off..off+3 immediately */
static void playscore(uint8_t off)
{
    uint8_t v;
    silence();
    for (v = 0; v < 4; v++) {
        struct voice *vc = &voices[v];
        vc->trak_beg = vc->trak_ptr = mw((off + v) * 2);
        vc->wave_num = (uint8_t)(new_wave[v] >> 8);
        vc->vol_num = (uint8_t)new_wave[v];
        vc->vol_delay = -1;
        vc->event_start = vc->event_stop = 0;
    }
    timeclock = 0;
    playing = 1;
}

/* _setscore: the new tracks start when the current ones loop */
static void setscore(uint8_t off)
{
    uint8_t v;
    for (v = 0; v < 4; v++)
        voices[v].trak_beg = mw((off + v) * 2);
}

void stopscore_b(void)
{
    silence();
    playing = 0;
}

/* gdriver.asm dovoice: one voice, one frame */
static void dovoice(uint8_t v)
{
    struct voice *vc = &voices[v];
    uint8_t cmd, val, vol;
    uint16_t len, period;

    if (!vc->trak_ptr) return;
    if ((int32_t)(timeclock - vc->event_start) >= 0) {
newnote:
        cmd = mb(vc->trak_ptr);
        val = mb(vc->trak_ptr + 1);
        vc->trak_ptr += 2;
        if (cmd & 0x80 && cmd != 0x80) {
            if (cmd == 0x81) {                          /* instrument */
                int16_t w = new_wave[val & 15];
                vc->wave_num = (uint8_t)(w >> 8);
                vc->vol_num = (uint8_t)w;
            }
            else if (cmd == 0x90) tempo = val;          /* tempo */
            else if (cmd == 0xFF) {                     /* end of track */
                if (val) { vc->trak_ptr = vc->trak_beg; goto newnote; }
                vc->trak_ptr = 0;
                vc->vol_delay = -1;
                set_volume(v, 0);
                return;
            }
            goto newnote;
        }
        /* note or rest: duration */
        len = mw(MUSIC_OFF_NOTEVALS + (val & 0x3F) * 2);
        vc->event_stop = vc->event_start + (len > 300 ? len - 300 : len);
        vc->event_start += len;
        vc->vol_list = MUSIC_OFF_ENVELOPES + (uint16_t)vc->vol_num * 256;
        vol = mb(vc->vol_list++);
        vc->vol_delay = 0;
        if (cmd == 0x80) {                              /* rest */
            set_volume(v, 0);
            vc->event_stop = timeclock;
            return;
        }
        if (cmd > 83) cmd = 83;
        period = mw(MUSIC_OFF_PERIODS + (uint16_t)cmd * 2) >> wave_octave[vc->wave_num & 7];
        if (v == 3) period >>= BASS_OCTAVE;             /* out of the hum range */
        set_period(v, period);
        set_volume(v, vol);
        return;
    }
    if ((int32_t)(timeclock - vc->event_stop) >= 0) { set_volume(v, 0); return; }
    if (vc->vol_delay) return;
    vol = mb(vc->vol_list);
    if (vol & 0x80) return;                             /* negative: hold */
    vc->vol_list++;
    set_volume(v, vol);
}

static void fx_tick(void)
{
    if (!fx_left) return;
    fx_left--;
    ay_select(1);
    ay(10, (uint8_t)((uint16_t)fx_left * 15 / fx_frames));
}

/* one 50 Hz frame */
void audio_tick_b(void)
{
    uint8_t v;
    timeclock += tempo;
    if (playing && music_on)
        for (v = 0; v < 4; v++) dovoice(v);
    fx_tick();
    mmu_set(1, 0xFF);
}

/* fmain.c:3616 - effect(num, period): a noise burst as long as the Amiga sample */
void effect_b(uint8_t num, uint16_t period)
{
    uint32_t frames;
    uint8_t noise;
    if (!sound_on || num > 5) return;
    frames = ((uint32_t)sample_size[num] * period * 50) / 3546895UL;
    if (frames < 2) frames = 2;
    if (frames > 50) frames = 50;
    fx_frames = fx_left = (uint8_t)frames;
    noise = (uint8_t)(period >> 7);
    if (noise < 1) noise = 1;
    if (noise > 31) noise = 31;
    ay_select(1);
    ay(6, noise);
    ay(10, 15);
}

/* fmain.c:2936 setmood: day, battle, night, astral plane, indoors, death */
void setmood_b(uint8_t now)
{
    uint8_t off;
    if (anim_list[0].vitality == 0) off = 6 * 4;
    else if (hero_x > 0x2400 && hero_x < 0x3100 && hero_y > 0x8200 && hero_y < 0x8a00)
        off = 4 * 4;
    else if (battleflag) off = 4;
    else if (region_num > 7) {
        off = 5 * 4;
        new_wave[10] = (region_num == 9) ? 0x0307 : 0x0100;
    }
    else if (lightlevel > 120) off = 0;
    else off = 8;

    if (music_on) {
        if (now) playscore(off);
        else setscore(off);
    }
    else stopscore_b();
    mmu_set(1, 0xFF);
}

void audio_options_b(uint8_t music, uint8_t sound)
{
    sound_on = sound;
    if (music != music_on) {
        music_on = music;
        if (!music) stopscore_b();
        else setmood_b(TRUE);
    }
}

void music_play_b(uint8_t off)
{
    if (music_on) playscore(off);
    mmu_set(1, 0xFF);
}
