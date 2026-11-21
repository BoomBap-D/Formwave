#pragma once
#include <JuceHeader.h>

#include "../Core/Bank.h"
#include "Look.h"

#include <set>

namespace fwui
{
// ---------------------------------------------------------------------------------------------
// Editable harmonic bars with the sum of sine waves drawn underneath.
class HarmonicView : public juce::Component, private juce::Timer
{
public:
    explicit HarmonicView (juce::AudioProcessorValueTreeState& s) : apvts (s)
    {
        for (int k = 0; k < 32; ++k)
        {
            params_[(size_t) k] = apvts.getParameter ("h" + juce::String (k + 1));
            raw_[(size_t) k] = apvts.getRawParameterValue ("h" + juce::String (k + 1));
        }
        const char* names[5] { "SIN", "SAW", "SQR", "TRI", "BUILD" };
        for (int i = 0; i < 5; ++i)
        {
            btn_[i].setButtonText (names[i]);
            addAndMakeVisible (btn_[i]);
            btn_[i].onClick = [this, i] { if (i == 4) startBuild(); else shape (i); };
        }
        useBtn_.setButtonText ("USE ON OSC 1");
        useBtn_.onClick = [this]
        {
            if (auto* p = apvts.getParameter ("o1_wave")) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) fw::W_ADD)); p->endChangeGesture(); }
        };
        addAndMakeVisible (useBtn_);
        startTimerHz (20);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 0);
        auto head = r.removeFromTop (28).withTrimmedTop (4);
        useBtn_.setBounds (head.removeFromRight (104));
        head.removeFromRight (6);
        for (int i = 4; i >= 0; --i) { btn_[i].setBounds (head.removeFromRight (i == 4 ? 54 : 40)); head.removeFromRight (4); }
        auto body = r.withTrimmedBottom (6);
        bars_ = body.removeFromTop (body.getHeight() * 56 / 100);
        body.removeFromTop (4);
        sum_ = body;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (dim);
        g.setFont (mono (10.5f));
        g.drawText ("HARMONICS", 10, 6, 100, 16, juce::Justification::left);
        g.setColour (ink);
        g.fillRoundedRectangle (bars_.toFloat(), 4.0f);
        g.fillRoundedRectangle (sum_.toFloat(), 4.0f);

        const float bw = ((float) bars_.getWidth() - 12.0f) / 32.0f, mid = (float) bars_.getCentreY(), half = (float) bars_.getHeight() * 0.5f - 6.0f;
        g.setColour (line);
        g.drawHorizontalLine ((int) mid, (float) bars_.getX(), (float) bars_.getRight());
        float hv[33] {};
        for (int k = 1; k <= 32; ++k) hv[k] = raw_[(size_t) k - 1]->load();
        for (int k = 1; k <= 32; ++k)
        {
            const float h = hv[k] * half;
            const bool hidden = building_ >= 0 && k > building_;
            g.setColour ((building_ == k ? orange : teal).withAlpha (hidden ? 0.2f : 1.0f));
            g.fillRect ((float) bars_.getX() + 6.0f + (float) (k - 1) * bw + 1.0f, h >= 0 ? mid - h : mid, bw - 2.0f, std::max (1.0f, std::fabs (h)));
            if (k == 1 || k % 8 == 0)
            {
                g.setColour (dim); g.setFont (mono (9.0f));
                g.drawText (juce::String (k), (int) ((float) bars_.getX() + 6.0f + (float) (k - 1) * bw - 4.0f), bars_.getBottom() - 12, (int) bw + 8, 11, juce::Justification::centred);
            }
        }

        // sum of the sines, two cycles
        const int N = 240, upto = building_ >= 0 ? building_ : 32;
        std::vector<float> y ((size_t) N, 0.0f);
        float full = 1e-6f;
        for (int i = 0; i < N; ++i)
        {
            const double x = (double) i / N * 4.0 * fw::kPi;
            double s = 0, sf = 0;
            for (int k = 1; k <= 32; ++k) if (hv[k] != 0.0f) { const double v = hv[k] * std::sin (k * x); sf += v; if (k <= upto) s += v; }
            y[(size_t) i] = (float) s; full = std::max (full, (float) std::fabs (sf));
        }
        juce::Path path;
        const float smid = (float) sum_.getCentreY(), amp = (float) sum_.getHeight() * 0.5f - 6.0f;
        for (int i = 0; i < N; ++i)
        {
            const float px = (float) sum_.getX() + (float) sum_.getWidth() * (float) i / (float) (N - 1), py = smid - y[(size_t) i] / full * amp;
            i == 0 ? path.startNewSubPath (px, py) : path.lineTo (px, py);
        }
        g.setColour (orange);
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
    }

    void mouseDown (const juce::MouseEvent& e) override { touched_.clear(); edit (e); }
    void mouseDrag (const juce::MouseEvent& e) override { edit (e); }
    void mouseUp (const juce::MouseEvent&) override { for (int k : touched_) params_[(size_t) k]->endChangeGesture(); touched_.clear(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { shape (0); }

private:
    void timerCallback() override
    {
        if (building_ >= 0)
        {
            int last = 32; while (last > 1 && raw_[(size_t) last - 1]->load() == 0.0f) --last;
            if (++tick_ % 6 == 0) { ++building_; if (building_ > last + 8) building_ = -1; }
        }
        repaint();
    }
    void startBuild() { building_ = 0; tick_ = 0; }
    void setHarm (int k, float v)
    {
        auto* p = params_[(size_t) k];
        if (touched_.insert (k).second) p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-1.0f, 1.0f, v)));
    }
    void edit (const juce::MouseEvent& e)
    {
        if (! bars_.contains (e.getPosition()) && touched_.empty()) return;
        const float bw = ((float) bars_.getWidth() - 12.0f) / 32.0f;
        const int k = juce::jlimit (0, 31, (int) (((float) e.x - (float) bars_.getX() - 6.0f) / bw));
        const float mid = (float) bars_.getCentreY(), half = (float) bars_.getHeight() * 0.5f - 6.0f;
        float v = (mid - (float) e.y) / half;
        v = std::round (v * 50.0f) / 50.0f;
        if (e.mods.isRightButtonDown() || e.mods.isAltDown()) v = 0.0f;
        building_ = -1;
        setHarm (k, v);
    }
    void shape (int which)
    {
        for (int k = 1; k <= 32; ++k)
        {
            float v = 0;
            if (which == 0) v = k == 1 ? 1.0f : 0.0f;
            else if (which == 1) v = 1.0f / (float) k;
            else if (which == 2) v = k % 2 ? 1.0f / (float) k : 0.0f;
            else v = k % 2 ? (((k - 1) / 2) % 2 ? -1.0f : 1.0f) / (float) (k * k) : 0.0f;
            auto* p = params_[(size_t) k - 1];
            p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (v)); p->endChangeGesture();
        }
        building_ = -1;
    }

    juce::AudioProcessorValueTreeState& apvts;
    std::array<juce::RangedAudioParameter*, 32> params_ {};
    std::array<std::atomic<float>*, 32> raw_ {};
    juce::TextButton btn_[5], useBtn_;
    juce::Rectangle<int> bars_, sum_;
    std::set<int> touched_;
    int building_ = -1, tick_ = 0;
};

