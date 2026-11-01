/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 ChoralRoot FM-1 contributors (a fork of Felucca) */
/* The Ableton Note Options in the instrument (docs/ABLETON-NOTE.md), on the firmware as the emulator builds it
 * (tools/emu/emu_fw.c: the whole instrument on a fresh RAM flash, headless, simulated time), driven through the panel:
 *   build/host/cr_note_ui_test [SHOTDIR]          (sh tests/run_cr_tests.sh; SHOTDIR: the screens as .ppm)
 * Checks: the six entries listed after Output with their values and lines; KNOB 1 sets them and the route follows;
 * GLO + BASS cycles Part to Note without opening anything or switching the bass; Note Setup's OCT+ sets the USB and
 * Note entries, the parts' channels on; the line under Note Setup names Key Mode's key and the tempo; the record keeps
 * every Note Option over a capture (what the Options save writes) and a load gives them back (a power cycle). */
#include <stdint.h>
#include "../tools/emu/emu_fw.c"

static int bad, checks;
static void check(const char *what, int ok)
{
    checks++;
    if (!ok)
        printf("cr_note_ui: FAIL %s\n", what);
    bad += !ok;
}
static uint32_t ms_now, frame_ms;
static uint64_t frames_done;
static void run(uint32_t ms)
{
    uint32_t end = ms_now + ms;
    int16_t buf[2 * EMU_BLOCK];
    for (; ms_now < end; ms_now++) {
        emu_fw_tick(ms_now);
        if (ms_now == 0 || ms_now - frame_ms >= 15u) {
            frame_ms = ms_now;
            emu_fw_frame();
        } else {
            emu_fw_idle();
        }
        while (frames_done + EMU_BLOCK <= (uint64_t)ms_now * EMU_FS / 1000u) {
            emu_fw_audio(buf, EMU_BLOCK);
            frames_done += EMU_BLOCK;
        }
    }
}
static void press(uint32_t label, uint32_t ms)    /* a button down for ms, then up */
{
    emu_hal.buttons |= 1u << emu_hal.btn_id[label];
    run(ms);
    emu_hal.buttons &= ~(1u << emu_hal.btn_id[label]);
    run(40);
}
static void knob(uint32_t role, int32_t steps)     /* detents on a knob, one every 60 ms */
{
    int32_t i, d = steps < 0 ? -1 : 1;
    for (i = 0; i != steps; i += d) {
        emu_hal.enc[emu_hal.enc_id[role]] += d * emu_hal.enc_dir[role];
        run(60);
    }
    run(100);
}
static const char *shots;
static void shot(const char *name)                 /* the LCD as a PPM (RGB565, big-endian as sent) */
{
    char path[512];
    FILE *f;
    uint32_t i;
    if (!shots)
        return;
    snprintf(path, sizeof path, "%s/%s.ppm", shots, name);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", EMU_LCD_W, EMU_LCD_H);
    for (i = 0; i < EMU_LCD_W * EMU_LCD_H; i++) {
        uint16_t v = emu_hal.lcd[i];
        uint8_t rgb[3];
        v = (uint16_t)((v >> 8) | (v << 8));
        rgb[0] = (uint8_t)(((v >> 11) & 31u) * 255u / 31u);
        rgb[1] = (uint8_t)(((v >> 5) & 63u) * 255u / 63u);
        rgb[2] = (uint8_t)((v & 31u) * 255u / 31u);
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
static int screen_has(const char *value, const char *label)   /* the Options page's value and line */
{
    cr_screen_t s;
    memset(&s, 0, sizeof s);
    cu_options_screen(&s);
    if (value && strcmp(s.value, value)) {
        printf("cr_note_ui: value '%s' (expected '%s')\n", s.value, value);
        return 0;
    }
    if (label && strcmp(s.label, label)) {
        printf("cr_note_ui: line '%s' (expected '%s')\n", s.label, label);
        return 0;
    }
    return 1;
}
static void goto_opt(uint32_t o)                   /* the Options open on entry o (SELECT from where they open) */
{
    if (!cu.opt_open)
        press(EMU_B_GLO, 60);
    while (cu.opt_sel < o)
        knob(EMU_E_SELECT, 1);
    while (cu.opt_sel > o)
        knob(EMU_E_SELECT, -1);
}

int main(int argc, char **argv)
{
    uint32_t o;
    shots = argc > 1 ? argv[1] : 0;
    emu_hal.ready = 2;                                  /* simulated time */
    emu_flash_path[0] = 0;                              /* a fresh RAM flash: the defaults */
    emu_hal.master = 700;
    emu_hal.knob_quick = 0;
    emu_fw_init(0);
    run(3000);                                          /* past the splash */

    /* the list: after MIDI Clock, in this order, each listed */
    check("the entries follow Output, before Version (every older entry keeps its place)",
          O_NOTE_PART == O_OUTPUT + 1 && O_NOTE_SETUP == O_NOTE_PART + 5 && O_VERSION == O_NOTE_SETUP + 1 &&
          O_VIEW == O_CLOCK + 1);
    for (o = O_NOTE_PART; o <= O_NOTE_SETUP; o++)
        if (!cu_opt_vis(o))
            check("an entry hidden", 0);
    check("their names", !strcmp(O_NAME[O_NOTE_PART], "Part to Note") && !strcmp(O_NAME[O_NOTE_SETUP], "Note Setup") &&
                         !strcmp(O_NAME[O_VIEW], "View") && !strcmp(O_NAME[O_VERSION], "Version"));
    check("O_MAX in step: View's and Version's maxima where they were", O_MAX[O_VIEW] == V_N - 1 &&
          O_MAX[O_NOTE_PART] == 3 && O_MAX[O_NOTE_REMOTE] == 16 && O_MAX[O_VERSION] == 0 && O_MAX[O_HOLD] == 3);

    /* defaults: as 0.28 sends */
    goto_opt(O_NOTE_PART);
    check("Options open on Part to Note", cu.opt_open && cu.opt_sel == O_NOTE_PART);
    check("Part to Note: All, every part", screen_has("All", "MIDI out: every part"));
    shot("note_part_all");
    check("the route at its defaults", cr_route.out_mask == 7u && !cr_route.oct[0] && !cr_route.oct[1] &&
          !cr_route.remote && cr_usb_src == CR_USB_SRC_MASTER);
    knob(EMU_E_K1, 1);
    check("KNOB 1: Chords", cs.note_part == CRS_NOTE_CHORDS && cr_route.out_mask == (1u << CR_STREAM_MAIN) &&
          screen_has("Chords", "MIDI out: chords \267 GLO+BASS"));
    shot("note_part_chords");

    goto_opt(O_NOTE_AUDIO);
    check("Note Audio: Master", screen_has("Master", "USB 1-2: the master"));
    knob(EMU_E_K1, 1);
    check("Note Audio: Follow Part, the chord pair", cs.note_audio == 1u && cr_usb_src == CR_USB_SRC_CHORD &&
          screen_has("Follow Part", "USB 1-2: the chords, dry"));
    shot("note_audio_follow");

    goto_opt(O_NOTE_OCT_CH);
    check("Note Oct Chords: 0", screen_has("0", "MIDI only, the FM-1 unchanged"));
    knob(EMU_E_K1, -1);
    check("Note Oct Chords: -1, on MAIN and RAW", cs.note_oct[0] == -1 && cr_route.oct[0] == -1 &&
          cr_route.oct[2] == -1 && screen_has("-1 octave", 0));
    shot("note_oct_chords");
    goto_opt(O_NOTE_OCT_BS);
    knob(EMU_E_K1, -2);
    check("Note Oct Bass: -2", cs.note_oct[1] == -2 && cr_route.oct[1] == -2 && screen_has("-2 octaves", 0));
    knob(EMU_E_K1, -3);
    check("Note Oct Bass: stops at -2", cs.note_oct[1] == -2);

    goto_opt(O_NOTE_REMOTE);
    check("Note Start/Stop: Off", screen_has("Off", "LOOP starts Note too: off"));
    knob(EMU_E_K1, 16);
    check("Note Start/Stop: ch 16", cs.note_remote == 16u && cr_route.remote == 16u &&
          screen_has("ch 16", "LOOP, OCT+: CC 102 to Note"));
    shot("note_startstop");
    {                                                   /* OCT+: one press now (for Note's MIDI mapping) */
        uint32_t pkt, n = 0, cc = 0, n0 = cr_note_rt_n;
        while (emu_fw_midi_out_take(&pkt))
            ;
        press(EMU_B_OCTUP, 60);
        while (emu_fw_midi_out_take(&pkt)) {
            n++;
            if (((pkt >> 8) & 0xFFu) == 0xBFu && ((pkt >> 16) & 0x7Fu) == CRS_NOTE_CC_START)
                cc++;
        }
        check("Note Start/Stop OCT+: CC 102 press and release on ch 16, the Options still open",
              cc == 2u && cr_note_rt_n == n0 + 1u && cu.opt_open && cu.opt_sel == O_NOTE_REMOTE);
    }

    /* GLO + BASS from the view: the part cycles, nothing opens, the bass stays as it was */
    press(EMU_B_HOME, 60);
    run(300);
    {
        uint8_t bass = cs.bass_on, part = cs.note_part;
        emu_hal.buttons |= 1u << emu_hal.btn_id[EMU_B_GLO];
        run(80);
        press(EMU_B_ENV, 80);
        emu_hal.buttons &= ~(1u << emu_hal.btn_id[EMU_B_GLO]);
        run(200);
        check("GLO + BASS: Chords -> Bass, the bass pair on USB 1-2", part == CRS_NOTE_CHORDS &&
              cs.note_part == CRS_NOTE_BASS && cr_route.out_mask == (1u << CR_STREAM_BASS) &&
              cr_usb_src == CR_USB_SRC_BASS);
        check("GLO + BASS: the bass not switched, no Options, no layer", cs.bass_on == bass && !cu.opt_open &&
              !cu.lock);
        shot("note_glo_bass");
        emu_hal.buttons |= 1u << emu_hal.btn_id[EMU_B_GLO];
        run(80);
        press(EMU_B_ENV, 80);
        press(EMU_B_ENV, 80);
        emu_hal.buttons &= ~(1u << emu_hal.btn_id[EMU_B_GLO]);
        run(200);
        check("GLO + BASS twice more: Bass -> All -> Chords", cs.note_part == CRS_NOTE_CHORDS &&
              cr_usb_src == CR_USB_SRC_CHORD);
    }

    /* Note Setup: Key Mode D minor at 96 BPM shows; OCT+ sets the rest */
    cs.key_on = 1; cs.tonic = 2; cs.scale = CR_SCALE_MINOR; cs.transpose = 0; cs.bpm = 96;
    cs.ch[CR_STREAM_BASS] = 0;                          /* (the bass's MIDI off: Note Setup turns it on) */
    cs.note_part = CRS_NOTE_ALL; cs.note_remote = 0; cs.note_audio = 0; cs.usb48 = 0; cs.usb_fixed = 0;
    cu_route();
    goto_opt(O_NOTE_SETUP);
    check("Note Setup: the line names the key and the tempo", screen_has("OCT+ sets it up", "Note: D Minor \267 96 BPM"));
    shot("note_setup");
    cs.transpose = 3;
    check("Note Setup: Transpose moves the key (D + 3 = F)", screen_has(0, "Note: F Minor \267 96 BPM"));
    cs.transpose = -3;
    check("Note Setup: Transpose -3 (D - 3 = B)", screen_has(0, "Note: B Minor \267 96 BPM"));
    cs.key_on = 0;
    check("Note Setup: no Key Mode", screen_has(0, "Note: Key Mode off \267 96 BPM"));
    cs.key_on = 1; cs.transpose = 0;
    press(EMU_B_OCTUP, 60);
    check("Note Setup OCT+: USB Record on, 44.1 + 48 kHz, Fixed", cs.usb_in && cs.usb48 && cs.usb_fixed);
    check("Note Setup OCT+: Chords to Note, Follow, Start/Stop ch 16", cs.note_part == CRS_NOTE_CHORDS &&
          cs.note_audio && cs.note_remote == 16u && cr_route.remote == 16u && cr_usb_src == CR_USB_SRC_CHORD);
    check("Note Setup OCT+: the bass's MIDI on again (ch 2), the chords' kept (ch 1)", cs.ch[CR_STREAM_BASS] == 2u &&
          cs.ch[CR_STREAM_MAIN] == 1u && cr_route.midi_en[CR_STREAM_BASS]);
    check("Note Setup OCT+: the Options stay open on it", cu.opt_open && cu.opt_sel == O_NOTE_SETUP);
    shot("note_setup_done");

    /* the record: what the Options save writes keeps them; a load gives them back */
    {
        cr_settings_t s;
        crs_capture(&s);
        check("capture: every Note Option in the record", CRS_NOTE_PART(&s) == CRS_NOTE_CHORDS && CRS_NOTE_AUDIO(&s) &&
              CRS_NOTE_REMOTE(&s) == 16u && CRS_NOTE_OCT(&s, 0) == -1 && CRS_NOTE_OCT(&s, 1) == -2 &&
              CRS_NOTE_OCT(&s, 2) == -1 && CRS_USB_LEVEL(&s) == CRS_USB_FIXED && CRS_MEN_ON(&s, 1));
        cs.note_part = 0; cs.note_audio = 0; cs.note_remote = 0; cs.note_oct[0] = cs.note_oct[1] = 0;
        cr_settings_seal(&s);
        crs_rec.cr = s;
        cr_settings_load();
        check("load: they come back, and the route with them", cs.note_part == CRS_NOTE_CHORDS && cs.note_audio &&
              cs.note_remote == 16u && cs.note_oct[0] == -1 && cs.note_oct[1] == -2 && cr_route.remote == 16u &&
              cr_route.oct[1] == -2 && cr_usb_src == CR_USB_SRC_CHORD && cs.usb_fixed == 1u);
    }
    printf(bad ? "cr_note_ui_test: %d of %d checks FAILED\n" : "cr_note_ui_test: %2$d checks, all ok\n", bad, checks);
    return bad != 0;
}
