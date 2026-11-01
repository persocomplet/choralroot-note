/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca) */
/* Host test of the Ableton Note Options' MIDI and USB side (firmware/src/cr_out.c; docs/ABLETON-NOTE.md): the
 * stream callbacks and the transport edge driven directly, the USB MIDI out queue read back, the sound side through
 * tests/hostsim.c as tests/cr_trans_test.c builds it.
 *   sh tests/run_cr_tests.sh
 * Checks: Part to Note sends one stream and keeps the others sounding; Note Oct shifts the MIDI notes alone, a note-off
 * and the polyphonic aftertouch go to the note as it went out whatever the octave is set to meanwhile, a retrigger
 * after a change ends the old one, a note shifted off the keyboard is not sent; Note Start/Stop sends CC 102 127 / 0
 * on the loop's start and stop (with or without the MIDI clock), never with MIDI Clock In, never when Off; Note Audio
 * puts the part's pair on ChoralRoot In 1-2 */
#define FELUCCA_VA 1
#define FELUCCA_UAC 1                                /* (fx.c stages ChoralRoot In: Note Audio checked) */
#define FELUCCA_SLICE 0
#define FELUCCA_SEQ 0                                /* as choralroot.c: cr_out.c's events_block, no seq.c */
#define main hostsim_main
#include "hostsim.c"
#undef main
#define cr_param_t cr_eparam_t
#include "../firmware/src/cr_engine.h"
#include "../firmware/src/cr_engine.c"
#undef cr_param_t
#include "../firmware/src/cr_loop.h"
#include "../firmware/src/cr_loop.c"
static void fm1_irq_off(void) {}                 /* (the HAL's: no ISR on the host) */
static void fm1_irq_on(void) {}
#include "../firmware/src/cr_out.c"

static int fails, checks;
#define CHECK(c, ...)                                                   \
    do {                                                                \
        checks++;                                                       \
        if (!(c)) {                                                     \
            fails++;                                                    \
            printf("  FAIL %s:%d: ", __FILE__, __LINE__);               \
            printf(__VA_ARGS__);                                        \
            printf("\n");                                               \
        }                                                               \
    } while (0)

/* the USB MIDI out queue as messages: status, d1, d2 (a real-time byte: d1 = d2 = 0) */
typedef struct { uint32_t st, d1, d2; } msg_t;
static msg_t got[MQ];
static uint32_t ngot;
static void take(void)
{
    ngot = 0;
    for (; mo_r != mo_w; mo_r++) {
        uint32_t p = midi_out_q[mo_r % MQ];
        got[ngot].st = (p >> 8) & 0xFFu;
        got[ngot].d1 = (p >> 16) & 0x7Fu;
        got[ngot].d2 = (p >> 24) & 0x7Fu;
        ngot++;
    }
}
static int has(uint32_t st, uint32_t d1, uint32_t d2)   /* st with d1 / d2 (0xFF: any) in what take() read */
{
    uint32_t i;
    for (i = 0; i < ngot; i++)
        if (got[i].st == st && (d1 == 0xFFu || got[i].d1 == d1) && (d2 == 0xFFu || got[i].d2 == d2))
            return 1;
    return 0;
}
static int at(uint32_t i, uint32_t st, uint32_t d1, uint32_t d2)
{
    return i < ngot && got[i].st == st && got[i].d1 == d1 && got[i].d2 == d2;
}
static int gated(const track_t *t, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].gate && t->v[i].note == note)
            return 1;
    return 0;
}
static void t_tracks_init(void)
{
    uint32_t i, k;
    for (i = 0; i < G_COUNT; i++)
        song.g[i] = GP[i].def;
    for (k = 0; k < NTRK; k++)
        for (i = 0; i < P_E0; i++)
            trk[k].p[i] = TP[i].def;
    song.master_q12 = 4096;
}
static void t_preset(track_t *t, uint32_t e, uint32_t pi)
{
    const preset_t *p = &ENGINES[e]->presets[pi];
    uint32_t i;
    t->eng_req = t->engine = (uint8_t)e;
    t->preset = (uint8_t)pi;
    for (i = 0; i < 8u; i++)
        t->p[P_E0 + i] = p->e[i];
    t->p[P_ATK] = p->env[0];
    t->p[P_DEC] = p->env[1];
    t->p[P_SUS] = p->env[2];
    t->p[P_REL] = p->env[3];
    t->p[P_ED_FLT] = p->fenv;
}
static void route_defaults(void)
{
    cr_route.out_mask = 7u;
    cr_route.oct[0] = cr_route.oct[1] = cr_route.oct[2] = 0;
    cr_route.remote = 0;
    cr_route.clock_out = cr_route.clock_in = 0;
}

