#pragma once
// --- Audio ---
// Sample rate is fixed by the Teensy Audio Library (44.1 kHz on SGTL5000).
#define APP_SAMPLE_RATE_HZ    44100

// --- Default headphone volume on the SGTL5000 (0.0 .. 1.0) ---
#define APP_HP_VOLUME         0.5f

// --- Serial port for the ESP-AT modem ---
// Serial1 = pins 0/1 on Teensy 4.0
#define AT_SERIAL             Serial1
#define AT_BAUD               115200

// --- Spotify ---
#define SPOTIFY_POLL_PERIOD_MS  5000

// Public-facing Spotify creds + Wi-Fi creds live in secrets.h (not in git).
// Copy secrets.example.h -> secrets.h and fill in.

#include "secrets.h"