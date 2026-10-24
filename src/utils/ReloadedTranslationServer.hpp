#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/translation.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The one catalogue Godot's i18n translates with, in its selected language: the
    /// wrapper's own .po files (`reloaded/locale/translations`) with `<language>.po` of the
    /// catalogue directory a plugin gives (translation_set_catalogue_dir()) merged over them, so
    /// the data's translation of a msgid wins. Without one only the wrapper's own strings are
    /// translated.
    ///
    /// It is the only translation in TranslationServer: every Control, `tr()` and `atr()` looks
    /// up through it, and re-translates itself when the language changes.
    class ReloadedTranslationServer : public Object {
            GDCLASS(ReloadedTranslationServer, Object)

        public:
            static ReloadedTranslationServer *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline ReloadedTranslationServer *singleton = nullptr;

        public:
            /// The wrapper's own .po files, merged under the game's catalogue
            static constexpr const char *TRANSLATIONS_SETTING = "reloaded/locale/translations";
            /// The original's own strings, untranslated - no catalogue needed.
            static constexpr const char *DEFAULT_LANGUAGE = "en";
            /// The game's data was read again, or another catalogue directory given:
            /// translation_get_languages() lists what it has now
            static constexpr const char *TRANSLATION_LANGUAGES_CHANGED_SIGNAL = "translation_languages_changed";

        private:
            /// The merged catalogue registered in TranslationServer
            Ref<Translation> translation;
            PackedStringArray languages;
            String language = DEFAULT_LANGUAGE;
            /// Where the data's catalogues are, one `<language>.po` each; none without a plugin
            String catalogue_dir;

            void _on_data_reload_requested();
            void _on_language_changed();

        protected:
            static void _bind_methods();

        public:
            ReloadedTranslationServer();
            ~ReloadedTranslationServer() override;

            /// Replaces the game's catalogue with the one in `p_po_path` and puts the merged catalogue
            /// in TranslationServer in place of the previous one; with no such file only the
            /// wrapper's own strings are translated, as in the original (translation.cpp:21)
            void translation_load(const String &p_po_path);
            /// The directory of the data's catalogues: the languages are read from it again
            void translation_set_catalogue_dir(const String &p_dir);
            /// The language of the game's strings, by the name of its catalogue.
            void set_language(const String &p_language);
            String get_language() const;
            /// The languages there is a catalogue for in the catalogue directory, English (the msgids
            /// themselves) always first
            PackedStringArray translation_get_languages() const;
    };
} // namespace godot
