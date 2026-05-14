/*
 * preset_mgr — routes genre into audio_chain.
 *
 *   spotify_client::poll() ── on track change ──► on_genre(csv)
 *   ui::tick()             ── encoder press   ──► apply_ui_override / clear
 */

#include "preset_manager.h"

#include "config.h"
#include "presets.h"

namespace presets {

static char       s_last_genre[160] = {0};
static bool       s_override = false;
static DspParams  s_override_params = { 0.0f, 0.0f, 0.0f };

void begin() {}

void on_genre(const char *genre_csv) {
    if (!genre_csv) return;
    strncpy(s_last_genre, genre_csv, sizeof(s_last_genre) - 1);
    s_last_genre[sizeof(s_last_genre) - 1] = 0;

    if (s_override) {
        Serial.printf("[preset] genre='%s' (override active — not applying)\n",
                      genre_csv);
        return;
    }
    const DspParams &p = preset_lookup(genre_csv);
    Serial.printf("[preset] genre='%s' -> low=%+.1f dB high=%+.1f dB\n",
                  genre_csv, p.low_db, p.high_db);
    audio_chain::apply(p);
}

void apply_ui_override(const DspParams &params) {
    s_override = true;
    s_override_params = params;
    Serial.println("[preset] ui override engaged");
    audio_chain::apply(s_override_params);
}

void clear_ui_override() {
    s_override = false;
    Serial.printf("[preset] ui override cleared; resuming '%s'\n",
                  s_last_genre);
    audio_chain::apply(preset_lookup(s_last_genre));
}

const char *last_genre() { return s_last_genre; }
bool ui_override_active() { return s_override; }

}
