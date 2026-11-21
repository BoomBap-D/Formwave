// Formwave core tests (no JUCE needed):
//   g++ -std=c++17 -O2 -Wall -Wextra -I Source/Core tests/core_tests.cpp -o core_tests && ./core_tests [demo.wav]
#include "Bank.h"

#include <cstdio>
#include <memory>
#include <set>

using namespace fw;

static int g_fail = 0, g_pass = 0;
#define CHECK(cond, msg) do { if (cond) { ++g_pass; std::printf ("  ok    %s\n", msg); } else { ++g_fail; std::printf ("  FAIL  %s  (line %d)\n", msg, __LINE__); } } while (0)

static void writeWav (const char* path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    FILE* f = std::fopen (path, "wb"); if (! f) return;
    const uint32_t n = (uint32_t) l.size(), bytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        const int16_t a = (int16_t) std::lround (std::max (-1.0f, std::min (1.0f, l[i])) * 32767.0f);
        const int16_t b = (int16_t) std::lround (std::max (-1.0f, std::min (1.0f, r[i])) * 32767.0f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

static void testParams()
{
    std::printf ("parameters\n");
    const auto& t = paramTable();
    CHECK ((int) t.size() == P_COUNT, "parameter table matches the index enum");
    std::set<std::string> ids; bool dup = false, okDef = true;
    for (const auto& d : t) { if (! ids.insert (d.id).second) dup = true; if (d.def < d.min || d.def > d.max) okDef = false; }
    CHECK (! dup, "parameter ids are unique");
    CHECK (okDef, "every default lies inside its range");
    CHECK (paramIndex ("h1") == P_HARM0 && paramIndex ("level") == P_LEVEL && paramIndex ("o4_decay") == oscIdx (3, O_DECAY), "ids map to the right indices");
}

static void testBank()
{
    std::printf ("factory bank\n");
    const auto& bank = factoryBank();
    CHECK (bank.size() >= 200, "at least 200 factory presets");
    { std::set<std::string> names; for (const auto& p : bank) names.insert (p.name); CHECK (names.size() == bank.size(), "preset names are unique"); }
    int bad = 0, hot = 0;
    for (const auto& p : bank)
        for (int note : { 36, 60, 84 })
        {
            const auto b = renderPatch (p, note, 1.0f, 0.6f);
            float pk = 0; bool fin = true;
            for (float x : b) { fin = fin && std::isfinite (x); pk = std::max (pk, std::fabs (x)); }
            if (! fin || pk < 1e-4f) { ++bad; std::printf ("    bad: %s note %d\n", p.name.c_str(), note); }
            if (pk > 2.5f) ++hot;
        }
    CHECK (bad == 0, "every preset renders finite, audible audio at three pitches");
    CHECK (hot == 0, "calibrated presets stay under 2.5 peak");
    int seen = 0; std::set<std::string> cats;
    for (const auto& p : bank) { cats.insert (p.category); ++seen; }
    CHECK (cats.size() == 13, "thirteen categories");

    // extreme settings must stay finite
    Patch x = bank[40];
    x.v[P_DRIVE] = 6; x.v[P_F_ON] = 1; x.v[P_F_Q] = 20; x.v[P_F_HZ] = 30; x.v[P_FM_I] = 12; x.v[P_LEVEL] = 18;
    bool fin = true;
    for (float s : renderPatch (x, 96, 1.0f, 0.6f)) fin = fin && std::isfinite (s);
    CHECK (fin, "extreme settings stay finite");
}

static void testSound()
{
    std::printf ("sound\n");
    Patch p = Patch::init();                 // sine, notes mode, C4 = 261.6 Hz
    p.v[P_LEVEL] = 0; p.v[P_A] = 0.001f; p.v[P_S] = 1.0f;
    const auto b = renderPatch (p, 60, 1.0f, 1.0f);
    int c = 0; for (size_t i = 5001; i < 5000 + 22050; ++i) if (b[i - 1] <= 0 && b[i] > 0) ++c;
    CHECK (std::fabs (c - 261.63 * 0.5) < 2.0, "init patch plays middle C at 261.6 Hz");
    p.v[P_OCT] = -2;
    const auto b2 = renderPatch (p, 60, 1.0f, 1.0f);
    c = 0; for (size_t i = 5001; i < 5000 + 22050; ++i) if (b2[i - 1] <= 0 && b2[i] > 0) ++c;
    CHECK (std::fabs (c - 65.41 * 0.5) < 1.5, "octave -2 gives 65.4 Hz");

    // additive: harmonic 2 only -> twice the frequency
    Patch a = Patch::init(); a.v[oscIdx (0, O_WAVE)] = W_ADD; a.v[P_LEVEL] = 0; a.v[P_S] = 1; a.v[P_A] = 0.001f;
    for (int k = 0; k < 32; ++k) a.v[P_HARM0 + k] = 0.0f;
    a.v[P_HARM0 + 1] = 1.0f;
    const auto b3 = renderPatch (a, 60, 1.0f, 1.0f);
    c = 0; for (size_t i = 5001; i < 5000 + 22050; ++i) if (b3[i - 1] <= 0 && b3[i] > 0) ++c;
    CHECK (std::fabs (c - 523.25 * 0.5) < 3.0, "additive oscillator with only harmonic 2 sounds an octave up");

    // envelope: a one-shot ends by itself, a note ends after release
    auto eOwner = std::make_unique<Engine>(); Engine& e = *eOwner; e.prepare (44100.0);   // heap: Engine is ~0.5 MB, three on the stack overflow Windows' 1 MB default
    Patch k = factoryBank()[0];
    e.setPatch (k.v.data());
    std::vector<float> L (44100 * 4), R (44100 * 4);
    e.noteOn (60, 1.0f); e.process (L.data(), R.data(), (int) L.size());
    CHECK (e.activeVoices() == 0, "one-shot voice frees itself");
    Patch pad = factoryBank()[50]; for (const auto& q : factoryBank()) if (q.name == "Warm Pad") pad = q;
    e.setPatch (pad.v.data()); e.noteOn (60, 1.0f);
    e.process (L.data(), R.data(), 22050);
    CHECK (e.activeVoices() == 1, "held note keeps sounding");
    e.noteOff (60); e.process (L.data(), R.data(), 44100 * 3);
    CHECK (e.activeVoices() == 0, "released note ends");

    // sustain pedal holds a released note until the pedal lifts
    e.noteOn (64, 1.0f); e.setSustain (true); e.noteOff (64);
    e.process (L.data(), R.data(), 44100 * 3);
    CHECK (e.activeVoices() == 1, "sustain pedal keeps the note");
    e.setSustain (false); e.process (L.data(), R.data(), 44100 * 3);
    CHECK (e.activeVoices() == 0, "note ends after the pedal lifts");

    // pitch bend moves the pitch by two semitones
    Patch s = Patch::init(); s.v[P_LEVEL] = 0; s.v[P_A] = 0.001f; s.v[P_S] = 1.0f;
    auto e2Owner = std::make_unique<Engine>(); Engine& e2 = *e2Owner; e2.prepare (44100.0); e2.setPatch (s.v.data()); e2.setBend (2.0f); e2.noteOn (69, 1.0f);
    std::vector<float> bl (44100), br (44100); e2.process (bl.data(), br.data(), 44100);
    c = 0; for (size_t i = 5001; i < 5000 + 22050; ++i) if (bl[i - 1] <= 0 && bl[i] > 0) ++c;
    CHECK (std::fabs (c - 493.88 * 0.5) < 3.0, "pitch bend +2 semitones raises A4 to B4");

    // output never exceeds 1.0 even when everything is turned up
    Patch loud = Patch::init(); loud.v[P_LEVEL] = 18; loud.v[P_DRIVE] = 6; loud.v[P_S] = 1;
    auto e3Owner = std::make_unique<Engine>(); Engine& e3 = *e3Owner; e3.prepare (44100.0); e3.setPatch (loud.v.data());
    for (int n : { 48, 52, 55, 60, 64, 67 }) e3.noteOn (n, 1.0f);
    std::vector<float> ol (44100), orr (44100); e3.process (ol.data(), orr.data(), 44100);
    float pk = 0; for (float x : ol) pk = std::max (pk, std::fabs (x));
    CHECK (pk <= 1.0f && pk > 0.5f, "output limiter holds a six-note chord at +18 dB under 1.0");
}

static void testFxAndPresets()
{
    std::printf ("effects and presets\n");
    Patch p = Patch::init(); p.v[P_LEVEL] = 0; p.v[P_RV_MIX] = 0.5f; p.v[P_DL_MIX] = 0.4f; p.v[P_CH_MIX] = 0.5f; p.v[P_A] = 0.001f; p.v[P_D] = 0.05f; p.v[P_S] = 0.0f;
    auto eOwner = std::make_unique<Engine>(); Engine& e = *eOwner; e.prepare (48000.0); e.setPatch (p.v.data());
    e.noteOn (60, 1.0f);
    std::vector<float> L (48000 * 3), R (48000 * 3);
    e.process (L.data(), R.data(), (int) L.size());
    double tail = 0; bool fin = true, stereo = false;
    for (size_t i = 48000 * 2; i < L.size(); ++i) { tail += std::fabs (L[i]); fin = fin && std::isfinite (L[i]) && std::isfinite (R[i]); if (std::fabs (L[i] - R[i]) > 1e-4f) stereo = true; }
    CHECK (fin, "effects stay finite");
    CHECK (tail > 0.01, "reverb and delay leave a tail after the note ends");
    CHECK (stereo, "effects widen the mono voice into stereo");

    // preset file round trip
    Patch q = factoryBank()[12]; q.name = "My Test"; q.category = "User";
    Patch r; const bool ok = patchFromText (patchToText (q), r);
    bool same = ok && r.name == "My Test";
    for (int i = 0; i < P_COUNT && same; ++i) same = std::fabs (r.v[(size_t) i] - q.v[(size_t) i]) < 1e-3f * std::max (1.0f, std::fabs (q.v[(size_t) i]));
    CHECK (same, "preset text round trip is lossless");
    Patch bad; CHECK (! patchFromText ("garbage", bad), "garbage is rejected as a preset");
    CHECK (patchFromText ("formwave-preset 1\nname=X\nlevel=999\nnonsense=5\n", r) && r.v[P_LEVEL] == 18.0f, "out-of-range values clamp, unknown keys are ignored");
}

static void renderDemo (const char* path)
{
    const auto& bank = factoryBank();
    auto idx = [&] (const char* n) { for (size_t i = 0; i < bank.size(); ++i) if (bank[i].name == n) return (int) i; return 0; };
    const double sr = 44100.0, step = 60.0 / 90.0 / 4.0 * sr;
    const int total = (int) (step * 32);
    std::vector<float> L ((size_t) total, 0.0f), R ((size_t) total, 0.0f);
    // each instrument is its own engine so every part keeps its own patch
    struct Part { const char* name; Engine e; };
    auto mk = [&] (const char* n) { auto p = std::make_unique<Part>(); p->name = n; p->e.prepare (sr); p->e.setPatch (bank[(size_t) idx (n)].v.data()); return p; };
    auto kick = mk ("Boom Kick"), snare = mk ("Boom-bap Snare"), hat = mk ("Closed Hat"), bass = mk ("Sub Sine Bass"), keys = mk ("Rhodes-ish EP");
    Patch ep = bank[(size_t) idx ("Rhodes-ish EP")]; ep.v[P_CH_MIX] = 0.6f; ep.v[P_RV_MIX] = 0.25f; keys->e.setPatch (ep.v.data());
    int pos = 0;
    for (int s = 0; s < 32; ++s)
    {
        const int at = (int) (s * step), n = (int) ((s + 1) * step) - at, st = s % 16;
        if (st == 0 || st == 6 || st == 9) kick->e.noteOn (60, 1.0f);
        if (st == 4 || st == 12) snare->e.noteOn (60, 1.0f);
        if (st % 2 == 0) hat->e.noteOn (60, st % 4 ? 0.5f : 0.8f);
        if (st == 0) bass->e.noteOn (60, 1.0f);
        if (st == 6) { bass->e.noteOff (60); bass->e.noteOn (63, 1.0f); }
        if (st == 12) { bass->e.noteOff (63); bass->e.noteOn (58, 1.0f); }
        if (st == 0) keys->e.noteOn (67, 0.6f);
        if (st == 8) { keys->e.noteOff (67); keys->e.noteOn (70, 0.6f); }
        std::vector<float> tl ((size_t) n), tr ((size_t) n);
        for (auto* part : { kick.get(), snare.get(), hat.get(), bass.get(), keys.get() })
        {
            part->e.process (tl.data(), tr.data(), n);
            for (int i = 0; i < n; ++i) { L[(size_t) (at + i)] += tl[(size_t) i]; R[(size_t) (at + i)] += tr[(size_t) i]; }
        }
        pos = at + n;
    }
    float pk = 0; for (int i = 0; i < pos; ++i) pk = std::max (pk, std::fabs (L[(size_t) i]));
    for (auto& v : L) v *= 0.8f / std::max (pk, 1e-6f);
    for (auto& v : R) v *= 0.8f / std::max (pk, 1e-6f);
    writeWav (path, L, R, (int) sr);
    CHECK (pk > 0.1f, "demo beat renders audibly");
}

int main (int argc, char** argv)
{
    testParams(); testBank(); testSound(); testFxAndPresets();
    std::printf ("demo\n"); renderDemo (argc > 1 ? argv[1] : "demo.wav");
    std::printf ("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
