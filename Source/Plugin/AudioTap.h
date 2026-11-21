#pragma once
#include <JuceHeader.h>

#include <array>

// Lock-free hand-off of the output signal from the audio thread to the UI (scope and spectrum).
class AudioTap
{
public:
    static constexpr int kWindow = 2048;

    // audio thread
    void push (const float* left, const float* right, int n)
    {
        int s1, z1, s2, z2;
        fifo_.prepareToWrite (n, s1, z1, s2, z2);
        for (int i = 0; i < z1; ++i) buf_[(size_t) (s1 + i)] = 0.5f * (left[i] + right[i]);
        for (int i = 0; i < z2; ++i) buf_[(size_t) (s2 + i)] = 0.5f * (left[z1 + i] + right[z1 + i]);
        fifo_.finishedWrite (z1 + z2);     // if the UI is slow, the surplus is simply dropped
    }

    // UI thread: drain everything new into the rolling window. Returns the number of new samples.
    int pull()
    {
        const int ready = fifo_.getNumReady();
        int s1, z1, s2, z2;
        fifo_.prepareToRead (ready, s1, z1, s2, z2);
        auto append = [this] (const float* src, int n)
        {
            if (n >= kWindow) { std::copy (src + n - kWindow, src + n, window_.begin()); return; }
            std::copy (window_.begin() + n, window_.end(), window_.begin());
            std::copy (src, src + n, window_.end() - n);
        };
        if (z1 > 0) append (buf_.data() + s1, z1);
        if (z2 > 0) append (buf_.data() + s2, z2);
        fifo_.finishedRead (z1 + z2);
        return z1 + z2;
    }

    const float* window() const { return window_.data(); }   // oldest to newest, kWindow samples

private:
    juce::AbstractFifo fifo_ { 16384 };
    std::array<float, 16384> buf_ {};
    std::array<float, kWindow> window_ {};
};
