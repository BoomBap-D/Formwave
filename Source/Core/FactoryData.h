#pragma once
// Authoring data for the 67 factory presets, written with a small builder. Bank.h converts each one
// into a full parameter Patch, so every factory sound is an editable starting point.
#include <string>
#include <vector>

namespace fw
{
enum class AWave { Sine, Square, Saw, Tri };
enum class ANoise { White, Pink, Crackle };
enum class AFilt { Lp, Hp, Bp };

struct AOsc
{
    AWave w = AWave::Sine;
    float r = 1.0f;      // frequency ratio to the note
    float c = 0.0f;      // detune, cents
    float l = 1.0f;      // level
    float hz = -1.0f;    // fixed frequency (inharmonic partial) when >= 0
    float dec = 0.0f;    // own decay time (s) when > 0
};

struct ANoiseSpec
{
    bool on = false;
    ANoise kind = ANoise::White;
    AFilt filt = AFilt::Hp;
    float hz = 1000.0f, q = 0.7f, l = 1.0f, att = 0.001f, dec = 0.1f;
    int bursts = 0;
    float gap = 0.01f;
    bool sweep = false;
    float s0 = 0.0f, s1 = 0.0f, st = 1.0f;
};

struct AFilterSpec
{
    bool on = false;
    AFilt type = AFilt::Lp;
    float hz = 1000.0f, q = 0.7f, env = 0.0f, dec = 0.2f;
    bool lfo = false;
    float lfoHz = 1.0f, lfoCents = 0.0f;
};

struct AuthoringPreset
{
    std::string cat, name;
    bool drum = true;
    float hz = 200.0f;        // base frequency at note 60 (drums)
    int oct = 0;              // octave shift (notes)
    std::vector<AOsc> o;
    bool pe = false;  float pe0 = 0.0f, pe1 = 0.05f;      // pitch envelope: semitones, seconds
    bool fm = false;  float fmR = 1.0f, fmI = 1.0f, fmDec = 0.3f;
    ANoiseSpec n;
    AFilterSpec f;
    float drive = 0.0f;
    float a[4] { 0.005f, 0.2f, 0.8f, 0.1f };               // notes: A D S R
    float len = 0.3f;                                      // drums: total decay time
    bool rise = false;
    bool vib = false;  float vibHz = 5.0f, vibDep = 10.0f, vibDelay = 0.2f;
    float glide = 0.0f;
    bool mono = false;
    float trim = 1.0f;        // filled by SynthEngine::calibrate

