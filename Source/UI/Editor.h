#pragma once
#include <JuceHeader.h>

#include <map>

#include "../Plugin/Processor.h"
#include "Look.h"
#include "Views.h"

namespace fwui
{
class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& label)
    {
        const int idx = fw::paramIndex (id.toStdString());
        const auto& d = fw::paramTable()[(size_t) idx];
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 14);
        const float span = d.max - d.min;
        slider.setNumDecimalPlacesToDisplay (span >= 500.0f ? 0 : span >= 20.0f ? 1 : 2);
        if (! d.unit.empty()) slider.setTextValueSuffix (" " + juce::String (d.unit));
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
        slider.setDoubleClickReturnValue (true, d.def);
        name.setText (label, juce::dontSendNotification);
        name.setJustificationType (juce::Justification::centred);
        name.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (slider);
        addAndMakeVisible (name);
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (12)); slider.setBounds (r); }

    juce::Slider slider;
    juce::Label name;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

class ParamCombo : public juce::ComboBox
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState& state, const juce::String& id)
    {
        const auto& d = fw::paramTable()[(size_t) fw::paramIndex (id.toStdString())];
        juce::StringArray items;
        for (const auto& c : d.choices) items.add (c);
        addItemList (items, 1);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, id, *this);
    }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

class ParamToggle : public juce::TextButton
{
public:
    ParamToggle (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& label)
    {
        setButtonText (label);
        setClickingTogglesState (true);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, id, *this);
    }
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};
} // namespace fwui

class FormwaveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FormwaveEditor (FormwaveProcessor&);
    ~FormwaveEditor() override;
    void resized() override;

    static constexpr int kW = 1180, kH = 760;

private:
    struct Content : public juce::Component
    {
        explicit Content (FormwaveEditor& e) : owner (e) {}
        void paint (juce::Graphics& g) override { owner.paintContent (g); }
        FormwaveEditor& owner;
    };

    void paintContent (juce::Graphics&);
    void timerCallback() override;
    void layout();
    void showPresetMenu();
    void stepPreset (int delta);
    void choosePreset (int flatIndex);
    void askSavePreset();
    void updatePresetLabel();
    bool modified() const;

    template <class T, class... A> T* add (A&&... a)
    {
        auto c = std::make_unique<T> (std::forward<A> (a)...);
        auto* p = c.get();
        content.addAndMakeVisible (*p);
        owned.push_back (std::move (c));
        return p;
    }
    fwui::ParamKnob* knob (const char* id, const char* label)
    {
        auto* k = add<fwui::ParamKnob> (proc.apvts, id, label);
        knobs[id] = k;
        return k;
    }

    FormwaveProcessor& proc;
    fwui::FormwaveLook look;
    Content content { *this };
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::map<std::string, fwui::ParamKnob*> knobs;

    struct PanelRect { juce::Rectangle<int> r; juce::String title; };
    std::vector<PanelRect> panels;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, saveBtn { "SAVE" }, initBtn { "INIT" };
    fwui::HarmonicView* harm = nullptr;
    fwui::ScopeView* scope = nullptr;
    fwui::SpectrumView* spectrum = nullptr;
    fwui::EnvView* env = nullptr;
    juce::MidiKeyboardComponent keyboard;
    juce::Rectangle<int> meterArea;

    int lastHash = 0, previewCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormwaveEditor)
};
