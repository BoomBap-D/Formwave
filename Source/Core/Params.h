#pragma once
// Formwave parameter table. This is the single source of truth: the DSP reads values by index,
// the plug-in builds its host parameters from this table, presets are arrays of these values.
#include <array>
#include <cmath>
#include <string>
#include <vector>

namespace fw
{
enum Pid : int
{
    P_MODE = 0,        // 0 = one-shot (drum), 1 = notes
    P_BASEHZ,          // one-shot base pitch at middle C
    P_OCT,             // note-mode octave shift
    P_OSC0,            // 4 oscillators x 5 fields
    P_HARM0 = P_OSC0 + 20,   // 32 additive harmonics
    P_PE_ST = P_HARM0 + 32, P_PE_T,
    P_FM_R, P_FM_I, P_FM_D,
    P_N_LVL, P_N_KIND, P_N_FT, P_N_HZ, P_N_Q, P_N_ATT, P_N_DEC, P_N_BURSTS, P_N_GAP, P_N_SWEEP, P_N_SWEEPT,
    P_F_ON, P_F_TYPE, P_F_HZ, P_F_Q, P_F_ENV, P_F_ENVD, P_F_KT, P_F_LFO, P_F_LFOD,
    P_DRIVE,
    P_A, P_D, P_S, P_R, P_LEN, P_RISE,
    P_VIB_HZ, P_VIB_DEP, P_VIB_DLY, P_GLIDE, P_MONO,
    P_CH_MIX, P_CH_RATE, P_CH_DEP,
    P_DL_TIME, P_DL_FB, P_DL_MIX,
    P_RV_SIZE, P_RV_DAMP, P_RV_MIX,
    P_LEVEL,           // output level in dB (factory presets store their calibrated level here)
    P_UNI_N, P_UNI_DET, P_UNI_WID,          // unison: voices (index = count - 1), detune in cents, stereo spread
    P_LFO0,                                 // 2 LFOs x 4 fields (see LfoField)
    P_VEL_AMP = P_LFO0 + 8, P_VEL_CUT,      // velocity to level, velocity to filter cutoff (octaves)
    P_MW_VIB, P_MW_CUT,                     // mod wheel to vibrato depth (cents), to filter cutoff (octaves)
    P_COUNT
};

enum OscField { O_WAVE = 0, O_RATIO, O_CENTS, O_LEVEL, O_DECAY };
enum LfoField { L_RATE = 0, L_SHAPE, L_DEP, L_DEST };
enum LfoShape { LS_SINE = 0, LS_TRI, LS_SAW, LS_SQUARE, LS_RANDOM };
enum LfoDest { LD_OFF = 0, LD_PITCH, LD_CUTOFF, LD_AMP, LD_FM, LD_PAN };
enum Wave { W_SINE = 0, W_TRI, W_SAW, W_SQUARE, W_ADD, W_METAL };
enum NoiseKind { NK_WHITE = 0, NK_PINK, NK_CRACKLE };
enum FiltType { FT_LP = 0, FT_HP, FT_BP };

constexpr int oscIdx (int osc, int field) { return P_OSC0 + osc * 5 + field; }
constexpr int lfoIdx (int lfo, int field) { return P_LFO0 + lfo * 4 + field; }

enum ParamType { PT_FLOAT = 0, PT_CHOICE, PT_BOOL };

struct ParamDef
{
    std::string id, name, group, unit;
    float min = 0, max = 1, def = 0, skew = 1.0f;   // skew < 1 puts more resolution at the low end
    ParamType type = PT_FLOAT;
    std::vector<std::string> choices;
};

inline const std::vector<ParamDef>& paramTable()
{
    static const std::vector<ParamDef> t = []
    {
        std::vector<ParamDef> v;
        auto F = [&] (const char* id, const char* name, const char* group, float mn, float mx, float df, const char* unit = "", float skew = 1.0f)
        { ParamDef d; d.id = id; d.name = name; d.group = group; d.min = mn; d.max = mx; d.def = df; d.unit = unit; d.skew = skew; v.push_back (d); };
        auto C = [&] (const char* id, const char* name, const char* group, std::vector<std::string> ch, int df)
        { ParamDef d; d.id = id; d.name = name; d.group = group; d.min = 0; d.max = (float) ch.size() - 1; d.def = (float) df; d.type = PT_CHOICE; d.choices = std::move (ch); v.push_back (d); };
        auto B = [&] (const char* id, const char* name, const char* group, bool df)
        { ParamDef d; d.id = id; d.name = name; d.group = group; d.min = 0; d.max = 1; d.def = df ? 1.0f : 0.0f; d.type = PT_BOOL; v.push_back (d); };

        C ("mode", "Mode", "Voice", { "One-shot", "Notes" }, 1);
        F ("base", "Base Pitch", "Voice", 20, 3000, 200, "Hz", 0.35f);
        F ("oct", "Octave", "Voice", -4, 3, 0, "");
        for (int i = 0; i < 4; ++i)
        {
            const std::string n = std::to_string (i + 1), p = "o" + n + "_", g = "Osc " + n;
            C ((p + "wave").c_str(), ("Osc " + n + " Wave").c_str(), g.c_str(), { "Sine", "Triangle", "Saw", "Square", "Additive", "Metal" }, 0);
            F ((p + "ratio").c_str(), ("Osc " + n + " Ratio").c_str(), g.c_str(), 0.25f, 16.0f, 1.0f, "x", 0.5f);
            F ((p + "cents").c_str(), ("Osc " + n + " Fine").c_str(), g.c_str(), -100, 100, 0, "ct");
            F ((p + "level").c_str(), ("Osc " + n + " Level").c_str(), g.c_str(), 0, 1, i == 0 ? 1.0f : 0.0f, "");
            F ((p + "decay").c_str(), ("Osc " + n + " Decay").c_str(), g.c_str(), 0, 4, 0, "s", 0.5f);
        }
        for (int k = 1; k <= 32; ++k)
            F (("h" + std::to_string (k)).c_str(), ("Harmonic " + std::to_string (k)).c_str(), "Harmonics", -1, 1, k == 1 ? 1.0f : 0.0f, "");
        F ("pe_st", "Pitch Env Amount", "Pitch", -48, 48, 0, "st");
        F ("pe_t", "Pitch Env Time", "Pitch", 0.005f, 1.0f, 0.05f, "s", 0.4f);
        F ("fm_r", "FM Ratio", "FM", 0.25f, 16, 1, "x", 0.5f);
        F ("fm_i", "FM Amount", "FM", 0, 12, 0, "");
        F ("fm_d", "FM Decay", "FM", 0.02f, 3, 0.3f, "s", 0.5f);
        F ("n_lvl", "Noise Level", "Noise", 0, 2, 0, "");
        C ("n_kind", "Noise Type", "Noise", { "White", "Pink", "Crackle" }, 0);
        C ("n_ft", "Noise Filter", "Noise", { "Low-pass", "High-pass", "Band-pass" }, 1);
        F ("n_hz", "Noise Cutoff", "Noise", 40, 16000, 1000, "Hz", 0.3f);
        F ("n_q", "Noise Resonance", "Noise", 0.1f, 12, 0.7f, "");
        F ("n_att", "Noise Attack", "Noise", 0.001f, 1, 0.001f, "s", 0.4f);
        F ("n_dec", "Noise Decay", "Noise", 0, 6, 0.1f, "s", 0.4f);
        F ("n_bursts", "Clap Bursts", "Noise", 0, 6, 0, "");
        F ("n_gap", "Burst Gap", "Noise", 0.004f, 0.04f, 0.01f, "s");
        F ("n_sweep", "Noise Sweep To", "Noise", 0, 16000, 0, "Hz", 0.3f);
        F ("n_sweept", "Noise Sweep Time", "Noise", 0.1f, 4, 1, "s", 0.5f);
        B ("f_on", "Filter On", "Filter", false);
        C ("f_type", "Filter Type", "Filter", { "Low-pass", "High-pass", "Band-pass" }, 0);
        F ("f_hz", "Cutoff", "Filter", 30, 18000, 2000, "Hz", 0.3f);
        F ("f_q", "Resonance", "Filter", 0, 20, 0.7f, "dB");
        F ("f_env", "Filter Env", "Filter", -4, 6, 0, "oct");
        F ("f_envd", "Filter Env Decay", "Filter", 0.02f, 3, 0.2f, "s", 0.4f);
        F ("f_kt", "Key Track", "Filter", 0, 1, 0, "");
        F ("f_lfo", "Filter LFO Rate", "Filter", 0.05f, 20, 1, "Hz", 0.4f);
        F ("f_lfod", "Filter LFO Depth", "Filter", 0, 4800, 0, "ct");
        F ("drive", "Drive", "Output", 0, 6, 0, "");
        F ("amp_a", "Attack", "Amp", 0.001f, 4, 0.005f, "s", 0.3f);
        F ("amp_d", "Decay", "Amp", 0.01f, 4, 0.2f, "s", 0.4f);
        F ("amp_s", "Sustain", "Amp", 0, 1, 0.8f, "");
        F ("amp_r", "Release", "Amp", 0.01f, 4, 0.1f, "s", 0.4f);
        F ("len", "Length", "Amp", 0.03f, 4, 0.3f, "s", 0.4f);
        B ("rise", "Rising Envelope", "Amp", false);
        F ("vib_hz", "Vibrato Rate", "Pitch", 0.5f, 12, 5, "Hz");
        F ("vib_dep", "Vibrato Depth", "Pitch", 0, 100, 0, "ct");
        F ("vib_dly", "Vibrato Delay", "Pitch", 0, 1, 0.2f, "s");
        F ("glide", "Glide", "Pitch", 0, 1, 0, "s", 0.5f);
        B ("mono", "Mono", "Voice", false);
        F ("ch_mix", "Chorus Mix", "Chorus", 0, 1, 0);
        F ("ch_rate", "Chorus Rate", "Chorus", 0.1f, 4, 0.6f, "Hz", 0.5f);
        F ("ch_dep", "Chorus Depth", "Chorus", 0, 1, 0.5f);
        F ("dl_time", "Delay Time", "Delay", 20, 1000, 300, "ms", 0.5f);
        F ("dl_fb", "Delay Feedback", "Delay", 0, 0.9f, 0.35f);
        F ("dl_mix", "Delay Mix", "Delay", 0, 1, 0);
        F ("rv_size", "Reverb Size", "Reverb", 0, 1, 0.5f);
        F ("rv_damp", "Reverb Damping", "Reverb", 0, 1, 0.5f);
        F ("rv_mix", "Reverb Mix", "Reverb", 0, 1, 0);
        F ("level", "Level", "Output", -36, 18, 0, "dB");
        C ("uni_n", "Unison Voices", "Unison", { "1", "2", "3", "4", "5", "6", "7" }, 0);
        F ("uni_det", "Unison Detune", "Unison", 0, 60, 15, "ct");
        F ("uni_wid", "Unison Spread", "Unison", 0, 1, 0.7f);
        for (int i = 0; i < 2; ++i)
        {
            const std::string n = std::to_string (i + 1), p = "lfo" + n + "_", g = "LFO " + n;
            F ((p + "rate").c_str(), ("LFO " + n + " Rate").c_str(), g.c_str(), 0.05f, 20, i == 0 ? 2.0f : 5.0f, "Hz", 0.4f);
            C ((p + "shape").c_str(), ("LFO " + n + " Shape").c_str(), g.c_str(), { "Sine", "Triangle", "Saw", "Square", "Random" }, 0);
            F ((p + "dep").c_str(), ("LFO " + n + " Depth").c_str(), g.c_str(), 0, 1, 0, "");
            C ((p + "dest").c_str(), ("LFO " + n + " Target").c_str(), g.c_str(), { "Off", "Pitch", "Cutoff", "Level", "FM Amount", "Pan" }, 0);
        }
        F ("vel_amp", "Velocity to Level", "Velocity", 0, 1, 1, "");
        F ("vel_cut", "Velocity to Cutoff", "Velocity", 0, 4, 0, "oct");
        F ("mw_vib", "Mod Wheel Vibrato", "Mod Wheel", 0, 100, 60, "ct");
        F ("mw_cut", "Mod Wheel Cutoff", "Mod Wheel", 0, 3, 0, "oct");
        return v;
    }();
    return t;
}

inline int paramIndex (const std::string& id)
{
    const auto& t = paramTable();
    for (size_t i = 0; i < t.size(); ++i) if (t[i].id == id) return (int) i;
    return -1;
}

// A patch = one value per parameter, plus a name and category.
struct Patch
{
    std::array<float, P_COUNT> v {};
    std::string name = "Init", category = "User";

    static Patch init()
    {
        Patch p;
        const auto& t = paramTable();
        for (size_t i = 0; i < t.size() && i < (size_t) P_COUNT; ++i) p.v[i] = t[i].def;
        return p;
    }
};
} // namespace fw