static void t_part(track_t *c, track_t *b)
{
    route_defaults();
    take();
    cr_cb_note_on(0, CR_STREAM_MAIN, 60, 100);
    cr_cb_note_on(0, CR_STREAM_BASS, 40, 90);
    take();
    CHECK(has(0x90, 60, 100) && has(0x91, 40, 90), "All (the default): chords on ch 1, bass on ch 2, as before");
    cr_cb_note_off(0, CR_STREAM_MAIN, 60);
    cr_cb_note_off(0, CR_STREAM_BASS, 40);
    take();
    CHECK(has(0x80, 60, 0) && has(0x81, 40, 0), "All: their note-offs");

    cr_route.out_mask = 1u << CR_STREAM_MAIN;          /* Part to Note: Chords */
    cr_cb_note_on(0, CR_STREAM_MAIN, 62, 100);
    cr_cb_note_on(0, CR_STREAM_BASS, 38, 100);
    take();
    CHECK(has(0x90, 62, 100) && !has(0x91, 0xFF, 0xFF), "Chords: the chord notes go out, the bass's do not");
    CHECK(gated(c, 62) && gated(b, 38), "Chords: both parts still sound on the FM-1");
    cr_cb_note_off(0, CR_STREAM_MAIN, 62);
    cr_cb_note_off(0, CR_STREAM_BASS, 38);
    take();
    CHECK(has(0x80, 62, 0) && ngot == 1u, "Chords: only the chord's note-off (the bass sent nothing to end)");

    cr_route.out_mask = 1u << CR_STREAM_BASS;          /* Bass */
    cr_cb_note_on(0, CR_STREAM_MAIN, 64, 100);
    cr_cb_note_on(0, CR_STREAM_BASS, 36, 100);
    take();
    CHECK(has(0x91, 36, 100) && !has(0x90, 0xFF, 0xFF), "Bass: the bass notes alone");
    cr_cb_note_off(0, CR_STREAM_MAIN, 64);
    cr_cb_note_off(0, CR_STREAM_BASS, 36);
    take();

    cr_route.out_mask = 7u;                             /* a switch while a note sounds: its note-off still goes out */
    cr_cb_note_on(0, CR_STREAM_MAIN, 65, 100);
    take();
    cr_route.out_mask = 1u << CR_STREAM_BASS;
    cr_cb_note_off(0, CR_STREAM_MAIN, 65);
    take();
    CHECK(has(0x80, 65, 0), "a note sent before Part to Note changed: its note-off still goes out (nothing stuck)");
    route_defaults();
}

