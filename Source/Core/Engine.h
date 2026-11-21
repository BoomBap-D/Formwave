#pragma once
#include "Fx.h"
#include "Voice.h"

#include <array>

namespace fw
{
class Engine
{
public:
    static constexpr int kVoices = 16;

    void prepare (double sr)
    {
        sr_ = sr;
        for (auto& v : voices_) v = Voice();
        chorus_.prepare (sr); delay_.prepare (sr); reverb_.prepare (sr);
        patch_ = Patch::init().v;
    }

    // Copy new parameter values in (audio thread, once per block). Sounding notes follow live.
    void setPatch (const float* values) { for (int i = 0; i < P_COUNT; ++i) patch_[(size_t) i] = values[i]; }
    const float* patch() const { return patch_.data(); }

    void noteOn (int note, float vel)
    {
        const bool mono = patch_[P_MONO] > 0.5f;
        float prev = 0.0f;
        if (mono)
            for (auto& v : voices_) if (v.active()) { prev = v.baseHz(); v.kill(); }
        for (auto& v : voices_)
            if (v.active() && v.note() == note) v.kill();      // retrigger the same note cleanly
        Voice* c = nullptr;
        for (auto& v : voices_) if (! v.active()) { c = &v; break; }
        if (! c) { c = &voices_[0]; for (auto& v : voices_) if (v.order() < c->order()) c = &v; }
        ++order_;
        c->start (patch_.data(), sr_, note, vel, prev, order_, (uint32_t) order_);
        c->setBend (bend_); c->setModWheel (mod_);
        sustained_[(size_t) note & 127] = false;
    }
    void noteOff (int note)
    {
        for (auto& v : voices_)
            if (v.active() && v.note() == note && v.held())
            {
                if (sustain_) sustained_[(size_t) note & 127] = true; else v.release();
            }
    }
    void setSustain (bool on)
    {
        sustain_ = on;
        if (! on)
            for (auto& v : voices_)
                if (v.active() && v.held() && sustained_[(size_t) v.note() & 127]) { v.release(); sustained_[(size_t) v.note() & 127] = false; }
    }
    void setBend (float semis) { bend_ = semis; for (auto& v : voices_) v.setBend (semis); }
    void setModWheel (float x) { mod_ = x; for (auto& v : voices_) v.setModWheel (x); }
    void allNotesOff() { for (auto& v : voices_) v.kill(); sustained_.fill (false); }
    int activeVoices() const { int n = 0; for (const auto& v : voices_) n += v.active() ? 1 : 0; return n; }

    void process (float* left, float* right, int n)
    {
        const float* p = patch_.data();
        for (int i = 0; i < n; ++i)
        {
            float s = 0.0f;
            for (auto& v : voices_) { float o; if (v.process (o)) s += o; }
            float l = s, r = s;
            chorus_.process (l, r, p[P_CH_MIX], p[P_CH_RATE], p[P_CH_DEP]);
            delay_.process (l, r, p[P_DL_TIME], p[P_DL_FB], p[P_DL_MIX]);
            reverb_.process (l, r, p[P_RV_SIZE], p[P_RV_DAMP], p[P_RV_MIX]);
            left[i] = softClip (l); right[i] = softClip (r);
        }
    }

private:
    // Transparent below about -3 dB, then rounds off so the output can never exceed 1.
    static float softClip (float x)
    {
        const float a = std::fabs (x);
        if (a < 0.7f) return x;
        const float s = x < 0 ? -1.0f : 1.0f;
        return s * (0.7f + 0.3f * std::tanh ((a - 0.7f) / 0.3f));
    }

    std::array<Voice, kVoices> voices_ {};
    std::array<float, P_COUNT> patch_ {};
    std::array<bool, 128> sustained_ {};
    Chorus chorus_; StereoDelay delay_; Reverb reverb_;
    double sr_ = 44100; uint64_t order_ = 0; float bend_ = 0, mod_ = 0; bool sustain_ = false;
};
} // namespace fw
