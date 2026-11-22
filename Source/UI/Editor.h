#pragma once
#include <JuceHeader.h>

#include <array>
#include <map>

#include "../Plugin/Processor.h"
#include "Look.h"
#include "Panels.h"
#include "Views.h"

namespace fwui
{
using HintFn = std::function<void (const juce::String&, const juce::String&)>;

class ParamKnob : public juce::Component
{
public:
    ParamKnob (juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& label, HintFn hint)
        : onHint (std::move (hint))
    {
        const int idx = fw::paramIndex (id.toStdString());
        const auto& d = fw::paramTable()[(size_t) idx];
        fullName = juce::String (d.name).toUpperCase();
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 13);
        const float span = d.max - d.min;
        slider.setNumDecimalPlacesToDisplay (span >= 500.0f ? 0 : span >= 20.0f ? 1 : 2);
        if (! d.unit.empty()) slider.setTextValueSuffix (" " + juce::String (d.unit));
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, id, slider);
        slider.setDoubleClickReturnValue (true, d.def);
        slider.onValueChange = [this] { pushHint(); };
        slider.addMouseListener (this, false);
        name.setText (label, juce::dontSendNotification);
        name.setJustificationType (juce::Justification::centred);
        name.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (slider);
        addAndMakeVisible (name);
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (11)); slider.setBounds (r); }
    void mouseEnter (const juce::MouseEvent&) override { pushHint(); }
    void mouseExit (const juce::MouseEvent&) override { if (onHint) onHint ({}, {}); }

    juce::Slider slider;
    juce::Label name;
private:
    void pushHint()
    {
        if (onHint) onHint (fullName, slider.getTextFromValue (slider.getValue()) + "     drag to change  ·  double-click to reset  ·  mouse wheel for fine steps");
    }
    juce::String fullName;
    HintFn onHint;
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

// Modulation tab: two LFOs, unison, velocity and mod-wheel routing.
class ModPanel : public juce::Component
{
public:
    ModPanel (juce::AudioProcessorValueTreeState& s, HintFn hint)
    {
        auto k = [&] (const char* id, const char* label)
        {
            auto c = std::make_unique<ParamKnob> (s, id, label, hint);
            addAndMakeVisible (*c); knobs[id] = c.get(); ownedK.push_back (std::move (c));
        };
        auto cb = [&] (const char* id)
        {
            auto c = std::make_unique<ParamCombo> (s, id);
            addAndMakeVisible (*c); combos[id] = c.get(); ownedC.push_back (std::move (c));
        };
        for (int n = 1; n <= 2; ++n)
        {
            const std::string p = "lfo" + std::to_string (n) + "_";
            cb ((p + "shape").c_str()); cb ((p + "dest").c_str());
            k ((p + "rate").c_str(), "RATE"); k ((p + "dep").c_str(), "DEPTH");
        }
        cb ("uni_n"); k ("uni_det", "DETUNE"); k ("uni_wid", "SPREAD");
        k ("vel_amp", "TO LEVEL"); k ("vel_cut", "TO CUTOFF");
        k ("mw_vib", "VIBRATO"); k ("mw_cut", "CUTOFF");
    }

    void resized() override
    {
        auto a = getLocalBounds().reduced (14, 8);
        const int bw = (a.getWidth() - 16) / 2;
        for (int n = 0; n < 2; ++n)
        {
            const int x = a.getX() + n * (bw + 16), y = a.getY() + 16;
            const std::string p = "lfo" + std::to_string (n + 1) + "_";
            combos[p + "shape"]->setBounds (x, y, 150, 24);
            combos[p + "dest"]->setBounds (x, y + 30, 150, 24);
            knobs[p + "rate"]->setBounds (x + 160, y - 4, (bw - 160) / 2, 60);
            knobs[p + "dep"]->setBounds (x + 160 + (bw - 160) / 2, y - 4, (bw - 160) / 2, 60);
        }
        const int y2 = a.getY() + 108;
        combos["uni_n"]->setBounds (a.getX(), y2 + 18, 84, 24);
        knobs["uni_det"]->setBounds (a.getX() + 96, y2 + 2, 64, 60);
        knobs["uni_wid"]->setBounds (a.getX() + 164, y2 + 2, 64, 60);
        knobs["vel_amp"]->setBounds (a.getX() + 262, y2 + 2, 64, 60);
        knobs["vel_cut"]->setBounds (a.getX() + 330, y2 + 2, 64, 60);
        knobs["mw_vib"]->setBounds (a.getX() + 426, y2 + 2, 64, 60);
        knobs["mw_cut"]->setBounds (a.getX() + 494, y2 + 2, 64, 60);
    }