static void t_oct(track_t *c)
{
    route_defaults();
    cr_route.oct[CR_STREAM_MAIN] = -1;
    cr_cb_note_on(0, CR_STREAM_MAIN, 60, 100);
    take();
    CHECK(has(0x90, 48, 100) && gated(c, 60), "Note Oct -1: C4 goes out as C3, the FM-1 plays C4");
    cr_route.oct[CR_STREAM_MAIN] = 2;                  /* changed while it sounds */
    cr_cb_note_off(0, CR_STREAM_MAIN, 60);
    take();
    CHECK(has(0x80, 48, 0) && ngot == 1u, "changed while held: the note-off ends C3, the note as it went out");
    cr_cb_note_on(0, CR_STREAM_MAIN, 64, 100);
    take();
    CHECK(has(0x90, 88, 100), "Note Oct +2: E4 goes out as E6");
    cr_cb_note_off(0, CR_STREAM_MAIN, 64);
    take();
    /* a retrigger (the same stream note struck again) after a change: the old note ends first */
    cr_route.oct[CR_STREAM_MAIN] = 0;
    cr_cb_note_on(0, CR_STREAM_MAIN, 67, 100);
    take();
    cr_route.oct[CR_STREAM_MAIN] = 1;
    cr_cb_note_on(0, CR_STREAM_MAIN, 67, 90);
    take();
    CHECK(at(0, 0x80, 67, 0) && at(1, 0x90, 79, 90), "a retrigger after a change: G4 ends, then G5 starts");
    cr_cb_note_off(0, CR_STREAM_MAIN, 67);
    take();
    CHECK(has(0x80, 79, 0) && ngot == 1u, "and its note-off ends G5");
    /* a retrigger with nothing changed: as before (no extra note-off) */
    cr_cb_note_on(0, CR_STREAM_MAIN, 69, 100);
    cr_cb_note_on(0, CR_STREAM_MAIN, 69, 100);
    take();
    CHECK(ngot == 2u && at(0, 0x90, 81, 100) && at(1, 0x90, 81, 100), "a retrigger unchanged: two note-ons, as before");
    cr_cb_note_off(0, CR_STREAM_MAIN, 69);
    take();
    /* shifted off the keyboard: not sent, and its note-off sends nothing */
    cr_route.oct[CR_STREAM_MAIN] = 2;
    cr_cb_note_on(0, CR_STREAM_MAIN, 120, 100);
    take();
    CHECK(ngot == 0u && gated(c, 120), "Note Oct +2 on note 120: nothing sent (144), the FM-1 plays it");
    cr_cb_note_off(0, CR_STREAM_MAIN, 120);
    take();
    CHECK(ngot == 0u, "its note-off: nothing sent");
    cr_route.oct[CR_STREAM_MAIN] = -2;
    cr_cb_note_on(0, CR_STREAM_MAIN, 10, 100);
    take();
    CHECK(ngot == 0u, "Note Oct -2 on note 10: nothing sent (-14)");
    cr_cb_note_off(0, CR_STREAM_MAIN, 10);
    take();
    /* the bass's own octave; the raw stream's */
    cr_route.oct[CR_STREAM_MAIN] = 0;
    cr_route.oct[CR_STREAM_BASS] = -1;
    cr_route.midi_en[CR_STREAM_RAW] = 1;
    cr_route.oct[CR_STREAM_RAW] = 1;
    cr_cb_note_on(0, CR_STREAM_BASS, 40, 100);
    cr_cb_note_on(0, CR_STREAM_RAW, 60, 100);
    cr_cb_note_on(0, CR_STREAM_MAIN, 60, 100);
    take();
    CHECK(has(0x91, 28, 100) && has(0x92, 72, 100) && has(0x90, 60, 100), "per stream: bass -1, raw +1, chords 0");
    /* polyphonic aftertouch: to the notes as they went out */
    cr_touch_mode = CR_TOUCH_VEL_POLY;
    cr_touch_at(77);
    take();
    CHECK(has(0xA1, 28, 77) && has(0xA0, 60, 77) && !has(0xA1, 40, 0xFF), "poly aftertouch: on E1 (bass -1), C4");
    cr_route.out_mask = 1u << CR_STREAM_MAIN;
    cr_touch_at(50);
    take();
    CHECK(has(0xA0, 60, 50) && !has(0xA1, 0xFF, 0xFF), "poly aftertouch: Part to Note Chords, the chords' alone");
    cr_touch_mode = CR_TOUCH_VEL_AT;
    cr_touch_at(40);
    take();
    CHECK(has(0xD0, 40, 0) && !has(0xD1, 0xFF, 0xFF), "channel aftertouch: Part to Note Chords, ch 1 alone");
    cr_touch_mode = CR_TOUCH_FIXED;
    cr_route.out_mask = 7u;
    cr_cb_note_off(0, CR_STREAM_BASS, 40);
    cr_cb_note_off(0, CR_STREAM_RAW, 60);
    cr_cb_note_off(0, CR_STREAM_MAIN, 60);
    take();
    CHECK(has(0x81, 28, 0) && has(0x82, 72, 0) && has(0x80, 60, 0), "their note-offs, as they went out");
    cr_route.midi_en[CR_STREAM_RAW] = 0;
    route_defaults();
}

static void t_remote(void)
{
    route_defaults();
    cr_rt_was = 0;
    crl.state = CRL_STOPPED;
    take();
    cr_route.remote = 16;                               /* Note Start/Stop ch 16, MIDI Clock Off */
    crl.state = CRL_PLAYING;
    cr_rt_poll();
    take();
    CHECK(ngot == 2u && at(0, 0xBF, CRS_NOTE_CC_START, 127) && at(1, 0xBF, CRS_NOTE_CC_START, 0),
          "the loop starts: CC 102 127 then 0 on ch 16 (a button's press and release), no clock start (Clock Off)");
    cr_rt_poll();
    take();
    CHECK(ngot == 0u, "while it plays: nothing more");
    crl.state = CRL_STOPPED;
    cr_rt_poll();
    take();
    CHECK(ngot == 2u && at(0, 0xBF, CRS_NOTE_CC_START, 127), "the loop stops: the same press (Note's Start/Stop toggles)");
    cr_route.clock_out = 1;                             /* with MIDI Clock Out: the start and the press */
    crl.state = CRL_PLAYING;
    cr_rt_poll();
    take();
    CHECK(has(0xFA, 0, 0) && has(0xBF, CRS_NOTE_CC_START, 127) && ngot == 3u, "Clock Out: MIDI start and the press");
    crl.state = CRL_STOPPED;
    cr_rt_poll();
    take();
    CHECK(has(0xFC, 0, 0) && has(0xBF, CRS_NOTE_CC_START, 127), "Clock Out: MIDI stop and the press");
    cr_route.clock_out = 0;                             /* MIDI Clock In: the loop follows the outside, nothing sent */
    cr_route.clock_in = 1;
    crl.state = CRL_PLAYING;
    cr_rt_poll();
    crl.state = CRL_STOPPED;
    cr_rt_poll();
    take();
    CHECK(ngot == 0u, "MIDI Clock In: no press (it would stop what started the loop)");
    cr_route.clock_in = 0;
    cr_route.remote = 1;                                /* ch 1 */
    crl.state = CRL_PLAYING;
    cr_rt_poll();
    take();
    CHECK(at(0, 0xB0, CRS_NOTE_CC_START, 127), "Note Start/Stop ch 1");
    cr_route.remote = 0;                                /* Off */
    crl.state = CRL_STOPPED;
    cr_rt_poll();
    take();
    CHECK(ngot == 0u, "Note Start/Stop Off: nothing (as before)");
    route_defaults();
}

