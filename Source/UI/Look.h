#pragma once
#include <JuceHeader.h>

namespace fwui
{
inline const juce::Colour bg     { 0xff0c0d10 };
inline const juce::Colour panel  { 0xff15171b };
inline const juce::Colour panel2 { 0xff1c1f24 };
inline const juce::Colour ink    { 0xff0a0b0d };
inline const juce::Colour line   { 0xff272b31 };
inline const juce::Colour text   { 0xffe9e6df };
inline const juce::Colour dim    { 0xff8d939c };
inline const juce::Colour teal   { 0xff3dd6c6 };
inline const juce::Colour orange { 0xffff6a2b };

inline juce::Font mono (float h) { return juce::Font (juce::Font::getDefaultMonospacedFontName(), h, juce::Font::plain); }
inline juce::Font sans (float h, bool bold = false) { return juce::Font (juce::Font::getDefaultSansSerifFontName(), h, bold ? juce::Font::bold : juce::Font::plain); }

class FormwaveLook : public juce::LookAndFeel_V4
{
public:
    FormwaveLook()
    {
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::Slider::textBoxTextColourId, teal);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, text);
        setColour (juce::ComboBox::backgroundColourId, panel2);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, line);
        setColour (juce::ComboBox::arrowColourId, dim);
        setColour (juce::PopupMenu::backgroundColourId, panel2);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, teal);
        setColour (juce::PopupMenu::highlightedTextColourId, ink);
        setColour (juce::TextButton::buttonColourId, panel2);
        setColour (juce::TextButton::buttonOnColourId, teal);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, ink);
        setColour (juce::TextEditor::backgroundColourId, ink);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, line);
        setColour (juce::AlertWindow::backgroundColourId, panel);
        setColour (juce::AlertWindow::textColourId, text);
        setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
        setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, teal);
        setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, teal.withAlpha (0.35f));
        setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe6e2d9));
        setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff15171b));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float d = std::min (b.getWidth(), b.getHeight()), r = d * 0.5f;
        const auto c = b.getCentre();
        const float angle = start + pos * (end - start);
        juce::Path track;
        track.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0.0f, start, end, true);
        g.setColour (line);
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // bipolar parameters draw from the centre
        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float zero = bipolar ? start + (float) s.valueToProportionOfLength (0.0) * (end - start) : start;
        juce::Path val;
        val.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0.0f, std::min (zero, angle), std::max (zero, angle), true);
        g.setColour (s.isEnabled() ? teal : dim);
        g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (panel2);
        g.fillEllipse (c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);
        const float px = c.x + std::sin (angle) * r * 0.58f, py = c.y - std::cos (angle) * r * 0.58f;
        g.setColour (text);
        g.drawLine (c.x + std::sin (angle) * r * 0.2f, c.y - std::cos (angle) * r * 0.2f, px, py, 2.0f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& colour, bool over, bool down) override
    {
        auto c = colour;
        if (over) c = c.brighter (0.1f);
        if (down) c = c.brighter (0.2f);
        g.setColour (c);
        g.fillRoundedRectangle (b.getLocalBounds().toFloat().reduced (0.5f), 4.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.getLocalBounds().toFloat().reduced (0.5f), 4.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return mono (juce::jmin (12.0f, (float) h * 0.5f)); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return mono (12.0f); }
    juce::Font getPopupMenuFont() override { return sans (14.0f); }
    juce::Font getLabelFont (juce::Label&) override { return mono (10.5f); }
};
} // namespace fwui
