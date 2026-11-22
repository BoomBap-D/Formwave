#pragma once
#include <JuceHeader.h>

#include "../Core/Engine.h"
#include "AudioTap.h"
#include "PresetStore.h"

class FormwaveProcessor : public juce::AudioProcessor
{
public:
    FormwaveProcessor();
    ~FormwaveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Formwave"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- shared with the editor ----
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    AudioTap tap;
    PresetStore presets;
    std::atomic<float> outputPeak { 0.0f };
    // On-screen wheels (editor writes, audio thread applies): bend in semitone fraction -1..1, mod 0..1
    std::atomic<float> uiBend { 0.0f }, uiMod { 0.0f };

    // Message thread: write every parameter from a patch (host is notified, so automation lanes follow).
    void applyPatch (const fw::Patch& p);
    // Message thread: read the current parameter values as a patch.
    fw::Patch currentPatch() const;

    juce::String presetName() const { return presetName_; }
    void setPresetName (const juce::String& n) { presetName_ = n; }
    int presetIndex() const { return presetIndex_; }
    void setPresetIndex (int i) { presetIndex_ = i; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    fw::Engine engine_;
    std::array<std::atomic<float>*, fw::P_COUNT> raw_ {};
    std::array<juce::RangedAudioParameter*, fw::P_COUNT> params_ {};
    float lastUiBend_ = 0.0f, lastUiMod_ = 0.0f;
    juce::String presetName_ { "Init" };
    int presetIndex_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormwaveProcessor)
};