#if defined(FX_UA_STAGE) && FX_UA_STAGE
static void t_usb(void)
{
    int16_t st[4 * UA_CAP_CHANNELS];
    int32_t out[2 * 4];
    uint32_t i, ok_m = 1, ok_c = 1, ok_b = 1;
    for (i = 0; i < 4u; i++) {
        out[2 * i] = 1000 + (int32_t)i;
        out[2 * i + 1] = 2000 + (int32_t)i;
    }
    for (i = 0; i < 4u * UA_CAP_CHANNELS; i++)
        st[i] = (int16_t)(100 + i);
    cr_usb_src = CR_USB_SRC_MASTER;
    cr_usb_pair1(st, out, 4);
    for (i = 0; i < 4u; i++)
        ok_m &= st[i * UA_CAP_CHANNELS] == 1000 + (int)i && st[i * UA_CAP_CHANNELS + 1] == 2000 + (int)i &&
                st[i * UA_CAP_CHANNELS + 2] == (int16_t)(100 + i * UA_CAP_CHANNELS + 2);
    CHECK(ok_m, "Note Audio Master: 1-2 the master, 3-6 untouched (as before)");
    cr_usb_src = CR_USB_SRC_CHORD;
    cr_usb_pair1(st, out, 4);
    for (i = 0; i < 4u; i++)
        ok_c &= st[i * UA_CAP_CHANNELS] == st[i * UA_CAP_CHANNELS + 2] &&
                st[i * UA_CAP_CHANNELS + 1] == st[i * UA_CAP_CHANNELS + 3];
    CHECK(ok_c, "Note Audio Follow, Chords: 1-2 = the chord pair (3-4)");
    cr_usb_src = CR_USB_SRC_BASS;
    cr_usb_pair1(st, out, 4);
    for (i = 0; i < 4u; i++)
        ok_b &= st[i * UA_CAP_CHANNELS] == st[i * UA_CAP_CHANNELS + 4] &&
                st[i * UA_CAP_CHANNELS + 1] == st[i * UA_CAP_CHANNELS + 5];
    CHECK(ok_b, "Note Audio Follow, Bass: 1-2 = the bass pair (5-6)");
    out[0] = 99999;                                     /* the master saturated to 16 bits, as the shim did */
    cr_usb_src = CR_USB_SRC_MASTER;
    cr_usb_pair1(st, out, 1);
    CHECK(st[0] == 32767, "Note Audio Master: saturated to 16 bits");
}
#endif

int main(void)
{
    track_t *c = &trk[CR_PART_CHORD], *b = &trk[CR_PART_BASS];
    t_tracks_init();
    usb.config = 1;                                     /* (the MIDI out queue takes events) */
    t_preset(c, 7, 0);
    t_preset(b, 7, 0);
    c->p[P_VOICE] = V_POLY;
    b->p[P_VOICE] = V_MONO;
    CHECK(cr_route.out_mask == 7u && !cr_route.oct[0] && !cr_route.oct[1] && !cr_route.oct[2] && !cr_route.remote,
          "the route's power-on: every stream, no shift, no Start/Stop (0.28's MIDI out)");
    t_part(c, b);
    t_oct(c);
    t_remote();
#if defined(FX_UA_STAGE) && FX_UA_STAGE
    t_usb();
#else
    printf("cr_note_test: (no USB stage in this build: Note Audio not checked)\n");
#endif
    printf("cr_note_test: %d checks, %d failed\n", checks, fails);
    return fails ? 1 : 0;
}
