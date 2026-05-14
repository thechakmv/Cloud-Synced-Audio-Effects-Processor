#include <Arduino.h>

#include "audio_chain.h"
#include "config.h"
#include "preset_manager.h"
#include "spotify.h"

    

void setup() {
    Serial.begin(115200);
    int t0 = millis();
    while ((millis() - t0) < 2000) { /* wait for USB host briefly */ }
    Serial.println("\nCloud Synced Audio Effects Processor");
    
    if (!audio_chain::begin()) {
        Serial.println("FATAL: audio_chain failed — check Audio Shield seating");
        while (1) { delay(1000); }
    }

    presets::begin();

    if (!spotify::begin()) {
        Serial.println("[boot] Spotify init failed; running offline pass-through");
    }

    //ui::begin();
    Serial.println("[boot] complete");

}   

void loop() {
    spotify::poll();   // rate-limited internally
    //presets::tick();       // advance any in-flight parameter ramp
    //ui::tick();
    delay(1);
}
