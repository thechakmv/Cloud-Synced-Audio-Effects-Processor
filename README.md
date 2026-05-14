# Cloud Synced Audio Effects Processor

An embedded audio effects unit that **reshapes its EQ in real time based on the genre of whatever Spotify is currently playing**.

Plug your headphones (or speakers) in, hit play in Spotify on any device, and the Teensy quietly reads the "now playing" track, looks up the artist's genres, and dials in a tone curve tuned for that style — heavier bass for hip-hop, sparkle on top for electronic, a flatter response for jazz, and so on. No buttons. No app. Just music in, better-sounding music out.

---

## What it does

- Reads Spotify's *currently-playing* endpoint over Wi-Fi and detects the artist's genre tags
- Maps those tags to a curated set of **3-band EQ presets** (low / mid / high shelf gains)
- Applies the preset to a live stereo audio stream running through a Teensy 4.0 + SGTL5000 audio shield
- Runs entirely on the device — once configured, no laptop or app needs to be in the loop

## How it works

```
 ┌────────────┐   line-in    ┌───────────────────┐   line-out    ┌────────────┐
 │ Audio src  │ ───────────► │  Teensy 4.0       │ ────────────► │  Headphones│
 │ (any RCA / │              │  + SGTL5000       │               │  / speakers│
 │  3.5mm)    │              │  Audio Shield     │               │            │
 └────────────┘              │                   │               └────────────┘
                             │  ┌─────────────┐  │
                             │  │ AudioFilter │  │
                             │  │   Biquad    │  │   ◄── EQ params updated
                             │  │  (per chan) │  │       on track change
                             │  └─────────────┘  │
                             └─────────┬─────────┘
                                       │ Wi-Fi via ESP-AT
                                       ▼
                           Spotify Web API (currently-playing,
                                       artist genres)
```

The DSP runs in interrupt context on the Audio Library's biquad filters, so the EQ change is sample-accurate and adds zero latency. The Wi-Fi / Spotify polling lives in the main loop, rate-limited to once every few seconds, and only nudges the EQ when the track actually changes.

## Hardware

- **Teensy 4.0** (ARM Cortex-M7 @ 600 MHz)
- **Teensy Audio Shield Rev D** (SGTL5000 codec, line-in / line-out / headphone out)
- **ESP8266 / ESP-01** running ESP-AT firmware, wired to Teensy `Serial1` (pins 0/1) for Wi-Fi

## Software stack

- PlatformIO with the [tsandmann/platform-teensy](https://github.com/tsandmann/platform-teensy) toolchain
- Teensy Audio Library for the DSP graph
- [WiFiEspAT](https://github.com/jandrassy/WiFiEspAT) — Arduino TCP/SSL client over the ESP-AT modem
- [ArduinoJson](https://arduinojson.org/) v7 for parsing Spotify responses

## Project layout

```
src/
├── main.cpp                boot + loop()
├── audio_chain.{h,cpp}     DSP graph: I2S in → biquads → I2S out
├── presets.{h,cpp}         genre → DspParams lookup table
├── preset_manager.{h,cpp}  routes genre events + UI overrides to the chain
├── spotify.{h,cpp}         Wi-Fi, OAuth refresh, currently-playing poll
├── config.h                compile-time settings (sample rate, poll period, etc.)
└── secrets.h               Wi-Fi creds + Spotify client/refresh tokens (gitignored)
```

## Genre presets (excerpt)

| Genre family | Low shelf | High shelf |
|---|---:|---:|
| Hip-hop / Trap | +5 / +6 dB | +2 dB |
| Drum & Bass / Dubstep | +5 / +6 dB | +4 / +3 dB |
| House / Techno | +3 / +4 dB | +2 / +3 dB |
| Rock / Metal | +2 / +3 dB | +2 dB |
| Jazz / Classical | 0 / −1 dB | +2 / +3 dB |
| Lo-fi | +2 dB | −2 dB |

(Full table lives in [`src/presets.cpp`](src/presets.cpp). Falls back to flat if no genre matches.)

## Building & flashing

```bash
# 1. Copy secrets.h.example to src/secrets.h and fill in Wi-Fi + Spotify creds
# 2. Build & upload from PlatformIO
pio run -t upload
# 3. Monitor serial
pio device monitor -b 115200
```

A successful boot prints:

```
Cloud Synced Audio Effects Processor
Audio chain initialized
[spotify] joining 'YOUR_SSID'
[spotify] IP 192.168.x.x
[boot] complete
[spotify] track=… genres=[hip hop,trap]
[preset] genre='hip hop,trap' -> low=+5.0 dB high=+2.0 dB
```

## Why I built it

I wanted a small, self-contained DSP project that bridged two things I don't usually combine — real-time embedded audio and consumer web APIs — and that solved a real (if mild) annoyance: bass-heavy genres sound thin and acoustic genres sound muddy on the same EQ curve. Letting the music itself drive the tone shaping turned out to be a fun, mostly-invisible upgrade to my listening setup.

## License

MIT — see [LICENSE](LICENSE).
