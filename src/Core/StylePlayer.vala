/*-
 * Copyright (c) 2021-2022 Subhadeep Jasu <subhajasu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Authored by: Subhadeep Jasu <subhajasu@gmail.com>
 */


namespace Ensembles.Core {
    public class StylePlayer : Object {
        static int style_part;
        public StylePlayer (string? style_file = null) {
            style_player_init ();
            if (style_file != null) {
                style_player_add_style_file (style_file, 0);
            }

            set_style_change_callback ((state) => {
                if (Shell.RecorderScreen.sequencer != null && Shell.RecorderScreen.sequencer.current_state != MidiRecorder.RecorderState.PLAYING) {
                    var part_event = new MidiEvent ();
                    part_event.event_type = MidiEvent.EventType.STYLECONTROL;
                    part_event.value1 = style_part;

                    var event = new Core.MidiEvent ();
                    event.event_type = Core.MidiEvent.EventType.STYLESTARTSTOP;
                    event.value1 = state;

                    Shell.RecorderScreen.sequencer.record_event (part_event);
                    print ("Style: %d\n", state);
                    Shell.RecorderScreen.sequencer.record_event (event);
                }
                if (state == 1) {
                    Shell.RecorderScreen.sequencer.stop ();
                }
            });
        }
        ~StylePlayer () {
           style_player_destruct ();
        }

        string file_path;
        int tempo;

        public void add_style_file (string style_file, int tempo) {
            debug ("loading style %s\n", style_file);
            if (file_path != style_file || tempo != this.tempo) {
                file_path = style_file;
                this.tempo = tempo;
                if (Core.CentralBus.get_style_looping_on ()) {
                    sync_stop ();
                    Timeout.add (10, () => {
                        if (!Core.CentralBus.get_style_looping_on ()) {
                            int previous_tempo = Core.CentralBus.get_tempo ();
                            style_player_add_style_file (style_file, previous_tempo);
                            Idle.add (() => {
                                play_style ();
                                return false;
                            });
                        }
                        return Core.CentralBus.get_style_looping_on ();
                    });
                } else {
                    style_player_add_style_file (style_file, tempo);
                }
            }
        }

        public void reload_style () {
            style_player_reload_style ();
        }

        public void play_style () {
            style_player_toggle_play ();
        }

        public void stop_style () {
            style_player_stop ();
        }

        public void switch_var_a () {
            style_player_play_loop (3, 4);
            style_part = 2;
        }

        public void switch_var_b () {
            style_player_play_loop (5, 6);
            style_part = 3;
        }

        public void switch_var_c () {
            style_player_play_loop (7, 8);
            style_part = 4;
        }

        public void switch_var_d () {
            style_player_play_loop (9, 10);
            style_part = 5;
        }

        public void queue_intro_a () {
            style_player_queue_intro (1, 2);
            style_part = 0;
        }

        public void queue_intro_b () {
            style_player_queue_intro (2, 3);
            style_part  = 1;
        }

        public void queue_ending_a () {
            style_player_queue_ending (11, 12);
            style_part = 7;
        }

        public void queue_ending_b () {
            style_player_queue_ending (13, 14);
            style_part = 8;
        }

        public void break_play () {
            style_player_break ();
            style_part = 6;
        }

        public void sync_start () {
            style_player_sync_start ();
        }
        public void sync_stop () {
            style_player_sync_stop ();
        }

        public void change_chords (int chord_main, int chord_type) {
            style_player_change_chord (chord_main, chord_type);
        }

        public void change_tempo (int tempo) {
            style_player_set_tempo (tempo);
        }
    }
}

extern void style_player_init ();
extern void style_player_add_style_file (string mid_file, int custom_tempo);
extern void style_player_reload_style ();
extern void style_player_destruct ();
extern void style_player_toggle_play ();
extern void style_player_stop ();
extern void style_player_play_loop (int start, int end);
extern void style_player_queue_intro (int start, int end);
extern void style_player_queue_ending (int start, int end);
extern void style_player_break ();
extern void style_player_sync_start ();
extern void style_player_sync_stop ();
extern void style_player_set_tempo (int tempo_bpm);

extern void style_player_change_chord (int cd_main, int cd_type);

[CCode (cname = "style_player_change_state", has_target = false)]
extern delegate void style_player_change_state_callback (int started);
[CCode (has_target = false)]
extern void set_style_change_callback (style_player_change_state_callback function);
