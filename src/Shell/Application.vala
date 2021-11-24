/*
 * Copyright 2020-2022 Subhadeep Jasu <subhajasu@gmail.com>
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

namespace Ensembles.Shell {
    public class EnsemblesApp : Gtk.Application {
        static EnsemblesApp _instance = null;

        public static EnsemblesApp instance {
            get {
                if (_instance == null) {
                    _instance = new EnsemblesApp ();
                }
                return _instance;
            }
        }
        string version_string = "";

        public static Settings settings;

        public static Ensembles.Shell.MainWindow main_window;

        Gtk.CssProvider css_provider;

        string[] ? arg_file = null;

        construct {
            settings = new Settings ("com.github.subhadeepjasu.ensembles");
        }

        public EnsemblesApp () {
            Object (
                application_id: "com.github.subhadeepjasu.ensembles",
                flags: ApplicationFlags.HANDLES_OPEN
            );
            version_string = "1.0.0";
        }

        protected override void activate () {
            if (this.main_window == null) {
                Core.DriverSettingsProvider.check_drivers ();
                this.main_window = new Ensembles.Shell.MainWindow ();
                Hdy.init ();
                var media_key_listener = Interfaces.MediaKeyListener.listen ();
                media_key_listener.media_key_pressed_play.connect (main_window.media_toggle_play);
                media_key_listener.media_key_pressed_pause.connect (main_window.media_pause);
                media_key_listener.media_key_pressed_prev.connect (main_window.media_prev);
                this.add_window (main_window);
                var sound_indicator_listener = Interfaces.SoundIndicator.listen (main_window);
                main_window.song_player_state_changed.connect_after (sound_indicator_listener.change_song_state);

                init_theme ();
            }
            if (css_provider == null) {
                css_provider = new Gtk.CssProvider ();
                css_provider.load_from_resource ("/com/github/subhadeepjasu/ensembles/Application.css");
                // CSS Provider
                Gtk.StyleContext.add_provider_for_screen (
                    Gdk.Screen.get_default (),
                    css_provider,
                    Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
                );
            }
            this.main_window.show_all ();
        }

        public override void open (File[] files, string hint) {
            activate ();
            if (files [0].query_exists ()) {
                main_window.open_file (files [0]);
            }
        }

        public override int command_line (ApplicationCommandLine cmd) {
            command_line_interpreter (cmd);
            return 0;
        }

        private void command_line_interpreter (ApplicationCommandLine cmd) {
            string[] args_cmd = cmd.get_arguments ();
            unowned string[] args = args_cmd;

            GLib.OptionEntry [] options = new OptionEntry [2];
            options [0] = { "", 0, 0, OptionArg.STRING_ARRAY, ref arg_file, null, "URI" };
            options [1] = { null };

            var opt_context = new OptionContext ("actions");
            opt_context.add_main_entries (options, null);
            try {
                opt_context.parse (ref args);
            } catch (Error err) {
                warning (err.message);
                return;
            }

            if (GLib.FileUtils.test (arg_file[0], GLib.FileTest.EXISTS) && arg_file[0].down ().has_suffix (".mid")) {
                File file = File.new_for_path (arg_file[0]);
                open ({ file }, "");
                return;
            }

            activate ();
        }

        public static bool get_is_running_from_flatpak () {
            var flatpak_info = File.new_for_path ("/.flatpak-info");
            return flatpak_info.query_exists ();
        }

        private void init_theme () {
            GLib.Value value = GLib.Value (GLib.Type.STRING);
            Gtk.Settings.get_default ().get_property ("gtk-theme-name", ref value);
            if (!value.get_string ().has_prefix ("io.elementary.")) {
                Gtk.Settings.get_default ().set_property ("gtk-icon-theme-name", "elementary");
                Gtk.Settings.get_default ().set_property ("gtk-theme-name", "io.elementary.stylesheet.blueberry");
            }
        }
    }
}
