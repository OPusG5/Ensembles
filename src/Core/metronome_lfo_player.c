/*
 * Copyright 2020-2022 Subhadeep Jasu <subhajasu@gmail.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <fluidsynth.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

#include "style_analyser.h"
#include "central_bus.h"
#include "synthesizer.h"
#include "driver_settings_provider.h"

// None of this will be used for actual rendering /////////
fluid_settings_t* lfo_settings;
fluid_synth_t* lfo_synth;
fluid_audio_driver_t* lfo_adriver;
fluid_player_t* lfo_player;
///////////////////////////////////////////////////////////

int lfo_measure_length = 0;
int lfo_looping = 0;
int lfo_end_of_line = 0;

char* metronome_file_path;

int
lfo_parse_midi_events (void *data, fluid_midi_event_t *event) {
    int type = fluid_midi_event_get_type (event);
    int channel = fluid_midi_event_get_channel (event);
    int key = fluid_midi_event_get_key (event);

    int lfo_type = get_central_lfo_on ();
    if (lfo_type > 0 && lfo_type < 16) {
        if (lfo_type < 10) {
            if (lfo_type == (channel + 1) && fluid_midi_event_get_control (event) == 16) {
                set_central_lfo_value (fluid_midi_event_get_value (event));
            }
        } else if (lfo_type > 9) {
            if (lfo_type == channel && fluid_midi_event_get_control (event) == 16) {
                set_central_lfo_value (fluid_midi_event_get_value (event));
            }
        }
    }
    if (channel == 9) {
        // Send data to synth
        if (get_central_metronome_on ())
            synthesizer_send_notes_metronome (key, type);
    }
    return 0;
}


int
lfo_parse_ticks (void* data, int ticks) {
    if (ticks >= lfo_end_of_line) {
        fluid_player_stop (lfo_player);
    }
}


void
metronome_lfo_player_init () {
    lfo_settings = get_settings(METRONOME_PLAYER);
    lfo_synth = new_fluid_synth(lfo_settings);
    lfo_adriver = new_fluid_audio_driver(lfo_settings, lfo_synth);
}


void
metronome_lfo_player_change_base (const char* mid_file, int tempo, int eol) {
    lfo_end_of_line = eol;
    if (lfo_player) {
        fluid_player_stop (lfo_player);
        delete_fluid_player(lfo_player);
    }
    lfo_player = new_fluid_player(lfo_synth);
    fluid_player_set_playback_callback(lfo_player, lfo_parse_midi_events, lfo_synth);
    fluid_player_set_tick_callback (lfo_player, lfo_parse_ticks, lfo_synth);
    if (fluid_is_midifile(mid_file)) {
        fluid_player_add(lfo_player, mid_file);
    }
    fluid_player_set_tempo (lfo_player, FLUID_PLAYER_TEMPO_EXTERNAL_BPM, (double)tempo);
    fluid_player_play (lfo_player);
}

void
metronome_lfo_player_destruct () {
    /* wait for playback termination */
    fluid_synth_all_sounds_off (lfo_synth, -1);
    if (lfo_player) {
        fluid_player_stop (lfo_player);
        fluid_player_join(lfo_player);
        delete_fluid_player(lfo_player);
        lfo_player = NULL;
    }
    /* cleanup */
    if (lfo_adriver) {
        delete_fluid_audio_driver(lfo_adriver);
        lfo_adriver = NULL;
    }
    if (lfo_synth) {
        delete_fluid_synth(lfo_synth);
        lfo_synth = NULL;
    }
    if (lfo_settings) {
        delete_settings(METRONOME_PLAYER);
    }
}

void
metronome_lfo_player_play () {
    if (lfo_player) {
        fluid_player_stop (lfo_player);
        fluid_player_seek (lfo_player, 0);
        fluid_player_play (lfo_player);
    }
}

void
metronome_lfo_player_set_tempo (int tempo) {
    if (lfo_player) {
        fluid_player_set_tempo (lfo_player, FLUID_PLAYER_TEMPO_EXTERNAL_BPM, (double)tempo);
    }
}