// ---------------------------------------------------------------------------------------------
// Live oscilloscope, or the whole shape of the sound over time.
class ScopeView : public juce::Component
{
public:
    ScopeView()
    {
        live_.setButtonText ("LIVE"); shape_.setButtonText ("SHAPE");
        for (auto* b : { &live_, &shape_ }) { b->setClickingTogglesState (true); b->setRadioGroupId (77); addAndMakeVisible (b); }
        live_.setToggleState (true, juce::dontSendNotification);
        live_.onClick = [this] { repaint(); };
        shape_.onClick = [this] { repaint(); };
    }

    void setWindow (const float* w, int n) { win_.assign (w, w + n); if (live_.getToggleState()) repaint(); }
    void setPreview (std::vector<float> samples, double sr)
    {
        const int W = 400;
        peaks_.assign ((size_t) W, { 0.0f, 0.0f });
        float gp = 1e-6f;
        const double per = (double) samples.size() / W;
        for (int x = 0; x < W; ++x)
        {
            float lo = 0, hi = 0;
            const size_t a = (size_t) (x * per), b = std::min (samples.size(), (size_t) ((x + 1) * per) + 1);
            for (size_t i = a; i < b; ++i) { lo = std::min (lo, samples[i]); hi = std::max (hi, samples[i]); }
            peaks_[(size_t) x] = { lo, hi }; gp = std::max ({ gp, -lo, hi });
        }
        for (auto& p : peaks_) { p.first /= gp; p.second /= gp; }
        seconds_ = (float) ((double) samples.size() / sr);
        if (shape_.getToggleState()) repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 0);
        auto head = r.removeFromTop (28).withTrimmedTop (4);
        shape_.setBounds (head.removeFromRight (54)); head.removeFromRight (4);
        live_.setBounds (head.removeFromRight (54));
        area_ = r.withTrimmedBottom (6);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (dim); g.setFont (mono (10.5f));
        g.drawText ("WAVEFORM", 10, 6, 100, 16, juce::Justification::left);
        g.setColour (ink); g.fillRoundedRectangle (area_.toFloat(), 4.0f);
        const float mid = (float) area_.getCentreY(), amp = (float) area_.getHeight() * 0.5f - 6.0f;
        g.setColour (line); g.drawHorizontalLine ((int) mid, (float) area_.getX(), (float) area_.getRight());
        if (live_.getToggleState())
        {
            if (win_.size() < 1024) return;
            size_t s = 0;
            for (size_t i = 1; i < win_.size() / 2; ++i) if (win_[i - 1] < 0 && win_[i] >= 0) { s = i; break; }
            float mx = 0.05f; for (float v : win_) mx = std::max (mx, std::fabs (v));
            juce::Path p;
            const int n = 1024;
            for (int i = 0; i < n; ++i)
            {
                const float x = (float) area_.getX() + (float) area_.getWidth() * (float) i / (float) (n - 1), y = mid - win_[s + (size_t) i] / mx * amp;
                i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
            }
            g.setColour (teal); g.strokePath (p, juce::PathStrokeType (1.8f));
        }
        else if (! peaks_.empty())
        {
            juce::Path p;
            const int W = (int) peaks_.size();
            p.startNewSubPath ((float) area_.getX(), mid);
            for (int x = 0; x < W; ++x) p.lineTo ((float) area_.getX() + (float) area_.getWidth() * (float) x / (float) (W - 1), mid - peaks_[(size_t) x].second * amp);
            for (int x = W - 1; x >= 0; --x) p.lineTo ((float) area_.getX() + (float) area_.getWidth() * (float) x / (float) (W - 1), mid - peaks_[(size_t) x].first * amp);
            p.closeSubPath();
            g.setColour (orange); g.fillPath (p);
            g.setColour (dim); g.setFont (mono (10.0f));
            g.drawText (juce::String (seconds_, 2) + " s", area_.reduced (6, 3), juce::Justification::bottomRight);
        }
    }

