#pragma once
// Stereo effects: chorus, delay, reverb. Buffers are allocated in prepare(), never in process().
#include "Params.h"
#include "Voice.h"

#include <vector>

namespace fw
{
class Chorus
{
public:
    void prepare (double sr)
    {
        sr_ = sr;
        const size_t n = (size_t) (sr * 0.05) + 8;
        bufL_.assign (n, 0.0f); bufR_.assign (n, 0.0f); pos_ = 0; phase_ = 0.0;
    }
    void process (float& l, float& r, float mix, float rate, float depth)
    {
        if (mix <= 0.0001f) { push (l, r); return; }
        phase_ += rate / sr_; phase_ -= std::floor (phase_);
        const double base = 0.012 * sr_, dep = (0.002 + 0.006 * depth) * sr_;
        const float wl = read (bufL_, base + dep * std::sin (2.0 * kPi * phase_));
        const float wr = read (bufR_, base + dep * std::sin (2.0 * kPi * (phase_ + 0.5)));
        push (l, r);
        l = l * (1.0f - 0.5f * mix) + wl * mix * 0.7f;
        r = r * (1.0f - 0.5f * mix) + wr * mix * 0.7f;
    }
private:
    void push (float l, float r) { bufL_[pos_] = l; bufR_[pos_] = r; pos_ = (pos_ + 1) % bufL_.size(); }
    float read (const std::vector<float>& b, double delay) const
    {
        double p = (double) pos_ - delay; while (p < 0) p += (double) b.size();
        const size_t i0 = (size_t) p % b.size(), i1 = (i0 + 1) % b.size();
        const float f = (float) (p - std::floor (p));
        return b[i0] + (b[i1] - b[i0]) * f;
    }
    std::vector<float> bufL_, bufR_;
    size_t pos_ = 0; double phase_ = 0, sr_ = 44100;
};

class StereoDelay
{
public:
    void prepare (double sr)
    {
        sr_ = sr;
        const size_t n = (size_t) (sr * 1.1) + 8;
        bufL_.assign (n, 0.0f); bufR_.assign (n, 0.0f); pos_ = 0; lpL_ = lpR_ = 0.0f;
    }
    void process (float& l, float& r, float ms, float fb, float mix)
    {
        if (mix <= 0.0001f) { bufL_[pos_] = l; bufR_[pos_] = r; pos_ = (pos_ + 1) % bufL_.size(); return; }
        const size_t d = std::min (bufL_.size() - 2, (size_t) (ms * 0.001 * sr_));
        const size_t rp = (pos_ + bufL_.size() - d) % bufL_.size();
        const float dl = bufL_[rp], dr = bufR_[rp];
        lpL_ += 0.45f * (dl - lpL_); lpR_ += 0.45f * (dr - lpR_);
        // ping-pong: each side feeds the other
        bufL_[pos_] = l + lpR_ * fb;
        bufR_[pos_] = r + lpL_ * fb;
        pos_ = (pos_ + 1) % bufL_.size();
        l += dl * mix; r += dr * mix;
    }
private:
    std::vector<float> bufL_, bufR_;
    size_t pos_ = 0; double sr_ = 44100; float lpL_ = 0, lpR_ = 0;
};

// Freeverb-style reverb.
class Reverb
{
public:
    void prepare (double sr)
    {
        static const int ct[8] { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int at[4] { 556, 441, 341, 225 };
        const double k = sr / 44100.0;
        for (int i = 0; i < 8; ++i) { combL_[i].init ((int) (ct[i] * k)); combR_[i].init ((int) ((ct[i] + 23) * k)); }
        for (int i = 0; i < 4; ++i) { apL_[i].init ((int) (at[i] * k)); apR_[i].init ((int) ((at[i] + 23) * k)); }
    }
    void process (float& l, float& r, float size, float damp, float mix)
    {
        if (mix <= 0.0001f) return;
        const float fb = 0.7f + 0.28f * size, dm = 0.1f + 0.8f * damp, in = (l + r) * 0.015f;
        float ol = 0, or_ = 0;
        for (int i = 0; i < 8; ++i) { ol += combL_[i].process (in, fb, dm); or_ += combR_[i].process (in, fb, dm); }
        for (int i = 0; i < 4; ++i) { ol = apL_[i].process (ol); or_ = apR_[i].process (or_); }
        l += ol * mix * 3.0f; r += or_ * mix * 3.0f;
    }
private:
    struct Comb
    {
        std::vector<float> b; size_t p = 0; float s = 0;
        void init (int n) { b.assign ((size_t) std::max (1, n), 0.0f); p = 0; s = 0; }
        float process (float x, float fb, float dm) { const float o = b[p]; s = o * (1.0f - dm) + s * dm; b[p] = x + s * fb; p = (p + 1) % b.size(); return o; }
    };
    struct Allpass
    {
        std::vector<float> b; size_t p = 0;
        void init (int n) { b.assign ((size_t) std::max (1, n), 0.0f); p = 0; }
        float process (float x) { const float bo = b[p]; const float o = -x + bo; b[p] = x + bo * 0.5f; p = (p + 1) % b.size(); return o; }
    };
    Comb combL_[8], combR_[8]; Allpass apL_[4], apR_[4];
};
} // namespace fw
