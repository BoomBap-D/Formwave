#pragma once
// Factory preset bank, preset (de)serialisation and offline rendering.
#include "Engine.h"
#include "FactoryData.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>

namespace fw
{
inline float clampParam (int idx, float v)
{
    const auto& d = paramTable()[(size_t) idx];
    return std::min (d.max, std::max (d.min, v));
}

// Render one note of a patch (no effects) to a mono buffer.
inline std::vector<float> renderPatch (const Patch& p, int note, float vel, float hold, double sr = 44100.0)
{
    const bool drum = p.v[P_MODE] < 0.5f;
    const double dur = std::min (4.0, drum ? (double) p.v[P_LEN] + 0.1 : hold + p.v[P_R] * 1.6 + 0.15);
    std::vector<float> out ((size_t) (dur * sr));
    auto v = std::make_unique<Voice>();
    v->start (p.v.data(), sr, note, vel, 0.0f, 1, 7u);
    const size_t holdN = (size_t) (hold * sr);
    for (size_t i = 0; i < out.size(); ++i)
    {
        if (! drum && i == holdN) v->release();
        float s = 0.0f;
        if (! v->process (s)) s = 0.0f;
        out[i] = s;
    }
    return out;
}

inline std::string categoryLabel (const std::string& c)
{
    if (c == "Hats") return "Hats & Cymbals";
    if (c == "Perc") return "Percussion";
    return c;
}

inline Patch fromAuthoring (const AuthoringPreset& a)
{
    Patch p = Patch::init();
    p.name = a.name; p.category = categoryLabel (a.cat);
    auto set = [&] (int idx, float v) { p.v[(size_t) idx] = clampParam (idx, v); };
    set (P_MODE, a.drum ? 0.0f : 1.0f);
    set (P_BASEHZ, a.hz);
    set (P_OCT, (float) a.oct);

    struct Slot { int wave; float ratio, cents, level, dec; };
    std::vector<Slot> slots;
    int fixedSq = 0;
    for (const auto& s : a.o) if (s.hz >= 0.0f && s.w == AWave::Square && s.dec == 0.0f) ++fixedSq;
    bool metalDone = false;
    float metalLevel = 0.0f, metalScale = 1.0f;
    for (const auto& s : a.o)
    {
        if (fixedSq >= 6 && s.hz >= 0.0f && s.w == AWave::Square && s.dec == 0.0f)
        {
            if (! metalDone) { metalScale = s.hz / 263.0f * 200.0f / a.hz; metalDone = true; }
            metalLevel += s.l;
            continue;
        }
        Slot sl { W_SINE, 1.0f, s.c, s.l, s.dec };
        sl.wave = s.w == AWave::Sine ? W_SINE : s.w == AWave::Tri ? W_TRI : s.w == AWave::Saw ? W_SAW : W_SQUARE;
        sl.ratio = s.hz >= 0.0f ? s.hz / a.hz : s.r;
        slots.push_back (sl);
    }
    if (metalDone) slots.push_back ({ W_METAL, metalScale, 0.0f, metalLevel, 0.0f });

    bool additive = false;
    float harm[32] {};
    if (slots.size() > 4)
    {
        bool allSine = true;
        for (const auto& s : slots) if (s.wave != W_SINE || s.dec != 0.0f || s.cents != 0.0f) allSine = false;
        if (allSine)
        {
            float minR = 1e9f; for (const auto& s : slots) minR = std::min (minR, s.ratio);
            const float k = minR < 1.0f ? 1.0f / minR : 1.0f;
            for (const auto& s : slots) { const int h = (int) std::lround (s.ratio * k); if (h >= 1 && h <= 32) harm[h - 1] += s.level; }
            set (P_OCT, (float) a.oct - (k > 1.0f ? std::log2 (k) : 0.0f));
            slots.assign (1, { W_ADD, 1.0f, 0.0f, 1.0f, 0.0f });
            additive = true;
        }
        else
            while (slots.size() > 4)
            {
                size_t m = 0; for (size_t i = 1; i < slots.size(); ++i) if (slots[i].level < slots[m].level) m = i;
                slots.erase (slots.begin() + (long) m);
            }
    }
    float mx = 0.0f; for (const auto& s : slots) mx = std::max (mx, s.level);
    for (int i = 0; i < 4; ++i) set (oscIdx (i, O_LEVEL), 0.0f);
    for (size_t i = 0; i < slots.size() && i < 4; ++i)
    {
        set (oscIdx ((int) i, O_WAVE), (float) slots[i].wave);
        set (oscIdx ((int) i, O_RATIO), slots[i].ratio);
        set (oscIdx ((int) i, O_CENTS), slots[i].cents);
        set (oscIdx ((int) i, O_LEVEL), mx > 0.0f ? slots[i].level / mx : 1.0f);
        set (oscIdx ((int) i, O_DECAY), slots[i].dec);
    }
    if (additive)
    {
        float hm = 0; for (float h : harm) hm = std::max (hm, std::fabs (h));
        for (int k = 0; k < 32; ++k) set (P_HARM0 + k, hm > 0 ? harm[k] / hm : 0.0f);
    }
    if (a.pe) { set (P_PE_ST, a.pe0); set (P_PE_T, a.pe1); }
    if (a.fm) { set (P_FM_R, a.fmR); set (P_FM_I, a.fmI); set (P_FM_D, a.fmDec); }
    if (a.n.on)
    {
        set (P_N_LVL, a.n.l);
        set (P_N_KIND, a.n.kind == ANoise::White ? NK_WHITE : a.n.kind == ANoise::Pink ? NK_PINK : NK_CRACKLE);
        set (P_N_FT, a.n.filt == AFilt::Lp ? FT_LP : a.n.filt == AFilt::Hp ? FT_HP : FT_BP);
        set (P_N_HZ, a.n.hz); set (P_N_Q, a.n.q); set (P_N_ATT, a.n.att);
        set (P_N_DEC, a.n.dec >= 6.0f ? 0.0f : a.n.dec);
        set (P_N_BURSTS, (float) a.n.bursts); set (P_N_GAP, a.n.gap);
        if (a.n.sweep) { set (P_N_SWEEP, a.n.s1); set (P_N_SWEEPT, a.n.st); p.v[P_N_HZ] = clampParam (P_N_HZ, a.n.s0); }
    }
    if (a.f.on)
    {
        set (P_F_ON, 1.0f);
        set (P_F_TYPE, a.f.type == AFilt::Lp ? FT_LP : a.f.type == AFilt::Hp ? FT_HP : FT_BP);
        set (P_F_HZ, a.f.hz); set (P_F_Q, a.f.q); set (P_F_ENV, a.f.env); set (P_F_ENVD, a.f.dec);
        if (a.f.lfo) { set (P_F_LFO, a.f.lfoHz); set (P_F_LFOD, a.f.lfoCents); }
    }
    set (P_DRIVE, a.drive);
    set (P_A, a.a[0]); set (P_D, a.a[1]); set (P_S, a.a[2]); set (P_R, a.a[3]);
    set (P_LEN, a.len); set (P_RISE, a.rise ? 1.0f : 0.0f);
    if (a.vib) { set (P_VIB_HZ, a.vibHz); set (P_VIB_DEP, a.vibDep); set (P_VIB_DLY, a.vibDelay); }
    set (P_GLIDE, a.glide); set (P_MONO, a.mono ? 1.0f : 0.0f);
    return p;
}

// All factory presets, converted and level-calibrated once per process.
inline const std::vector<Patch>& factoryBank()
{
    static std::vector<Patch> bank;
    static std::once_flag once;
    std::call_once (once, []
    {
        for (const auto& a : authoringPresets())
        {
            Patch p = fromAuthoring (a);
            p.v[P_LEVEL] = 0.0f;
            const auto b = renderPatch (p, 60, 1.0f, 0.6f);
            float pk = 0.0f;
            for (float x : b) pk = std::max (pk, std::fabs (x));
            p.v[P_LEVEL] = pk > 1e-6f ? clampParam (P_LEVEL, 20.0f * std::log10 (0.6f / pk)) : 0.0f;
            bank.push_back (p);
        }
    });
    return bank;
}

// ---- preset files: plain text, "key=value" per line. Only non-default values are written. ----
inline std::string patchToText (const Patch& p)
{
    std::ostringstream o;
    o << "formwave-preset 1\nname=" << p.name << "\ncategory=" << p.category << "\n";
    const auto& t = paramTable();
    const Patch d = Patch::init();
    for (size_t i = 0; i < t.size(); ++i)
        if (std::fabs (p.v[i] - d.v[i]) > 1e-6f) o << t[i].id << "=" << p.v[i] << "\n";
    return o.str();
}

inline bool patchFromText (const std::string& text, Patch& out)
{
    std::istringstream in (text);
    std::string line;
    if (! std::getline (in, line) || line.rfind ("formwave-preset", 0) != 0) return false;
    Patch p = Patch::init();
    p.name = "Untitled"; p.category = "User";
    while (std::getline (in, line))
    {
        if (! line.empty() && line.back() == '\r') line.pop_back();
        const auto eq = line.find ('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr (0, eq), val = line.substr (eq + 1);
        if (k == "name") p.name = val;
        else if (k == "category") p.category = val;
        else
        {
            const int idx = paramIndex (k);
            if (idx >= 0) { try { p.v[(size_t) idx] = clampParam (idx, std::stof (val)); } catch (...) {} }
        }
    }
    out = p;
    return true;
}
} // namespace fw
