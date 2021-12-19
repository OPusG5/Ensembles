/*
 * Copyright 2020-2022 Subhadeep Jasu <subhajasu@gmail.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "synthesizer.h"

fluid_synth_t* realtime_render_synth;


// Accompaniment Flags
int accompaniment_mode = 0;

// Voice Settings
int soundfont_id = 0;
int synthesizer_voice_bank_l = 0;
int synthesizer_voice_program_l = 0;
int synthesizer_voice_bank_r1 = 0;
int synthesizer_voice_program_r1 = 0;
int synthesizer_voice_bank_r2 = 0;
int synthesizer_voice_program_r3 = 0;

// Equalizer
int velocity_buffer[20];


// Global scale shift
int synthesizer_transpose = 0;
int synthesizer_transpose_enable = 0;
int synthesizer_octave = 0;
int synthesizer_octave_shifted = 0;

void
synthesizer_edit_master_reverb (int level) {
    if (realtime_render_synth) {
        fluid_synth_set_reverb_group_roomsize (realtime_render_synth, -1, get_reverb_room_size(level));
        fluid_synth_set_reverb_group_damp (realtime_render_synth, -1, 0.1);
        fluid_synth_set_reverb_group_width (realtime_render_synth, -1, get_reverb_width(level));
        fluid_synth_set_reverb_group_level (realtime_render_synth, -1, get_reverb_level(level));
    }
}

void
synthesizer_set_master_reverb_active (int active) {
    if (realtime_render_synth)
        fluid_synth_reverb_on (realtime_render_synth, -1, active);
}

void
synthesizer_edit_master_chorus (int level) {
    if (realtime_render_synth) {
        fluid_synth_set_chorus_group_depth (realtime_render_synth, -1, get_chorus_depth(level));
        fluid_synth_set_chorus_group_level (realtime_render_synth, -1, get_chorus_level(level));
        fluid_synth_set_chorus_group_nr (realtime_render_synth, -1, get_chorus_nr(level));
    }
}

void
synthesizer_set_master_chorus_active (int active) {
    if (realtime_render_synth)
        fluid_synth_chorus_on (realtime_render_synth, -1, active);
}

void
synthesizer_set_defaults () {
    // Global reverb and chorus levels
    synthesizer_edit_master_reverb (5);
    synthesizer_edit_master_chorus (1);

    // CutOff for Realtime synth
    fluid_synth_cc (realtime_render_synth, 17, 74, 40);
    fluid_synth_cc (realtime_render_synth, 18, 74, 0);
    fluid_synth_cc (realtime_render_synth, 19, 74, 0);

    // Reverb and Chorus for R1 voice
    fluid_synth_cc (realtime_render_synth, 17, 91, 4);
    fluid_synth_cc (realtime_render_synth, 17, 93, 1);

    // Reverb and Chorus for Metronome
    fluid_synth_cc (realtime_render_synth, 16, 91, 0);
    fluid_synth_cc (realtime_render_synth, 16, 93, 0);

    // Default gain for Realtime synth
    fluid_synth_cc (realtime_render_synth, 17, 7, 100);
    fluid_synth_cc (realtime_render_synth, 18, 7, 90);
    fluid_synth_cc (realtime_render_synth, 19, 7, 80);


    // Default pitch of all synths
    for (int i = 17; i < 64; i++) {
        fluid_synth_cc (realtime_render_synth, i, 3, 64);
    }

    // Default cut-off and resonance for recorder
    for (int i = 24; i < 64; i++) {
        fluid_synth_cc (realtime_render_synth, i, 74, 40);
        fluid_synth_cc (realtime_render_synth, i, 71, 10);
    }

    // Default pitch for styles
    for (int i = 0; i < 16; i++) {
        fluid_synth_cc (realtime_render_synth, i, 3, 64);
    }
}

void
synthesizer_init(const char* loc, const char* dname, double buffer_size)
{
    set_driver_configuration(dname, buffer_size);
    realtime_render_synth = get_synthesizer(RENDER);
    if (fluid_is_soundfont(loc)) {
        soundfont_id = fluid_synth_sfload(realtime_render_synth, loc, 1);

        // Initialize voices
        fluid_synth_program_select (realtime_render_synth, 17, soundfont_id, 0, 0);
        fluid_synth_program_select (realtime_render_synth, 18, soundfont_id, 0, 49);
        fluid_synth_program_select (realtime_render_synth, 19, soundfont_id, 0, 33);

        // Initialize chord voices
        fluid_synth_program_select (realtime_render_synth, 20, soundfont_id, 0, 5);
        fluid_synth_program_select (realtime_render_synth, 21, soundfont_id, 0, 33);
        fluid_synth_program_select (realtime_render_synth, 22, soundfont_id, 0, 49);

        // Initialize metronome voice
        fluid_synth_program_select (realtime_render_synth, 16, soundfont_id, 128, 0);
    }

    synthesizer_set_defaults ();
}

void
synthesizer_send_notes_to_metronome(int key, int on)
{
    if (realtime_render_synth)
    {
        if (on == 144)
        {
            fluid_synth_noteon(realtime_render_synth, 16, key, 127);
        }
        else
        {
            fluid_synth_noteoff(realtime_render_synth, 16, key);
        }
    }
}

int
synthesizer_set_driver_configuration(const char* dname, double buffer_size)
{
    return set_driver_configuration(dname, buffer_size);
}

void
synthesizer_change_voice (int bank, int preset, int channel) {
    printf ("Voice: %d, Channel: %d\n", preset, channel);
    if (realtime_render_synth) {
        fluid_synth_program_select (realtime_render_synth, channel, soundfont_id, bank, preset);
    }
}

void
synthesizer_change_modulator (int channel, int modulator, int value) {
    if (realtime_render_synth) {
        fluid_synth_cc (realtime_render_synth, channel, modulator, value);
        if (channel < 16) {
            if (modulator == 7) {
                // printf ("%d, %d\n", channel, value);
                set_gain_value (channel, value);
            }
            if (modulator == 3 || modulator == 10) {
                set_mod_buffer_value (modulator, channel, value >= -64 ? value : -64);
            } else {
                set_mod_buffer_value (modulator, channel, value >= 0 ? value : 0);
            }
        }
    }
}

int
synthesizer_get_modulator_values (int channel, int modulator) {
    int mod_value = -1;
    if (realtime_render_synth) {
        fluid_synth_get_cc (realtime_render_synth, channel, modulator, &mod_value);
    }
    return mod_value;
}

int
synthesizer_get_velocity_levels (int channel) {
    return velocity_buffer [channel];
}


void
synthesizer_destruct () {
    printf ("Stopping Synthesizers\n");
    fluid_synth_all_sounds_off (realtime_render_synth, -1);
}

int
handle_events_for_midi_players (fluid_midi_event_t *event, int _is_style_player) {
    int type = fluid_midi_event_get_type(event);
    int chan = fluid_midi_event_get_channel(event);
    int cont = fluid_midi_event_get_control(event);
    int value= fluid_midi_event_get_value (event);

    // printf ("Type: %d, ", type);
    // printf ("Channel: %d, ", chan);
    // printf ("Control: %d, ", cont);
    // printf ("Value: %d\n", value);
    if (_is_style_player == 1)
    {
        if (type == 176)
        {
            if (cont == 85 && (value == 1 || value == 8 || value == 16 || value == 126)) {
                int sf_id, program_id, bank_id;
                fluid_synth_get_program (realtime_render_synth, chan, &sf_id, &bank_id, &program_id);
                fluid_synth_program_select (realtime_render_synth, chan, soundfont_id, value, program_id);
            }
            if (cont == 7) {
                if (get_gain_value(chan) >= 0) {
                    fluid_midi_event_set_value (event, get_gain_value(chan));
                }
            }
            if (cont == 16) {
                if (get_mod_buffer_value (16, chan) >= -64) {
                    fluid_midi_event_set_value (event, get_mod_buffer_value (9, chan));
                }
            } else if (cont == 10) {
                if (get_mod_buffer_value (10, chan) >= -64) {
                    fluid_midi_event_set_value (event, get_mod_buffer_value (10, chan));
                }
            } else {
                if (get_mod_buffer_value (cont, chan) >= 0) {
                    fluid_midi_event_set_value (event, get_mod_buffer_value (cont, chan));
                }
            }
        }
        if (chan != 9 && get_central_accompaniment_mode () == 0 && type == 144) {
            return 0;
        }
        if (type == 144)
        {
            velocity_buffer[chan] = value;
        }
        else if (type == 128)
        {
            velocity_buffer[chan] = 0;
        }
    }
    int ret_val = 0;
    if (realtime_render_synth) {
        ret_val = fluid_synth_handle_midi_event(realtime_render_synth, event);
    }
    if (event) {
        delete_fluid_midi_event (event);
    }
    return ret_val;
}

int
synthesizer_send_notes (int key, int on, int velocity, int channel, int* type)
{
    if (realtime_render_synth)
    {
        if (channel == 17 || channel == 24)
        {
            if (get_central_accompaniment_mode () > 0)
            {
                if (accompaniment_mode == 0)
                {
                    if (key <= get_central_split_key ())
                    {
                        int chrd_type = 0;
                        int chrd_main = chord_finder_infer (key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0), on, &chrd_type);
                        *type = chrd_type;
                        if (get_central_style_looping() == 0 && get_central_style_sync_start() == 0 && on == 144)
                        {
                            fluid_synth_all_notes_off (realtime_render_synth, 21);
                            fluid_synth_cc (realtime_render_synth, 20, 91, 0);
                            fluid_synth_cc (realtime_render_synth, 21, 91, 0);
                            fluid_synth_noteon (realtime_render_synth, 20, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 12, velocity * 0.6);
                            fluid_synth_noteon (realtime_render_synth, 21, chrd_main + 36, velocity);
                            fluid_synth_noteon (realtime_render_synth, 22, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 36, velocity * 0.2);
                            fluid_synth_noteon (realtime_render_synth, 22, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 24, velocity * 0.4);
                        }
                        if (on == 128)
                        {
                            fluid_synth_noteoff (realtime_render_synth, 20, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 12);
                            fluid_synth_all_notes_off (realtime_render_synth, 21);
                            fluid_synth_noteoff (realtime_render_synth, 22, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 36);
                            fluid_synth_noteoff (realtime_render_synth, 22, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0) + 24);
                        }
                        return chrd_main;
                    }
                }

            } else if (get_central_split_on () > 0) {
                if (key <= get_central_split_key ()) {
                    if (on == 144) {
                        fluid_synth_noteon(realtime_render_synth, 19, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0), velocity);
                        velocity_buffer[19] = velocity;
                    } else if (on == 128) {
                        fluid_synth_noteoff(realtime_render_synth, 19, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0));
                        velocity_buffer[19] = 0;
                    }
                    return -6;
                }
            }
            if (on == 144) {
                fluid_synth_noteon(realtime_render_synth, channel < 0 ? 0 : channel, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0), velocity);
                velocity_buffer[17] = velocity;
            } else if (on == 128) {
                fluid_synth_noteoff(realtime_render_synth, channel < 0 ? 0 : channel, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0));
                velocity_buffer[17] = 0;
            }
            if (get_central_layer_on () > 0) {
                if (on == 144) {
                    fluid_synth_noteon (realtime_render_synth, 18, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0), velocity);
                    velocity_buffer[18] = velocity;
                } else if (on == 128) {
                    fluid_synth_noteoff (realtime_render_synth, 18, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0));
                    velocity_buffer[18] = 0;
                }
            }
        } else {
            if (on == 144) {
                fluid_synth_noteon (realtime_render_synth, channel, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0), velocity);
                velocity_buffer[17] = velocity;
            } else if (on == 128) {
                fluid_synth_noteoff (realtime_render_synth, channel, key + ((synthesizer_octave_shifted > 0) ? (synthesizer_octave * 12) : 0) + ((synthesizer_transpose_enable > 0) ? synthesizer_transpose : 0));
                velocity_buffer[17] = 0;
            }
        }
    }
    return -6;
}

void
synthesizer_halt_notes () {
    printf ("Halt\n");
    if (realtime_render_synth) {
        for (int i = 0; i < 16; i++) {
            if (i != 9 && i != 10) {
                fluid_synth_all_notes_off (realtime_render_synth, i);
            }
        }
    }
}

void
synthesizer_halt_realtime () {
    if (realtime_render_synth) {
        fluid_synth_all_notes_off (realtime_render_synth, 17);
        fluid_synth_all_notes_off (realtime_render_synth, 18);
        fluid_synth_all_notes_off (realtime_render_synth, 19);

        for (int i = 20; i < 64; i++) {
            fluid_synth_all_notes_off (realtime_render_synth, i);
        }
    }
}


void
synthesizer_set_accomp_enable (int on) {
    set_central_accompaniment_mode (on);
}

float
synthesizer_get_version () {
    int major_version = 0;
    int minor_version = 0;
    int macro_version = 0;
    fluid_version (&major_version, &minor_version, &macro_version);
    return (float)major_version + (0.1f * minor_version);
}
