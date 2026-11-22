#pragma once
// Formwave voice: reads its settings live from the engine's patch array, so turning a knob
// changes notes that are already sounding. No allocation on the audio thread.
#include "Params.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fw
{
constexpr double kPi = 3.14159265358979323846;
constexpr int kTable = 2048;

inline const float* sineTable()
{
    static const std::array<float, kTable> t = [] { std::array<float, kTable> a {}; for (int i = 0; i < kTable; ++i) a[(size_t) i] = (float) std::sin (2.0 * kPi * i / kTable); return a; }();
    return t.data();
}

// One cycle of the sum of sines, band-limited to maxH harmonics, normalised to peak 1.
inline void buildAdditive (float* out, const float* harm, int maxH)
{
    const float* s = sineTable();
    maxH = std::min (32, std::max (1, maxH));
    float pk = 1e-9f;
    for (int i = 0; i < kTable; ++i)
    {
        float sum = 0.0f;
        for (int k = 1; k <= maxH; ++k)
            if (harm[k - 1] != 0.0f) sum += harm[k - 1] * s[(k * i) & (kTable - 1)];
        out[i] = sum; pk = std::max (pk, std::fabs (sum));
    }
    for (int i = 0; i < kTable; ++i) out[i] /= pk;
    out[kTable] = out[0];
}

class Biquad
{
public:
    // Q for low/high-pass is in dB (as in Web Audio); band-pass uses linear Q.
    void set (int type, double freq, double q, double sr)
    {
        freq = std::min (std::max (freq, 20.0), sr * 0.45);
        const double w = 2.0 * kPi * freq / sr, cw = std::cos (w), sw = std::sin (w);
        double b0, b1, b2;
        double alpha;
        if (type == FT_BP) { alpha = sw / (2.0 * std::max (0.05, q)); b0 = alpha; b1 = 0; b2 = -alpha; }
        else
        {
            alpha = sw / (2.0 * std::pow (10.0, q / 20.0));
            if (type == FT_LP) { b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = b0; }
            else               { b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = b0; }
        }
        const double a0 = 1 + alpha;
        b0_ = b0 / a0; b1_ = b1 / a0; b2_ = b2 / a0; a1_ = -2 * cw / a0; a2_ = (1 - alpha) / a0;
    }
    float process (float x) { const double y = b0_ * x + z1_; z1_ = b1_ * x - a1_ * y + z2_; z2_ = b2_ * x - a2_ * y; return (float) y; }
    void reset() { z1_ = z2_ = 0; }
private:
    double b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0, z1_ = 0, z2_ = 0;
};

class Voice
{
public:
    bool active() const { return active_; }
    int note() const { return note_; }
    bool held() const { return held_; }
    uint64_t order() const { return order_; }
    float baseHz() const { return base_; }

    void start (const float* patch, double sr, int note, float vel, float prevHz, uint64_t order, uint32_t seed)
    {
        pv_ = patch; sr_ = sr; note_ = note; vel_ = vel; order_ = order; prevHz_ = prevHz;
        rng_ = seed * 2654435761u + 977u; if (rng_ == 0) rng_ = 1;
        n_ = 0; relT_ = -1.0; killT_ = -1.0; active_ = true; held_ = true;
        drum_ = pv_[P_MODE] < 0.5f;
        pf_ = drum_ ? std::pow (2.0, (note - 60) / 12.0) : 1.0;
        base_ = drum_ ? (float) (pv_[P_BASEHZ] * pf_) : (float) (440.0 * std::pow (2.0, (note - 69.0) / 12.0 + pv_[P_OCT]));
        for (int i = 0; i < 4; ++i) { phase_[i] = 0.0; env_[i] = 1.0f; }
        for (auto& m : mph_) m = 0.0;
        modPhase_ = lfoPhase_ = lfoFPhase_ = 0.0;
        oscF_.reset(); noiseF_.reset();
        for (auto& x : pink_) x = 0.0f;
        pop_ = 0.0f;
        bend_ = 0.0f; modWheel_ = 0.0f;
        uniCents_ = 0.0f; pan_ = 0.0f; uniGain_ = 1.0f; panMod_ = 0.0f; ampMod_ = 1.0f; fmMod_ = 1.0f; lfoPitch_ = 0.0;
        for (int k = 0; k < 2; ++k) { lfoPh_[k] = 0.0; lfoHold_[k] = 0.0f; }
        buildTables();
        control (0.0);
        noiseF_.set (nf_.type, nf_.hz, nf_.q, sr_);
    }

    void release() { held_ = false; if (active_ && ! drum_ && relT_ < 0.0) relT_ = (double) n_ / sr_; }
    void kill() { if (active_ && killT_ < 0.0) killT_ = (double) n_ / sr_; }
    void setBend (float semis) { bend_ = semis; }
    // unison: detune (cents), pan (-1..1) and gain for this copy of the note
    void setUnison (float cents, float pan, float gain) { uniCents_ = cents; pan_ = pan; uniGain_ = gain; }
    void randomisePhase() { if (drum_) return; for (int i = 0; i < 4; ++i) phase_[i] = 0.5 * (rnd() + 1.0); }
    float pan() const { return std::max (-1.0f, std::min (1.0f, pan_ + panMod_)); }
    void setModWheel (float x) { modWheel_ = x; }

    bool process (float& out)
    {
        if (! active_) return false;
        const float* v = pv_;
        const double t = (double) n_ / sr_;
        if ((n_ & 7) == 0) control (t);

        // ---- amplitude ----
        double amp;
        if (drum_)
        {
            const double L = std::max (0.03, (double) v[P_LEN]);
            if (v[P_RISE] > 0.5f) amp = t < L * 0.93 ? t / (L * 0.93) : std::max (0.0, 1.0 - (t - L * 0.93) / (L * 0.07));
            else if (t < 0.0015) amp = t / 0.0015;
            else if (t <= L) amp = std::pow (0.0005, (t - 0.0015) / std::max (0.01, L - 0.0015));
            else amp = 0.0005 * std::max (0.0, 1.0 - (t - L) / 0.04);
            if (t >= L + 0.06) { finish(); out = 0; return false; }
        }
        else
        {
            const double A = std::max (0.001, (double) v[P_A]), D = std::max (0.0005, (double) v[P_D] / 3.0), S = v[P_S];
            amp = t < A ? t / A : S + (1.0 - S) * std::exp (-(t - A) / D);
            if (relT_ >= 0.0)
            {
                const double R = std::max (0.0005, (double) v[P_R] / 3.0), rt = t - relT_;
                amp *= std::exp (-rt / R);
                if (rt > v[P_R] * 1.6 + 0.08) { finish(); out = 0; return false; }
            }
        }
        if (killT_ >= 0.0)
        {
            const double kt = t - killT_;
            amp *= std::exp (-kt / 0.008);
            if (kt > 0.06) { finish(); out = 0; return false; }
        }

        // ---- pitch modulation (one exp2 per sample, shared by all oscillators) ----
        double cents = (double) bend_ * 100.0 + (double) uniCents_ + lfoPitch_;
        if (v[P_PE_ST] != 0.0f) cents += (double) v[P_PE_ST] * 100.0 * std::exp (-t / std::max (0.0005, (double) v[P_PE_T] / 3.0));
        if (vibDepth_ > 0.0)
        {
            lfoPhase_ += v[P_VIB_HZ] / sr_; lfoPhase_ -= std::floor (lfoPhase_);
            cents += vibDepth_ * std::min (1.0, t / (v[P_VIB_DLY] + 0.3)) * std::sin (2.0 * kPi * lfoPhase_);
        }
        const double pm = cents != 0.0 ? std::exp2 (cents / 1200.0) : 1.0;
        double glideF = 1.0;
        const double gl = v[P_GLIDE];
        if (prevHz_ > 0.0f && gl > 0.0 && ! drum_) glideF = std::pow ((double) prevHz_ / base_, 1.0 - std::min (1.0, t / gl));

        double fmDev = 0.0;
        if (v[P_FM_I] > 0.0f)
        {
            const double r = v[P_FM_R];
            modPhase_ += (double) base_ * r * pm / sr_; modPhase_ -= std::floor (modPhase_);
            const double e = 0.04 + 0.96 * std::exp (-t / std::max (0.0005, (double) v[P_FM_D] / 3.0));
            fmDev = (double) v[P_FM_I] * fmMod_ * base_ * r * e * std::sin (2.0 * kPi * modPhase_);
        }

        // ---- oscillators ----
        float sum = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            if (lvl_[i] <= 0.0f) continue;
            const int w = (int) (v[oscIdx (i, O_WAVE)] + 0.5f);
            const double ratio = v[oscIdx (i, O_RATIO)];
            double f = (double) base_ * ratio * glideF * pm * detune_[i] + fmDev;
            f = std::max (0.0, std::min (f, sr_ * 0.49));
            const double dt = f / sr_;
            float s;
            if (w == W_METAL)
            {
                s = 0.0f;
                for (int j = 0; j < 6; ++j)
                {
                    const double dj = std::min (0.49, kMetal[j] * (double) base_ * ratio * glideF * pm * detune_[i] / 200.0 / sr_);
                    mph_[j] += dj; mph_[j] -= std::floor (mph_[j]);
                    s += sq (mph_[j], dj);
                }
                s *= (1.0f / 6.0f);
            }
            else
            {
                phase_[i] += dt; phase_[i] -= std::floor (phase_[i]);
                s = wave (w, phase_[i], dt, i);
            }
            float g = lvl_[i];
            if (decCoef_[i] > 0.0f) { env_[i] *= decCoef_[i]; g *= env_[i]; }
            sum += s * g;
        }
        if (fOn_) sum = oscF_.process (sum);
        if (nLvl_ > 0.0f) sum += noise (t);
        if (driveK_ > 1.0f) sum = std::tanh (driveK_ * sum) * driveNorm_;
        out = sum * (float) amp * outGain_ * uniGain_ * ampMod_;
        ++n_;
        return true;
    }

private:
    static constexpr double kMetal[6] { 263, 400, 421, 474, 587, 845 };

    void finish() { active_ = false; }

    static float polyBlep (double t, double dt)
    {
        if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }
    static float sq (double ph, double dt)
    {
        float v = ph < 0.5 ? 1.0f : -1.0f;
        v += polyBlep (ph, dt);
        double p2 = ph + 0.5; p2 -= std::floor (p2);
        v -= polyBlep (p2, dt);
        return v;
    }
    float wave (int w, double ph, double dt, int osc) const
    {
        switch (w)
        {
            case W_SINE:
            {
                const double x = ph * kTable; const int i = ((int) x) & (kTable - 1); const float fr = (float) (x - std::floor (x));
                const float* t = sineTable();
                return t[i] + (t[(i + 1) & (kTable - 1)] - t[i]) * fr;
            }
            case W_TRI:  return (float) (4.0 * std::fabs (ph - 0.5) - 1.0);
            case W_SAW:  return (float) (2.0 * ph - 1.0) - polyBlep (ph, dt);
            case W_SQUARE: return sq (ph, dt);
            case W_ADD:
            {
                const double x = ph * kTable; const int i = (int) x; const float fr = (float) (x - i);
                const float* t = table_[osc];
                return t[i] + (t[i + 1] - t[i]) * fr;
            }
        }
        return 0.0f;
    }
    float rnd() { rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5; return (float) (rng_ & 0xFFFFFF) / 8388608.0f - 1.0f; }

    float noise (double t)
    {
        const float* v = pv_;
        float x;
        const int kind = (int) (v[P_N_KIND] + 0.5f);
        if (kind == NK_PINK)
        {
            const float w = rnd();
            pink_[0] = .99886f * pink_[0] + w * .0555179f; pink_[1] = .99332f * pink_[1] + w * .0750759f;
            pink_[2] = .969f * pink_[2] + w * .153852f;    pink_[3] = .8665f * pink_[3] + w * .3104856f;
            pink_[4] = .55f * pink_[4] + w * .5329522f;    pink_[5] = -.7616f * pink_[5] - w * .016898f;
            x = (pink_[0] + pink_[1] + pink_[2] + pink_[3] + pink_[4] + pink_[5] + pink_[6] + w * .5362f) * .11f;
            pink_[6] = w * .115926f;
        }
        else if (kind == NK_CRACKLE)
        {
            x = rnd() * .02f + pop_; pop_ = 0.0f;
            if (rnd() * 0.5f + 0.5f < .0007f) { const float a = rnd() * (.3f + (rnd() * .5f + .5f) * .7f); x += a; pop_ = -a * .6f; }
        }
        else x = rnd();
        x = noiseF_.process (x);

        const double dec = v[P_N_DEC];
        double g = nLvl_;
        const bool shaped = drum_ ? v[P_RISE] <= 0.5f : dec > 0.0;
        if (shaped && dec > 0.0)
        {
            const int nb = (int) (v[P_N_BURSTS] + 0.5f);
            if (nb > 0)
            {
                g = 0.0;
                const double gap = v[P_N_GAP];
                for (int k = 0; k < nb; ++k)
                {
                    const double tk = k * gap;
                    if (t >= tk) g = nLvl_ * std::exp (-(t - tk) / (k < nb - 1 ? 0.004 : std::max (0.0005, dec / 3.0)));
                }
            }
            else
            {
                const double att = v[P_N_ATT];
                g = t < att ? nLvl_ * t / att : nLvl_ * std::exp (-(t - att) / std::max (0.0005, dec / 3.0));
            }
        }
        return x * (float) g;
    }

    struct NF { int type = FT_HP; double hz = 1000, q = 0.7; };

    void buildTables()
    {
        const float* v = pv_;
        for (int i = 0; i < 4; ++i)
        {
            if ((int) (v[oscIdx (i, O_WAVE)] + 0.5f) != W_ADD || v[oscIdx (i, O_LEVEL)] <= 0.0f) continue;
            const double f = (double) base_ * v[oscIdx (i, O_RATIO)] * std::pow (2.0, std::max (0.0f, v[P_PE_ST]) / 12.0 + 0.2);
            buildAdditive (table_[i], v + P_HARM0, (int) (sr_ * 0.45 / std::max (20.0, f)));
        }
    }

    // Two LFOs, each aimed at one destination. Runs at control rate (every 8 samples).
    void updateLfos()
    {
        const float* v = pv_;
        lfoPitch_ = 0.0; cutOct_ = 0.0; ampMod_ = 1.0f; fmMod_ = 1.0f; panMod_ = 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            const int dest = (int) (v[lfoIdx (k, L_DEST)] + 0.5f);
            const double dep = v[lfoIdx (k, L_DEP)];
            if (dest == LD_OFF || dep <= 0.0) continue;
            const double before = lfoPh_[k];
            lfoPh_[k] += (double) v[lfoIdx (k, L_RATE)] * 8.0 / sr_;
            lfoPh_[k] -= std::floor (lfoPh_[k]);
            double x;
            switch ((int) (v[lfoIdx (k, L_SHAPE)] + 0.5f))
            {
                case LS_TRI:    x = 1.0 - 4.0 * std::fabs (lfoPh_[k] - 0.5) ; x = -x; break;
                case LS_SAW:    x = 2.0 * lfoPh_[k] - 1.0; break;
                case LS_SQUARE: x = lfoPh_[k] < 0.5 ? 1.0 : -1.0; break;
                case LS_RANDOM: if (lfoPh_[k] < before || n_ == 0) lfoHold_[k] = rnd(); x = lfoHold_[k]; break;
                default:        x = std::sin (2.0 * kPi * lfoPh_[k]); break;
            }
            switch (dest)
            {
                case LD_PITCH:  lfoPitch_ += dep * x * 300.0; break;
                case LD_CUTOFF: cutOct_ += dep * x * 3.0; break;
                case LD_AMP:    ampMod_ *= (float) (1.0 - dep * (0.5 - 0.5 * x)); break;
                case LD_FM:     fmMod_ *= (float) std::max (0.0, 1.0 + dep * x); break;
                case LD_PAN:    panMod_ += (float) (dep * x); break;
                default: break;
            }
        }
    }

    void control (double t)
    {
        const float* v = pv_;
        float sumL = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            lvl_[i] = v[oscIdx (i, O_LEVEL)];
            sumL += lvl_[i];
            detune_[i] = std::exp2 ((double) v[oscIdx (i, O_CENTS)] / 1200.0);
            const double d = v[oscIdx (i, O_DECAY)];
            decCoef_[i] = d > 0.0 ? (float) std::exp (-1.0 / (std::max (0.0005, d / 3.0) * sr_)) : 0.0f;
        }
        if (sumL > 0.0f) for (auto& l : lvl_) l /= sumL;
        outGain_ = (float) std::pow (10.0, v[P_LEVEL] / 20.0) * ((1.0f - v[P_VEL_AMP]) + v[P_VEL_AMP] * vel_);
        const float dr = v[P_DRIVE];
        driveK_ = dr > 0.01f ? 1.0f + dr * 2.5f : 1.0f;
        driveNorm_ = 1.0f / std::tanh (driveK_);
        vibDepth_ = (double) v[P_VIB_DEP] + (double) modWheel_ * v[P_MW_VIB];
        updateLfos();
        nLvl_ = v[P_N_LVL];
        fOn_ = v[P_F_ON] > 0.5f;
        if (fOn_)
        {
            const double kt = std::pow ((double) base_ / 261.63, (double) v[P_F_KT]);
            const double cut = std::min (18000.0, std::max (30.0, (double) v[P_F_HZ] * kt));
            double f = cut;
            if (v[P_F_ENV] != 0.0f) f = cut + (cut * std::pow (2.0, v[P_F_ENV]) - cut) * std::exp (-t / std::max (0.0005, (double) v[P_F_ENVD] / 3.0));
            if (v[P_F_LFOD] > 0.0f)
            {
                lfoFPhase_ += v[P_F_LFO] * 8.0 / sr_; lfoFPhase_ -= std::floor (lfoFPhase_);
                f *= std::exp2 (v[P_F_LFOD] * std::sin (2.0 * kPi * lfoFPhase_) / 1200.0);
            }
            f *= std::exp2 ((double) v[P_VEL_CUT] * ((double) vel_ - 1.0) + (double) v[P_MW_CUT] * modWheel_ + cutOct_);
            oscF_.set ((int) (v[P_F_TYPE] + 0.5f), std::min (18000.0, std::max (30.0, f)), v[P_F_Q], sr_);
        }
        nf_.type = (int) (v[P_N_FT] + 0.5f); nf_.q = v[P_N_Q];
        nf_.hz = v[P_N_HZ];
        if (v[P_N_SWEEP] > 0.0f) nf_.hz = (double) v[P_N_HZ] * std::pow ((double) v[P_N_SWEEP] / std::max (1.0, (double) v[P_N_HZ]), std::min (1.0, t / v[P_N_SWEEPT]));
        if (n_ > 0) noiseF_.set (nf_.type, nf_.hz, nf_.q, sr_);
    }

    const float* pv_ = nullptr;
    bool active_ = false, drum_ = true, held_ = false, fOn_ = false;
    int note_ = 0;
    uint64_t order_ = 0, n_ = 0;
    uint32_t rng_ = 1;
    double sr_ = 44100, pf_ = 1, relT_ = -1, killT_ = -1, vibDepth_ = 0;
    double phase_[4] {}, mph_[6] {}, modPhase_ = 0, lfoPhase_ = 0, lfoFPhase_ = 0, detune_[4] { 1, 1, 1, 1 };
    float lvl_[4] {}, env_[4] {}, decCoef_[4] {};
    float base_ = 0, prevHz_ = 0, vel_ = 1, outGain_ = 1, driveK_ = 1, driveNorm_ = 1, nLvl_ = 0, bend_ = 0, modWheel_ = 0;
    float uniCents_ = 0, pan_ = 0, uniGain_ = 1, panMod_ = 0, ampMod_ = 1, fmMod_ = 1;
    double lfoPh_[2] {}, lfoPitch_ = 0, cutOct_ = 0;
    float lfoHold_[2] {};
    float pink_[7] {}, pop_ = 0;
    float table_[4][kTable + 1] {};
    NF nf_;
    Biquad oscF_, noiseF_;
};
} // namespace fw