private:
    juce::TextButton live_, shape_;
    juce::Rectangle<int> area_;
    std::vector<float> win_;
    std::vector<std::pair<float, float>> peaks_;
    float seconds_ = 0.0f;
};

// ---------------------------------------------------------------------------------------------
// Spectrum bars on top, scrolling spectrogram underneath.
class SpectrumView : public juce::Component
{
public:
    static constexpr int kOrder = 11, kSize = 1 << kOrder, kBands = 64;

    SpectrumView() : fft_ (kOrder), window_ (kSize, juce::dsp::WindowingFunction<float>::hann)
    {
        for (int i = 0; i < 256; ++i) lut_[(size_t) i] = colourFor ((float) i / 255.0f);
    }

    void update (const float* samples, double sr)
    {
        sr_ = sr;
        std::copy (samples, samples + kSize, data_.begin());
        std::fill (data_.begin() + kSize, data_.end(), 0.0f);
        window_.multiplyWithWindowingTable (data_.data(), kSize);
        fft_.performFrequencyOnlyForwardTransform (data_.data());
        for (int b = 0; b < kBands; ++b)
        {
            const double f0 = 30.0 * std::pow (16000.0 / 30.0, (double) b / kBands), f1 = 30.0 * std::pow (16000.0 / 30.0, (double) (b + 1) / kBands);
            const int i0 = (int) (f0 / sr * kSize), i1 = std::max (i0 + 1, (int) (f1 / sr * kSize));
            float m = 0.0f;
            for (int i = i0; i < i1 && i < kSize / 2; ++i) m = std::max (m, data_[(size_t) i]);
            const float db = juce::Decibels::gainToDecibels (m / (float) kSize * 4.0f, -100.0f);
            const float v = juce::jlimit (0.0f, 1.0f, (db + 85.0f) / 70.0f);
            bands_[(size_t) b] = std::max (v, bands_[(size_t) b] * 0.86f);
            col_[(size_t) b] = v;
        }
        pushColumn();
        repaint();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 0);
        r.removeFromTop (28);
        r.removeFromBottom (6);
        spec_ = r.removeFromTop (r.getHeight() * 45 / 100);
        r.removeFromTop (4);
        gram_ = r;
        img_ = juce::Image (juce::Image::RGB, std::max (2, gram_.getWidth()), 96, true);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (dim); g.setFont (mono (10.5f));
        g.drawText ("SPECTRUM", 10, 6, 100, 16, juce::Justification::left);
        g.setColour (ink); g.fillRoundedRectangle (spec_.toFloat(), 4.0f);
        const float bw = (float) spec_.getWidth() / kBands;
        for (int b = 0; b < kBands; ++b)
        {
            const float v = bands_[(size_t) b], h = v * ((float) spec_.getHeight() - 14.0f);
            g.setColour ((v > 0.7f ? orange : teal).withAlpha (0.4f + 0.6f * v));
            g.fillRect ((float) spec_.getX() + (float) b * bw + 0.5f, (float) spec_.getBottom() - 12.0f - h, bw - 1.0f, h);
        }
        g.setColour (dim); g.setFont (mono (9.0f));
        for (double f : { 100.0, 1000.0, 10000.0 })
        {
            const float x = (float) spec_.getX() + (float) (std::log (f / 30.0) / std::log (16000.0 / 30.0)) * (float) spec_.getWidth();
            g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), (int) x - 12, spec_.getBottom() - 12, 24, 11, juce::Justification::centred);
        }
        g.drawImage (img_, gram_.toFloat());
    }

