# BUILD NOTES — Formwave v0.1 (Koda build, Nov 21, 2026)

Built from Formwave-v0.1.zip exactly as delivered. NO source changes were needed:
the project already pins JUCE 7.0.12 and already targets Windows 8.1.

Verified on Linux (this machine):
- Core test suite: 26 passed, 0 failed (also renders demo.wav — one bar of
  boom-bap played entirely by factory presets; included in the Linux package).
- Full CMake build: EXIT 0 — Standalone (11.0 MB) and VST3 (11.7 MB) produced.
- VST3 loads: dlopen OK, GetPluginFactory present.
- Standalone RUNS — screenshot-verified: full UI renders (harmonics editor,
  waveform/spectrum, 4 oscillators, filter + ADSR, effects strip, keyboard).
  Per the README this is the first time the plug-in/UI code has been linked
  and run anywhere.

First-run observation (cosmetic): numeric readouts show long decimals
(e.g. 0.0000000 dB, 1.0000001 x) under the knobs. Everything else rendered
complete at first look.

Next step for Windows/Mac: create a GitHub repo for Formwave, upload the
GitHub-Ready package contents, and its own workflow builds Windows (x64 +
32-bit) and Mac automatically.

- Nov 21, 2026 (Koda): the first GitHub run failed before any job started — the workflow's upload-artifact step used a one-line { } block that YAML cannot parse around ${{ }}. Fix prepared: expand that 'with:' block to indented lines (same change for the macOS step). Workflow file itself must be updated with the fixed version.
