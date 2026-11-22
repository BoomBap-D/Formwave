# Formwave

A sound-design synthesizer for hip-hop and beyond, as a **VST3 instrument** and a **standalone app**.
You can see every sound form: draggable harmonic bars, a live oscilloscope, a spectrum and a scrolling
spectrogram. Every sound is made from oscillators, noise, filters and envelopes, so all 218 factory presets
are editable starting points. (Formwave is a working name; check it for trademark conflicts before release.)

**Version 0.4.** Read "Status" before you rely on anything.

## What it does

- 4 oscillators: sine, triangle, saw, square, **additive** (32 harmonics you draw) and **metal** (six inharmonic
  squares for hats and bells). Each has ratio, fine tune, level and its own decay.
- Pitch envelope, FM, noise (white, pink, vinyl crackle) with its own filter, clap bursts and sweep
- Filter (low, high, band) with envelope, key tracking and LFO; drive
- Amp ADSR for notes, or a one-shot length for drums, plus a rising mode for risers
- Vibrato, glide, mono mode; pitch bend, mod wheel and sustain pedal
- **Unison** (up to 7 detuned, stereo-spread voices), **two LFOs** (sine, triangle, saw, square, random) aimed at pitch, cutoff, level, FM amount or pan, velocity to level and cutoff, mod wheel to vibrato and cutoff (Modulation tab)
- Chorus, ping-pong delay, reverb; output limiter
- 218 factory presets in 13 categories: kicks, 808s, snares, claps, hats and cymbals, percussion, plucks, strings and brass, bass, keys,
  leads, pads, FX. Save your own (Documents/Formwave/Presets, plain text `.fwp` files).
- Interface (v0.3): searchable preset browser with category chips and favourites, tabbed display (harmonics, waveform, spectrum and spectrogram), draggable filter curve, glyph wave selectors, on-screen pitch and mod wheels, A/B compare, and a status bar that explains what you hover over
- Resizable window, every knob is a host-automatable parameter, double-click a knob to reset it

## Build on Windows

You need Visual Studio 2022 (Desktop development with C++), CMake 3.22+ and Git, and internet for the
first configure (it downloads JUCE).

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target Formwave_VST3 Formwave_Standalone
```

Outputs: `build/Formwave_artefacts/Release/VST3/Formwave.vst3` and `.../Standalone/Formwave.exe`.
For an installer, build, copy the two outputs into `dist/`, then compile `installer/formwave.iss` with Inno Setup.
Push to GitHub and `.github/workflows/build.yml` builds Windows (64 and 32 bit) and Mac for you.

## Windows 8.1 through 11

JUCE 8 needs Windows 10 or later, so this project pins **JUCE 7.0.12**, targets Windows 8.1 and links the
C++ runtime statically. I could not confirm JUCE 7's exact Windows floor and nothing has been run on 8.1:
treat 8.1 support as the design, not a tested fact. Many current DAWs no longer run on 8.1 either; the
standalone app has no such limit.

## Licence: check before you sell

JUCE is AGPLv3 or a paid commercial licence. A closed-source product you sell needs the commercial licence.
The VST3 SDK has its own terms. Confirm both before release.

## Status

Tested (`tests/core_tests.cpp`, no JUCE needed, 40 checks): parameter table, all 67 presets render finite and
audible at three pitches and extreme settings, pitch accuracy, additive oscillator, note lifecycle, sustain
pedal, pitch bend, output limiter, effects tails and stereo, preset file round trip.

```
g++ -std=c++17 -O2 -I Source/Core tests/core_tests.cpp -o core_tests && ./core_tests demo.wav
```

`demo.wav` is a one-bar boom-bap played entirely by factory presets.

The plug-in and interface (`Source/Plugin`, `Source/UI`) pass a compiler syntax check against the JUCE 7.0.12
headers, but have **never been linked or run**: no JUCE build was possible where this was written. Expect
some small problems on the first real run (layout nudges, a missed repaint). Send me what you see.

Not built yet: unison and stereo spread, an LFO matrix, arpeggiator, wavetable import, preset tagging,
a resizable-text UI scale option, installer signing, and listening tests on real hardware.

## Layout

```
Source/Core/     framework-free engine (params, voice, effects, factory bank)
Source/Plugin/   JUCE processor, preset store, audio tap
Source/UI/       editor, views, look and feel
installer/       Inno Setup script
tests/           core tests
```