private:
    static juce::Colour colourFor (float v)
    {
        struct S { float p; juce::uint8 r, g, b; };
        static const S st[] { { 0.0f, 10, 11, 13 }, { 0.3f, 15, 74, 82 }, { 0.55f, 61, 214, 198 }, { 0.8f, 255, 106, 43 }, { 1.0f, 255, 241, 214 } };
        for (int i = 1; i < 5; ++i)
            if (v <= st[i].p)
            {
                const float k = (v - st[i - 1].p) / (st[i].p - st[i - 1].p);
                return juce::Colour ((juce::uint8) (st[i - 1].r + (st[i].r - st[i - 1].r) * k), (juce::uint8) (st[i - 1].g + (st[i].g - st[i - 1].g) * k), (juce::uint8) (st[i - 1].b + (st[i].b - st[i - 1].b) * k));
            }
        return juce::Colours::white;
    }

    void pushColumn()
    {
        if (! img_.isValid()) return;
        const int w = img_.getWidth(), h = img_.getHeight();
        img_.moveImageSection (0, 0, 1, 0, w - 1, h);
        juce::Image::BitmapData bd (img_, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < h; ++y)
        {
            const int band = juce::jlimit (0, kBands - 1, (int) ((float) (h - 1 - y) / (float) h * kBands));
            bd.setPixelColour (w - 1, y, lut_[(size_t) juce::jlimit (0, 255, (int) (col_[(size_t) band] * 255.0f))]);
        }
    }

    juce::dsp::FFT fft_;
    juce::dsp::WindowingFunction<float> window_;
    std::array<float, kSize * 2> data_ {};
    std::array<float, kBands> bands_ {}, col_ {};
    std::array<juce::Colour, 256> lut_;
    juce::Rectangle<int> spec_, gram_;
    juce::Image img_;
    double sr_ = 44100.0;
};

// ---------------------------------------------------------------------------------------------
// Small picture of the amplitude envelope.
class EnvView : public juce::Component, private juce::Timer
{
public:
    explicit EnvView (juce::AudioProcessorValueTreeState& s) : apvts (s) { startTimerHz (15); }
    void paint (juce::Graphics& g) override
    {
        g.setColour (ink); g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);
        auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
        const bool drum = P ("mode") < 0.5f;
        const float A = P ("amp_a"), D = P ("amp_d"), S = P ("amp_s"), R = P ("amp_r"), L = P ("len");
        const float total = drum ? L * 1.1f : A + D * 1.5f + 0.3f + R;
        auto amp = [&] (float t) -> float
        {
            if (drum) return t < 0.0015f ? t / 0.0015f : std::pow (0.0005f, std::min (1.0f, (t - 0.0015f) / std::max (0.01f, L - 0.0015f)));
            const float hold = A + D * 1.5f + 0.3f;
            if (t < hold) return t < A ? t / A : S + (1.0f - S) * std::exp (-(t - A) / (D / 3.0f));
            const float v0 = S + (1.0f - S) * std::exp (-(hold - A) / (D / 3.0f));
            return v0 * std::exp (-(t - hold) / (R / 3.0f));
        };
        auto b = getLocalBounds().toFloat().reduced (6.0f);
        juce::Path p;
        for (int i = 0; i <= 120; ++i)
        {
            const float x = b.getX() + b.getWidth() * (float) i / 120.0f, y = b.getBottom() - amp (total * (float) i / 120.0f) * b.getHeight();
            i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
        }
        g.setColour (teal); g.strokePath (p, juce::PathStrokeType (2.0f));
        g.setColour (dim); g.setFont (mono (9.0f));
        g.drawText (drum ? "ONE-SHOT" : "ADSR", getLocalBounds().reduced (6, 2), juce::Justification::topRight);
    }
private:
    void timerCallback() override { repaint(); }
    juce::AudioProcessorValueTreeState& apvts;
};
} // namespace fwui
