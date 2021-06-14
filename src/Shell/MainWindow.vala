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
 * Authored by: Subhadeep Jasu
 */
namespace Ensembles.Shell { 
    public class MainWindow : Gtk.Window {
        StyleControllerView style_controller_view;
        BeatCounterView beat_counter_panel;
        MainDisplayCasing main_display_unit;
        ControlPanel ctrl_panel;
        AppMenuView app_menu;
        Ensembles.Core.Synthesizer synthesizer;
        Ensembles.Core.StyleDiscovery style_discovery;
        Ensembles.Core.StylePlayer style_player;
        Ensembles.Core.CentralBus bus;
        Ensembles.Core.Controller controller_connection;

        string sf_loc = Constants.PKGDATADIR + "/SoundFonts/EnsemblesGM.sf2";
        public MainWindow () {
            Gtk.Settings settings = Gtk.Settings.get_default ();
            settings.gtk_application_prefer_dark_theme = true;
            bus = new Ensembles.Core.CentralBus ();
            make_bus_events ();

            beat_counter_panel = new BeatCounterView ();
            var headerbar = new Gtk.HeaderBar ();
            headerbar.has_subtitle = false;
            headerbar.set_show_close_button (true);
            headerbar.title = "Ensembles";
            headerbar.pack_start (beat_counter_panel);

            Gtk.Button app_menu_button = new Gtk.Button.from_icon_name ("preferences-system-symbolic", Gtk.IconSize.BUTTON);
            headerbar.pack_end (app_menu_button);
            this.set_titlebar (headerbar);

            app_menu = new AppMenuView (app_menu_button);

            app_menu_button.clicked.connect (() => {
                app_menu.popup ();
            });

            main_display_unit = new MainDisplayCasing ();

            ctrl_panel = new ControlPanel ();

            style_controller_view = new StyleControllerView ();

            var grid = new Gtk.Grid ();
            grid.attach (main_display_unit, 0, 0, 1, 1);
            grid.attach (ctrl_panel, 1, 0, 1, 1);
            grid.attach (style_controller_view, 0, 1, 2, 1);
            this.add (grid);
            this.show_all ();
            




            controller_connection = new Ensembles.Core.Controller ();
            app_menu.change_enable_midi_input.connect ((enable) => {
                if (enable) {
                    var devices_found = controller_connection.get_device_list ();
                    app_menu.update_devices (devices_found);
                }
            });
            synthesizer = new Ensembles.Core.Synthesizer (sf_loc);
            style_player = new Ensembles.Core.StylePlayer ();

            style_discovery = new Ensembles.Core.StyleDiscovery ();
            style_discovery.analysis_complete.connect (() => {
                style_player.add_style_file (style_discovery.style_files.nth_data (0));
                main_display_unit.update_style_list (
                    style_discovery.style_files,
                    style_discovery.style_names,
                    style_discovery.style_genre,
                    style_discovery.style_tempo
                );
            });

            make_ui_events ();
        }
        void make_bus_events () {
            bus.clock_tick.connect (() => {
                beat_counter_panel.sync ();
                main_display_unit.set_measure_display (Ensembles.Core.CentralBus.get_measure ());
                this.queue_draw ();
            });
            bus.system_halt.connect (() => {
                style_player.add_style_file (style_discovery.style_files.nth_data (0));
                beat_counter_panel.halt ();
            });
            bus.system_ready.connect (() => {
                main_display_unit.queue_remove_splash ();
                style_controller_view.ready ();
            });
            bus.style_section_change.connect ((section) => {
                style_controller_view.set_style_section (section);
            });
            bus.loaded_tempo_change.connect ((tempo) => {
                beat_counter_panel.change_tempo (tempo);
                main_display_unit.set_tempo_display (tempo);
            });
        }
        void make_ui_events () {
            app_menu.change_active_input_device.connect ((device) => {
                //  print("%d %s\n", device.id, device.name);
                controller_connection.connect_device (device.id);
            });
            ctrl_panel.accomp_change.connect ((active) => {
                print("on\n");
                synthesizer.set_accompaniment_on (active);
            });
            controller_connection.receive_note_event.connect ((key, on, velocity)=>{
                //  print ("%d %d %d\n", key, on, velocity);
                synthesizer.send_notes_realtime (key, on, velocity);
            });
            style_controller_view.start_stop.connect (() => {
                style_player.play_style ();
            });

            style_controller_view.switch_var_a.connect (() => {
                style_player.switch_var_a ();
            });

            style_controller_view.switch_var_b.connect (() => {
                style_player.switch_var_b ();
            });

            style_controller_view.switch_var_c.connect (() => {
                style_player.switch_var_c ();
            });

            style_controller_view.switch_var_d.connect (() => {
                style_player.switch_var_d ();
            });

            style_controller_view.queue_intro_a.connect (() => {
                style_player.queue_intro_a ();
            });

            style_controller_view.queue_intro_b.connect (() => {
                style_player.queue_intro_b ();
            });

            style_controller_view.queue_ending_a.connect (() => {
                style_player.queue_ending_a ();
            });

            style_controller_view.queue_ending_b.connect (() => {
                style_player.queue_ending_b ();
            });

            style_controller_view.break_play.connect (() => {
                style_player.break_play ();
            });

            style_controller_view.sync_stop.connect (() => {
                style_player.sync_stop ();
            });
            synthesizer.detected_chord.connect ((chord) => {
                style_player.change_chords (chord, 0);
            });
            print("Initialized...\n");
        }
    }
}