    // ---- tiny builder ----
    AuthoringPreset& D (float baseHz, float length) { drum = true; hz = baseHz; len = length; return *this; }
    AuthoringPreset& N (int octave, float A, float Dc, float S, float R) { drum = false; oct = octave; a[0] = A; a[1] = Dc; a[2] = S; a[3] = R; return *this; }
    AuthoringPreset& osc (AWave w, float l = 1.0f, float r = 1.0f, float c = 0.0f, float dec = 0.0f) { AOsc s; s.w = w; s.l = l; s.r = r; s.c = c; s.dec = dec; o.push_back (s); return *this; }
    AuthoringPreset& fixedOsc (AWave w, float freq, float l = 1.0f, float dec = 0.0f) { AOsc s; s.w = w; s.hz = freq; s.l = l; s.dec = dec; o.push_back (s); return *this; }
    AuthoringPreset& sq6 (float base = 1.0f, float l = 1.0f)
    {
        for (float h : { 263.0f, 400.0f, 421.0f, 474.0f, 587.0f, 845.0f })
            fixedOsc (AWave::Square, h * base, l);
        return *this;
    }
    AuthoringPreset& pitchEnv (float semis, float t) { pe = true; pe0 = semis; pe1 = t; return *this; }
    AuthoringPreset& fmod (float r, float i, float dec) { fm = true; fmR = r; fmI = i; fmDec = dec; return *this; }
    AuthoringPreset& noise (ANoise k, AFilt ft, float freq, float q_, float level, float decay, float attack = 0.001f)
    { n.on = true; n.kind = k; n.filt = ft; n.hz = freq; n.q = q_; n.l = level; n.dec = decay; n.att = attack; return *this; }
    AuthoringPreset& bursts (int count, float g) { n.bursts = count; n.gap = g; return *this; }
    AuthoringPreset& sweepNoise (float a_, float b_, float t) { n.sweep = true; n.s0 = a_; n.s1 = b_; n.st = t; return *this; }
    AuthoringPreset& filter (AFilt t, float freq, float q_ = 0.7f, float envOct = 0.0f, float dec = 0.2f)
    { f.on = true; f.type = t; f.hz = freq; f.q = q_; f.env = envOct; f.dec = dec; return *this; }
    AuthoringPreset& filterLfo (float hzRate, float cents) { f.lfo = true; f.lfoHz = hzRate; f.lfoCents = cents; return *this; }
    AuthoringPreset& drv (float d) { drive = d; return *this; }
    AuthoringPreset& vibrato (float rate, float depthCents, float delay) { vib = true; vibHz = rate; vibDep = depthCents; vibDelay = delay; return *this; }
    AuthoringPreset& slide (float t) { glide = t; mono = true; return *this; }
    AuthoringPreset& rising() { rise = true; return *this; }
};

inline const std::vector<AuthoringPreset>& authoringPresets()
{
    static const std::vector<AuthoringPreset> list = []
    {
        std::vector<AuthoringPreset> v;
        auto add = [&] (const char* cat, const char* name) -> AuthoringPreset&
        {
            v.emplace_back();
            v.back().cat = cat;
            v.back().name = name;
            return v.back();
        };
        using W = AWave; using K = ANoise; using F = AFilt;
        auto click = [] (AuthoringPreset& p, float hz, float l, float dec) { p.noise (K::White, F::Hp, hz, 0.7f, l, dec); };

        // ---- Kicks ----
        click (add ("Kicks", "Boom Kick").D (52, .55f).osc (W::Sine).pitchEnv (26, .05f).drv (1.6f), 3500, .12f, .01f);
        add ("Kicks", "Dusty Kick").D (58, .38f).osc (W::Sine).osc (W::Tri, .35f).pitchEnv (14, .04f).filter (F::Lp, 700).drv (1.2f).noise (K::Pink, F::Lp, 900, .7f, .05f, .3f);
        add ("Kicks", "Trap Thump").D (46, .8f).osc (W::Sine).pitchEnv (32, .03f).drv (2.2f);
        click (add ("Kicks", "Drill Punch").D (56, .32f).osc (W::Sine).pitchEnv (38, .022f).drv (2.8f), 2500, .2f, .006f);
        add ("Kicks", "Sub Boom").D (40, 1.3f).osc (W::Sine).pitchEnv (14, .09f).drv (.4f);
        click (add ("Kicks", "Tight Kick").D (72, .2f).osc (W::Sine).pitchEnv (22, .018f).drv (1.4f), 4000, .2f, .006f);
        add ("Kicks", "Crushed Kick").D (60, .42f).osc (W::Tri).pitchEnv (24, .04f).drv (5);
        add ("Kicks", "Cassette Kick").D (55, .5f).osc (W::Sine).osc (W::Tri, .4f).pitchEnv (10, .06f).filter (F::Lp, 900).drv (1).noise (K::Pink, F::Lp, 6000, .7f, .04f, .5f);
        // ---- 808s ----
        add ("808s", "808 Classic").D (65.41f, 1.6f).osc (W::Sine).pitchEnv (14, .03f).drv (1.3f);
        add ("808s", "808 Long Glide").D (65.41f, 2.2f).osc (W::Sine).pitchEnv (26, .14f).drv (1);
        add ("808s", "808 Distorted").D (65.41f, 1.5f).osc (W::Sine).osc (W::Tri, .3f).pitchEnv (12, .03f).drv (5);
        add ("808s", "808 Boomy Sub").D (65.41f, 2.6f).osc (W::Sine).pitchEnv (5, .06f).drv (.35f);
        add ("808s", "808 Rising Slide").D (65.41f, 1.6f).osc (W::Sine).pitchEnv (-9, .25f).drv (1.4f);
        add ("808s", "808 West Coast Saw").D (65.41f, 1.4f).osc (W::Sine).osc (W::Saw, .5f).pitchEnv (7, .04f).filter (F::Lp, 320, 1).drv (1);
        add ("808s", "808 Detuned").D (65.41f, 1.6f).osc (W::Sine, 1, 1, -9).osc (W::Sine, 1, 1, 9).pitchEnv (12, .03f).drv (2);
        add ("808s", "808 Short Stab").D (65.41f, .55f).osc (W::Sine).pitchEnv (12, .02f).drv (1.6f);
        // ---- Snares ----
        add ("Snares", "Boom-bap Snare").D (185, .26f).osc (W::Tri, 1, 1, 0, .1f).osc (W::Sine, .4f, 1.8f, 0, .07f).pitchEnv (5, .02f).noise (K::White, F::Bp, 3200, .7f, 1.1f, .16f);
        add ("Snares", "Rim Crack").D (480, .09f).osc (W::Sine, 1, 1, 0, .04f).osc (W::Square, .4f, 1.5f, 0, .03f).noise (K::White, F::Hp, 2500, .7f, .8f, .04f);
        add ("Snares", "Trap Snare").D (200, .3f).osc (W::Tri, 1, 1, 0, .1f).pitchEnv (10, .03f).noise (K::White, F::Hp, 1800, .5f, 1.1f, .22f).drv (1.6f);
        add ("Snares", "Drill Snare").D (250, .24f).osc (W::Sine, 1, 1, 0, .06f).noise (K::White, F::Bp, 4800, .5f, 1.2f, .18f).drv (3);
        add ("Snares", "Dusty Snare").D (170, .32f).osc (W::Tri, 1, 1, 0, .1f).noise (K::White, F::Bp, 2200, .6f, .9f, .24f).drv (.6f);
        add ("Snares", "Big Room Snare").D (190, .55f).osc (W::Tri, 1, 1, 0, .15f).noise (K::White, F::Bp, 2800, .6f, 1.1f, .45f).drv (1);
        // ---- Claps ----
        add ("Claps", "909 Clap").D (200, .3f).noise (K::White, F::Bp, 1300, 1, 1, .2f).bursts (4, .011f);
        add ("Claps", "Dry Clap").D (200, .14f).noise (K::White, F::Bp, 1700, 1.1f, 1, .08f).bursts (3, .009f);
        add ("Claps", "Layered Clap").D (200, .32f).fixedOsc (W::Tri, 200, 1, .05f).noise (K::White, F::Bp, 1500, .9f, 1.2f, .22f).bursts (4, .012f).drv (.8f);
        // ---- Hats & cymbals ----
        add ("Hats", "Closed Hat").D (200, .07f).sq6().filter (F::Hp, 7000, .9f);
        add ("Hats", "Open Hat").D (200, .55f).sq6().filter (F::Hp, 7000, .9f);
        add ("Hats", "Tick Hat").D (200, .03f).sq6 (1.2f).filter (F::Hp, 9000, .9f);
        add ("Hats", "Trap Hat").D (200, .05f).sq6().filter (F::Hp, 8000, .9f).noise (K::White, F::Hp, 9000, .7f, .4f, .04f);
        add ("Hats", "Shaker").D (200, .12f).noise (K::White, F::Bp, 6500, .8f, 1, .07f, .025f);
        add ("Hats", "Crash").D (200, 1.8f).sq6().filter (F::Hp, 5000, .7f).noise (K::White, F::Hp, 3500, .7f, 1.2f, 1.5f);
        add ("Hats", "Ride").D (200, 1.3f).sq6 (1.5f).fixedOsc (W::Sine, 3000, 2, .8f).filter (F::Bp, 8000, .5f);
        // ---- Percussion ----
        click (add ("Perc", "Low Tom").D (110, .55f).osc (W::Sine).pitchEnv (10, .08f).drv (.5f), 3000, .08f, .01f);
        click (add ("Perc", "Mid Tom").D (170, .42f).osc (W::Sine).pitchEnv (9, .06f).drv (.5f), 3000, .08f, .01f);
        add ("Perc", "Cowbell 808").D (200, .32f).fixedOsc (W::Square, 540).fixedOsc (W::Square, 800).filter (F::Bp, 2640, .8f);
        add ("Perc", "Conga").D (330, .24f).osc (W::Sine, 1, 1, 0, .18f).osc (W::Tri, .3f, 2, 0, .07f).pitchEnv (4, .03f);
        add ("Perc", "Clave").D (2400, .09f).osc (W::Sine, 1, 1, 0, .06f).osc (W::Sine, .3f, 1.5f, 0, .04f);
        add ("Perc", "Tambourine").D (200, .2f).sq6 (1, .25f).filter (F::Hp, 8000, .7f).noise (K::White, F::Hp, 6000, .7f, 1.4f, .12f).bursts (3, .02f);
        // ---- Bass ----
        add ("Bass", "Sub Sine Bass").N (-2, .005f, .1f, .9f, .12f).osc (W::Sine);
        add ("Bass", "Moog Saw Bass").N (-2, .005f, .25f, .7f, .1f).osc (W::Saw).osc (W::Square, .6f, 1, -5).filter (F::Lp, 450, 4, 2.5f, .25f);
        add ("Bass", "Reese").N (-2, .03f, .3f, .9f, .3f).osc (W::Saw, 1, 1, -14).osc (W::Saw, 1, 1, 14).osc (W::Saw, .6f, .5f).filter (F::Lp, 600, 2).filterLfo (.4f, 600).drv (.6f);
        add ("Bass", "FM Bass").N (-2, .003f, .2f, .6f, .1f).osc (W::Sine).fmod (1, 4, .25f);
        add ("Bass", "Funk Pluck Bass").N (-2, .002f, .18f, .1f, .08f).osc (W::Saw).filter (F::Lp, 380, 3, 3.5f, .12f);
        add ("Bass", "Wobble Bass").N (-2, .01f, .1f, 1, .15f).osc (W::Saw).osc (W::Saw, 1, 1, 8).filter (F::Lp, 400, 6).filterLfo (2.2f, 2400).drv (1);
        // ---- Keys ----
        add ("Keys", "Rhodes-ish EP").N (-1, .002f, .9f, .25f, .5f).osc (W::Sine).osc (W::Sine, .3f, 2).fmod (14, .15f, .08f).drv (.5f);
        add ("Keys", "Wurly Keys").N (-1, .003f, .6f, .35f, .25f).osc (W::Tri).osc (W::Sine, .3f, 2, 0, .3f).drv (2);
        {
            auto& p = add ("Keys", "Organ").N (-1, .01f, .01f, 1, .04f);
            const float r[] { .5f, 1, 2, 3, 4, 6, 8 }, l[] { .8f, 1, .7f, .5f, .4f, .25f, .2f };
            for (int i = 0; i < 7; ++i) p.osc (W::Sine, l[i], r[i]);
        }
        {
            auto& p = add ("Keys", "Piano-ish").N (-1, .002f, 2, 0, .25f);
            const float r[] { 1, 2, 3, 4, 5 }, l[] { 1, .5f, .35f, .2f, .1f }, d[] { 2, 1.1f, .7f, .4f, .25f };
            for (int i = 0; i < 5; ++i) p.osc (W::Sine, l[i], r[i], 0, d[i]);
        }
        add ("Keys", "Clav").N (0, .001f, .25f, 0, .04f).osc (W::Saw).filter (F::Bp, 1800, 2.5f, 1.5f, .08f);
        add ("Keys", "Vibes").N (0, .002f, 2.5f, 0, .5f).osc (W::Sine).osc (W::Sine, .35f, 4, 0, .25f).vibrato (5.5f, 15, .05f);
        {
            auto& p = add ("Keys", "Jazz Stab").N (-1, .004f, .35f, 0, .12f).filter (F::Lp, 1600, 1, 1, .25f).drv (.4f);
            const float r[] { 1, 1.1892f, 1.4983f, 1.7818f };
            for (int i = 0; i < 4; ++i) p.osc (W::Saw, i == 3 ? .7f : 1.0f, r[i]);
        }
        // ---- Leads ----
        add ("Leads", "Saw Lead").N (0, .01f, .1f, .8f, .18f).osc (W::Saw, 1, 1, -7).osc (W::Saw, 1, 1, 7).filter (F::Lp, 3500, 1).vibrato (5, 12, .2f);
        add ("Leads", "G-Funk Whine").N (1, .03f, .05f, 1, .12f).osc (W::Sine).osc (W::Sine, .15f, 2).vibrato (5.2f, 18, .15f).slide (.12f);
        add ("Leads", "Pluck Lead").N (0, .002f, .3f, 0, .12f).osc (W::Saw).osc (W::Square, .5f, 1, 6).filter (F::Lp, 2500, 2, 3, .15f);
        add ("Leads", "Bell Lead").N (1, .001f, 2, 0, .6f).osc (W::Sine).fmod (3.5f, 3, 1.2f);
        add ("Leads", "Whistle Lead").N (1, .06f, .1f, .9f, .25f).osc (W::Sine).vibrato (6, 25, .25f);
        add ("Leads", "Chip Lead").N (0, .002f, .1f, .7f, .05f).osc (W::Square).vibrato (6, 10, .3f);
        // ---- Pads ----
        add ("Pads", "Warm Pad").N (-1, .7f, .5f, .85f, 1.4f).osc (W::Saw, 1, 1, -10).osc (W::Saw).osc (W::Saw, 1, 1, 10).osc (W::Tri, .6f, .5f).filter (F::Lp, 1100, .6f);
        add ("Pads", "Dark Strings").N (-1, .45f, .4f, .9f, 1.1f).osc (W::Saw, 1, 1, -12).osc (W::Saw, 1, 1, -4).osc (W::Saw, 1, 1, 4).osc (W::Saw, 1, 1, 12).filter (F::Lp, 800, .7f).filterLfo (.15f, 250).vibrato (5, 8, .4f);
        add ("Pads", "Choir Pad").N (0, .5f, .4f, .85f, 1.2f).osc (W::Tri).osc (W::Tri, .5f, 2, 5).osc (W::Sine, .3f, 3).filter (F::Lp, 1400, 3).noise (K::Pink, F::Bp, 1400, 1, .06f, 0);
        add ("Pads", "Glass Pad").N (0, .5f, .6f, .8f, 1.5f).osc (W::Sine).osc (W::Sine, .5f, 2).osc (W::Sine, .35f, 3).fmod (3.5f, 1.5f, 1.5f);
        add ("Pads", "Vinyl Haze").N (-1, .8f, .5f, .9f, 1.5f).osc (W::Sine, 1, .5f).osc (W::Sine, .6f).osc (W::Sine, .35f, 1.5f).filter (F::Lp, 700, .5f).noise (K::Crackle, F::Hp, 1500, .5f, .5f, 0);
        // ---- FX ----
        add ("FX", "Riser").D (200, 2.4f).rising().osc (W::Sine).pitchEnv (-36, 1.2f).noise (K::White, F::Bp, 300, 3, 1.4f, 50).sweepNoise (300, 7000, 2.2f);
        add ("FX", "Laser").D (900, .4f).osc (W::Square).pitchEnv (30, .18f).filter (F::Lp, 5000, 1).drv (1);
        add ("FX", "Vinyl Crackle").D (200, 2).noise (K::Crackle, F::Hp, 800, .5f, 1, 50);
        add ("FX", "Impact").D (38, 2.6f).osc (W::Sine).pitchEnv (12, .2f).noise (K::Pink, F::Lp, 600, .7f, .8f, 1.2f).drv (1.5f);
        add ("FX", "Air Horn").N (0, .02f, .05f, .95f, .08f).osc (W::Saw).osc (W::Saw, .8f, 1, 12).osc (W::Saw, .6f, 1.2599f).filter (F::Lp, 3000, .8f).vibrato (7, 8, .1f).drv (1.2f);
        return v;
    }();
    return list;
}
} // namespace fw