    void paint (juce::Graphics& g) override
    {
        auto a = getLocalBounds().reduced (14, 8);
        g.setColour (dim);
        g.setFont (mono (10.0f));
        const int bw = (a.getWidth() - 16) / 2;
        g.drawText ("LFO 1", a.getX(), a.getY(), 160, 14, juce::Justification::left);
        g.drawText ("LFO 2", a.getX() + bw + 16, a.getY(), 160, 14, juce::Justification::left);
        const int y2 = a.getY() + 108;
        g.drawText ("UNISON", a.getX(), y2 - 6, 100, 14, juce::Justification::left);
        g.drawText ("VELOCITY", a.getX() + 262, y2 - 6, 100, 14, juce::Justification::left);
        g.drawText ("MOD WHEEL", a.getX() + 426, y2 - 6, 100, 14, juce::Justification::left);
        g.setColour (line);
        g.drawHorizontalLine (a.getY() + 98, (float) a.getX(), (float) a.getRight());
        g.drawVerticalLine (a.getX() + bw + 8, (float) a.getY() + 4, (float) a.getY() + 92.0f);
        g.setColour (dim.withAlpha (0.8f));
        g.setFont (mono (9.5f));
        g.drawText ("cutoff targets need the filter switched on", a.getX() + bw + 16, a.getY() + 76, bw, 14, juce::Justification::left);
    }

private:
    std::map<std::string, ParamKnob*> knobs;
    std::map<std::string, ParamCombo*> combos;
    std::vector<std::unique_ptr<ParamKnob>> ownedK;
    std::vector<std::unique_ptr<ParamCombo>> ownedC;
};
} // namespace fwui

class FormwaveEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FormwaveEditor (FormwaveProcessor&);
    ~FormwaveEditor() override;
    void resized() override;

    static constexpr int kW = 1280, kH = 800;

private:
    struct Content : public juce::Component
    {
        explicit Content (FormwaveEditor& e) : owner (e) {}
        void paint (juce::Graphics& g) override { owner.paintContent (g); }
        FormwaveEditor& owner;
    };
    struct PanelRect { juce::Rectangle<int> r; juce::String title; };
    struct Caption { int x, y; juce::String text; };

    void paintContent (juce::Graphics&);
    void timerCallback() override;
    void layout();
    void showMainMenu();
    void stepPreset (int delta);
    void choosePreset (int flatIndex);
    void askSavePreset();
    void updatePresetLabel();
    void toggleAB();
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
        auto* k = add<fwui::ParamKnob> (proc.apvts, id, label, [this] (const juce::String& a, const juce::String& b) { hint.setHint (a, b); });
        knobs[id] = k;
        return k;
    }
    fwui::ParamCombo* combo (const char* id) { auto* c = add<fwui::ParamCombo> (proc.apvts, id); combos[id] = c; return c; }
    fwui::ParamToggle* toggle (const char* id, const char* label) { auto* t = add<fwui::ParamToggle> (proc.apvts, id, label); toggles[id] = t; return t; }

    // place a row of knobs evenly across a rectangle
    void knobRow (std::initializer_list<const char*> ids, juce::Rectangle<int> r)
    {
        const int n = (int) ids.size();
        int i = 0;
        for (auto* id : ids) { if (auto* k = knobs[id]) k->setBounds (r.getX() + r.getWidth() * i / n, r.getY(), r.getWidth() / n, r.getHeight()); ++i; }
    }

    FormwaveProcessor& proc;
    fwui::FormwaveLook look;
    Content content { *this };
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::map<std::string, fwui::ParamKnob*> knobs;
    std::map<std::string, fwui::ParamCombo*> combos;
    std::map<std::string, fwui::ParamToggle*> toggles;
    std::vector<PanelRect> panels;
    std::vector<Caption> captions;

    fwui::HintBar hint;
    fwui::PresetBrowser browser;
    fwui::TabbedDisplay tabs;
    fwui::FilterCurveView* filterCurve = nullptr;
    fwui::WheelView bendWheel { true, "BEND", fwui::teal }, modWheel { false, "MOD", fwui::orange };
    std::array<fwui::WaveSelector*, 4> waves {};

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, abBtn { "A" }, menuBtn { "=" };
    fwui::ModPanel* modPanel = nullptr;
    fwui::HarmonicView* harm = nullptr;
    fwui::ScopeView* scope = nullptr;
    fwui::SpectrumView* spectrum = nullptr;
    fwui::EnvView* env = nullptr;
    juce::MidiKeyboardComponent keyboard;
    juce::Rectangle<int> meterArea, displayRight;

    fw::Patch abSlot[2];
    int abActive = 0;
    bool abUsed = false;
    int lastHash = 0, previewCountdown = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FormwaveEditor)
};
