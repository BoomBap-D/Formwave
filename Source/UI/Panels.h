#pragma once
#include <JuceHeader.h>

#include <functional>

#include "../Core/Bank.h"
#include "../Plugin/PresetStore.h"
#include "Look.h"

namespace fwui
{
inline int textWidth (const juce::Font& f, const juce::String& t) { juce::GlyphArrangement ga; ga.addLineOfText (f, t, 0.0f, 0.0f); return (int) std::ceil (ga.getBoundingBox (0, -1, true).getWidth()); }

// ---------------------------------------------------------------------------------------------
// One-line status bar: names whatever the mouse is over and how to use it.
class HintBar : public juce::Component
{
public:
    void setHint (const juce::String& label, const juce::String& detail)
    {
        if (label != label_ || detail != detail_) { label_ = label; detail_ = detail; repaint(); }
    }
    void setRight (const juce::String& s) { if (s != right_) { right_ = s; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ink);
        g.setColour (line);
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());
        auto r = getLocalBounds().reduced (16, 0);
        g.setFont (mono (11.0f));
        g.setColour (dim);
        g.drawText (right_, r, juce::Justification::centredRight);
        g.setColour (teal);
        const int lw = label_.isEmpty() ? 0 : textWidth (mono (11.0f), label_) + 12;
        g.drawText (label_, r, juce::Justification::centredLeft);
        g.setColour (dim);
        g.drawText (detail_, r.withTrimmedLeft (lw), juce::Justification::centredLeft);
    }

private:
    juce::String label_, detail_, right_;
};

// ---------------------------------------------------------------------------------------------
// Six glyph buttons that drive a choice parameter (oscillator wave shape).
class WaveSelector : public juce::Component
{
public:
    WaveSelector (juce::AudioProcessorValueTreeState& s, const juce::String& id)
        : param_ (s.getParameter (id)), attach_ (*s.getParameter (id), [this] (float v) { value_ = (int) (v + 0.5f); repaint(); })
    {
        attach_.sendInitialUpdate();
    }

    std::function<void (const juce::String&)> onHint;

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (ink);
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);
        for (int i = 0; i < 6; ++i)
        {
            auto c = cell (i);
            const bool on = i == value_, hot = i == hover_;
            if (on) { g.setColour (teal); g.fillRoundedRectangle (c, 4.0f); }
            else if (hot) { g.setColour (panel2); g.fillRoundedRectangle (c, 4.0f); }
            g.setColour (on ? ink : hot ? text : dim);
            g.strokePath (glyph (i, c.reduced (6.0f, 5.0f)), juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    void mouseMove (const juce::MouseEvent& e) override { setHover (indexAt (e.position)); }
    void mouseExit (const juce::MouseEvent&) override { setHover (-1); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = indexAt (e.position);
        if (i >= 0) attach_.setValueAsCompleteGesture ((float) i);
    }

    static const char* waveName (int i)
    {
        static const char* n[6] { "Sine", "Triangle", "Saw", "Square", "Additive (draw the harmonics)", "Metal (inharmonic partials)" };
        return n[juce::jlimit (0, 5, i)];
    }

private:
    juce::Rectangle<float> cell (int i) const
    {
        auto b = getLocalBounds().toFloat().reduced (2.0f);
        const float w = b.getWidth() / 6.0f;
        return { b.getX() + w * (float) i, b.getY(), w, b.getHeight() };
    }
    int indexAt (juce::Point<float> p) const { for (int i = 0; i < 6; ++i) if (cell (i).contains (p)) return i; return -1; }
    void setHover (int i)
    {
        if (i == hover_) return;
        hover_ = i;
        if (onHint) onHint (i >= 0 ? juce::String (waveName (i)) : juce::String());
        repaint();
    }

    static juce::Path glyph (int w, juce::Rectangle<float> r)
    {
        juce::Path p;
        const float x0 = r.getX(), x1 = r.getRight(), y0 = r.getY(), y1 = r.getBottom(), ym = r.getCentreY(), W = r.getWidth();
        switch (w)
        {
            case 0:
                p.startNewSubPath (x0, ym);
                for (int i = 1; i <= 24; ++i) p.lineTo (x0 + W * (float) i / 24.0f, ym - std::sin ((float) i / 24.0f * 6.2832f) * r.getHeight() * 0.5f);
                break;
            case 1: p.startNewSubPath (x0, y1); p.lineTo (x0 + W * 0.25f, y0); p.lineTo (x0 + W * 0.75f, y1); p.lineTo (x1, y0); break;
            case 2: p.startNewSubPath (x0, y1); p.lineTo (x0 + W * 0.5f, y0); p.lineTo (x0 + W * 0.5f, y1); p.lineTo (x1, y0); break;
            case 3: p.startNewSubPath (x0, y1); p.lineTo (x0, y0); p.lineTo (x0 + W * 0.5f, y0); p.lineTo (x0 + W * 0.5f, y1); p.lineTo (x1, y1); p.lineTo (x1, y0); break;
            case 4:
                for (int i = 0; i < 5; ++i)
                {
                    const float x = x0 + W * ((float) i + 0.5f) / 5.0f, h = r.getHeight() * (1.0f - 0.18f * (float) i);
                    p.startNewSubPath (x, y1); p.lineTo (x, y1 - h);
                }
                break;
            default:
            {
                const float ys[] { 0.5f, 0.1f, 0.9f, 0.25f, 0.8f, 0.15f, 0.85f, 0.3f, 0.5f };
                for (int i = 0; i < 9; ++i)
                {
                    const float x = x0 + W * (float) i / 8.0f, y = y0 + ys[i] * r.getHeight();
                    i == 0 ? p.startNewSubPath (x, y) : p.lineTo (x, y);
                }
                break;
            }
        }
        return p;
    }

    juce::RangedAudioParameter* param_;
    juce::ParameterAttachment attach_;
    int value_ = 0, hover_ = -1;
};

// ---------------------------------------------------------------------------------------------
// Frequency response of the filter. Drag the dot: left/right is cutoff, up/down is resonance.
class FilterCurveView : public juce::Component, private juce::Timer
{
public:
    explicit FilterCurveView (juce::AudioProcessorValueTreeState& s)
        : hz_ (*s.getParameter ("f_hz"), [] (float) {}), q_ (*s.getParameter ("f_q"), [] (float) {}), apvts (s)
    {
        startTimerHz (20);
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (ink);
        g.fillRoundedRectangle (b, 6.0f);
        const bool on = P ("f_on") > 0.5f;
        auto a = plot();

        g.setColour (juce::Colour (0xff1c2025));
        for (double f : { 100.0, 1000.0, 10000.0 })
        {
            const float x = xOf (f);
            g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        }
        g.setColour (dim);
        g.setFont (mono (9.0f));
        g.drawText ("100", (int) xOf (100.0) + 2, (int) a.getBottom() - 11, 30, 11, juce::Justification::left);
        g.drawText ("1k", (int) xOf (1000.0) + 2, (int) a.getBottom() - 11, 30, 11, juce::Justification::left);
        g.drawText ("10k", (int) xOf (10000.0) + 2, (int) a.getBottom() - 11, 30, 11, juce::Justification::left);

        if (! on)
        {
            g.setColour (dim);
            g.setFont (mono (11.0f));
            g.drawText ("filter is off", getLocalBounds(), juce::Justification::centred);
            return;
        }

        juce::Path line_, fill;
        for (int i = 0; i <= 160; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, (double) i / 160.0);
            const float x = a.getX() + a.getWidth() * (float) i / 160.0f, y = yOf (response (f));
            if (i == 0) { line_.startNewSubPath (x, y); fill.startNewSubPath (x, a.getBottom()); fill.lineTo (x, y); }
            else { line_.lineTo (x, y); fill.lineTo (x, y); }
        }
        fill.lineTo (a.getRight(), a.getBottom());
        fill.closeSubPath();
        g.setColour (teal.withAlpha (0.14f));
        g.fillPath (fill);
        g.setColour (teal);
        g.strokePath (line_, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));

        const auto h = handle();
        g.setColour (orange.withAlpha (0.4f));
        g.drawVerticalLine ((int) h.x, a.getY(), a.getBottom());
        g.setColour (orange);
        g.fillEllipse (h.x - 5.0f, h.y - 5.0f, 10.0f, 10.0f);
        g.setColour (ink);
        g.drawEllipse (h.x - 5.0f, h.y - 5.0f, 10.0f, 10.0f, 1.5f);
    }

    void mouseDown (const juce::MouseEvent& e) override { dragging_ = P ("f_on") > 0.5f; if (dragging_) { hz_.beginGesture(); q_.beginGesture(); drag (e); } }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragging_) drag (e); }
    void mouseUp (const juce::MouseEvent&) override { if (dragging_) { hz_.endGesture(); q_.endGesture(); dragging_ = false; } }
    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        const auto& d = fw::paramTable()[(size_t) fw::paramIndex ("f_hz")];
        hz_.setValueAsCompleteGesture (d.def);
    }

private:
    void timerCallback() override { repaint(); }
    float P (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    juce::Rectangle<float> plot() const { return getLocalBounds().toFloat().reduced (6.0f, 8.0f); }
    float xOf (double f) const { auto a = plot(); return a.getX() + a.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); }
    float yOf (float db) const { auto a = plot(); return a.getBottom() - (juce::jlimit (-40.0f, 20.0f, db) + 40.0f) / 60.0f * a.getHeight(); }
    juce::Point<float> handle() const
    {
        const float f = P ("f_hz");
        return { xOf (f), yOf (response (f)) };
    }

    // magnitude in dB of the same biquad shape the engine uses
    float response (double f) const
    {
        const int type = (int) (P ("f_type") + 0.5f);
        const double fc = P ("f_hz"), qv = P ("f_q"), r = f / fc;
        double mag;
        if (type == 2)
        {
            const double q = std::max (0.05, qv);
            mag = (r / q) / std::sqrt ((1 - r * r) * (1 - r * r) + (r / q) * (r / q));
        }
        else
        {
            const double q = std::pow (10.0, qv / 20.0);
            const double num = type == 0 ? 1.0 : r * r;
            mag = num / std::sqrt ((1 - r * r) * (1 - r * r) + (r / q) * (r / q));
        }
        return (float) (20.0 * std::log10 (std::max (1e-6, mag)));
    }

    void drag (const juce::MouseEvent& e)
    {
        auto a = plot();
        const double fx = juce::jlimit (0.0, 1.0, (double) ((e.position.x - a.getX()) / a.getWidth()));
        hz_.setValueAsPartOfGesture ((float) juce::jlimit (30.0, 18000.0, 20.0 * std::pow (1000.0, fx)));
        const float yy = juce::jlimit (0.0f, 1.0f, (a.getBottom() - e.position.y) / a.getHeight());
        // vertical position maps resonance 0..20 dB (the handle sits on the peak, so this feels direct)
        q_.setValueAsPartOfGesture (juce::jlimit (0.0f, 20.0f, (yy - 0.55f) * 50.0f));
    }

    juce::ParameterAttachment hz_, q_;
    juce::AudioProcessorValueTreeState& apvts;
    bool dragging_ = false;
};

// ---------------------------------------------------------------------------------------------
// Pitch bend (springs back) and modulation wheel.
class WheelView : public juce::Component
{
public:
    WheelView (bool springs, const juce::String& name, juce::Colour colour) : springs_ (springs), name_ (name), colour_ (colour), value_ (springs ? 0.0f : 0.0f) {}

    std::function<void (float)> onChange;    // bend: -1..1, mod: 0..1
    float value() const { return value_; }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto body = b.withTrimmedBottom (14.0f);
        g.setColour (ink);
        g.fillRoundedRectangle (body, 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (body.reduced (0.5f), 6.0f, 1.0f);
        // ridges
        g.setColour (juce::Colour (0xff1c2025));
        for (float y = body.getY() + 6.0f; y < body.getBottom() - 4.0f; y += 6.0f) g.drawHorizontalLine ((int) y, body.getX() + 4.0f, body.getRight() - 4.0f);
        const float pos = springs_ ? (value_ + 1.0f) * 0.5f : value_;
        const float cy = body.getBottom() - 8.0f - pos * (body.getHeight() - 16.0f);
        auto cap = juce::Rectangle<float> (body.getX() + 3.0f, cy - 6.0f, body.getWidth() - 6.0f, 12.0f);
        g.setColour (colour_);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (ink.withAlpha (0.5f));
        g.drawHorizontalLine ((int) cy, cap.getX() + 3.0f, cap.getRight() - 3.0f);
        g.setColour (dim);
        g.setFont (mono (9.0f));
        g.drawText (name_, b.removeFromBottom (13.0f).toNearestInt(), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& e) override { set (e); }
    void mouseDrag (const juce::MouseEvent& e) override { set (e); }
    void mouseUp (const juce::MouseEvent&) override { if (springs_) { value_ = 0.0f; push(); } }

private:
    void set (const juce::MouseEvent& e)
    {
        const float h = (float) getHeight() - 14.0f - 16.0f;
        const float pos = juce::jlimit (0.0f, 1.0f, ((float) getHeight() - 14.0f - 8.0f - e.position.y) / h);
        value_ = springs_ ? pos * 2.0f - 1.0f : pos;
        if (springs_ && std::fabs (value_) < 0.04f) value_ = 0.0f;
        push();
    }
    void push() { repaint(); if (onChange) onChange (value_); }

    bool springs_;
    juce::String name_;
    juce::Colour colour_;
    float value_;
};

// ---------------------------------------------------------------------------------------------
// Searchable preset browser: category chips, text search, favourites, and a scrolling list.
class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::TextEditor::Listener
{
public:
    explicit PresetBrowser (PresetStore& store) : store_ (store)
    {
        search_.setTextToShowWhenEmpty ("Search sounds", dim);
        search_.setFont (mono (12.0f));
        search_.addListener (this);
        search_.setIndents (26, 6);
        addAndMakeVisible (search_);
        list_.setModel (this);
        list_.setRowHeight (30);
        list_.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list_.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (list_);
        loadFavourites();
        rebuildChips();
        refilter();
    }

    std::function<void (int)> onPick;            // flat index into PresetStore
    std::function<void()> onSave, onInit;

    void setCurrent (int flat)
    {
        if (flat == current_) return;
        current_ = flat;
        for (int r = 0; r < (int) rows_.size(); ++r)
            if (rows_[(size_t) r] == flat) { list_.scrollToEnsureRowIsOnscreen (r); break; }
        list_.repaint();
    }

    void reload() { rebuildChips(); refilter(); }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        search_.setBounds (r.removeFromTop (32));
        r.removeFromTop (8);
        chipArea_ = r.removeFromTop (chipRows_ * 27);
        layoutChips();
        r.removeFromTop (4);
        auto foot = r.removeFromBottom (32);
        r.removeFromBottom (8);
        saveBtn_.setBounds (foot.removeFromLeft (foot.getWidth() - 76));
        foot.removeFromLeft (8);
        initBtn_.setBounds (foot);
        list_.setBounds (r);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xff111317));
        g.setColour (line);
        g.drawVerticalLine (getWidth() - 1, 0.0f, (float) getHeight());
        auto s = search_.getBounds().toFloat();
        g.setColour (dim);
        g.drawEllipse (s.getX() + 10.0f, s.getCentreY() - 6.0f, 9.0f, 9.0f, 1.4f);
        g.drawLine (s.getX() + 17.5f, s.getCentreY() + 2.0f, s.getX() + 21.0f, s.getCentreY() + 5.5f, 1.4f);
        g.setFont (mono (10.0f));
        g.drawText (juce::String ((int) rows_.size()) + " of " + juce::String (store_.count()), getLocalBounds().reduced (14, 0).withTop (getHeight() - 76).withHeight (12), juce::Justification::centredRight);
    }

    bool keyPressed (const juce::KeyPress& k) override
    {
        if (k == juce::KeyPress::downKey) { step (1); return true; }
        if (k == juce::KeyPress::upKey)   { step (-1); return true; }
        return false;
    }

private:
    void step (int d)
    {
        if (rows_.empty()) return;
        int at = 0;
        for (int r = 0; r < (int) rows_.size(); ++r) if (rows_[(size_t) r] == current_) at = r;
        at = juce::jlimit (0, (int) rows_.size() - 1, at + d);
        list_.scrollToEnsureRowIsOnscreen (at);
        if (onPick) onPick (rows_[(size_t) at]);
    }

    void textEditorTextChanged (juce::TextEditor&) override { refilter(); }

    void rebuildChips()
    {
        chips_.clear();
        names_.clear();
        names_.add ("All");
        names_.add ("Favourites");
        for (int i = 0; i < store_.count(); ++i)
        {
            const auto c = store_.at (i).category;
            if (c.isNotEmpty() && ! names_.contains (c)) names_.add (c);
        }
        for (int i = 0; i < names_.size(); ++i)
        {
            auto b = std::make_unique<juce::TextButton> (names_[i]);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (4242);
            b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b->setColour (juce::TextButton::textColourOffId, juce::Colour (0xffc3c8cf));
            b->onClick = [this, i] { category_ = i; refilter(); };
            if (i == category_) b->setToggleState (true, juce::dontSendNotification);
            addAndMakeVisible (*b);
            chips_.push_back (std::move (b));
        }
        saveBtn_.setButtonText ("SAVE AS...");
        initBtn_.setButtonText ("INIT");
        saveBtn_.onClick = [this] { if (onSave) onSave(); };
        initBtn_.onClick = [this] { if (onInit) onInit(); };
        addAndMakeVisible (saveBtn_);
        addAndMakeVisible (initBtn_);
        resized();
    }

    void layoutChips()
    {
        int x = chipArea_.getX(), y = chipArea_.getY(), rows = 1;
        for (auto& c : chips_)
        {
            const int w = textWidth (mono (10.5f), c->getButtonText()) + 20;
            if (x + w > chipArea_.getRight()) { x = chipArea_.getX(); y += 27; ++rows; }
            c->setBounds (x, y, w, 22);
            x += w + 5;
        }
        if (rows != chipRows_) { chipRows_ = rows; resized(); }
    }

    void refilter()
    {
        rows_.clear();
        const auto q = search_.getText().trim().toLowerCase();
        const juce::String cat = category_ >= 0 && category_ < names_.size() ? names_[category_] : juce::String ("All");
        for (int i = 0; i < store_.count(); ++i)
        {
            const auto& e = store_.at (i);
            if (cat == "Favourites" && ! isFav (i)) continue;
            if (cat != "All" && cat != "Favourites" && e.category != cat) continue;
            if (q.isNotEmpty() && ! (e.name.toLowerCase().contains (q) || e.category.toLowerCase().contains (q))) continue;
            rows_.push_back (i);
        }
        list_.updateContent();
        list_.repaint();
        repaint();
    }

    // ---- favourites: stored by name in Documents/Formwave/favourites.txt ----
    static juce::File favFile() { return PresetStore::folder().getParentDirectory().getChildFile ("favourites.txt"); }
    void loadFavourites() { favs_.clear(); favs_.addLines (favFile().loadFileAsString()); favs_.removeEmptyStrings(); }
    void saveFavourites() { favFile().getParentDirectory().createDirectory(); favFile().replaceWithText (favs_.joinIntoString ("\n")); }
    bool isFav (int flat) const { return favs_.contains (store_.at (flat).name); }
    void toggleFav (int flat)
    {
        const auto n = store_.at (flat).name;
        if (favs_.contains (n)) favs_.removeString (n); else favs_.add (n);
        saveFavourites();
        refilter();
    }

    // ---- ListBoxModel ----
    int getNumRows() override { return (int) rows_.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool) override
    {
        if (row < 0 || row >= (int) rows_.size()) return;
        const int flat = rows_[(size_t) row];
        const auto& e = store_.at (flat);
        const bool sel = flat == current_;
        auto r = juce::Rectangle<float> (0.0f, 1.0f, (float) w, (float) h - 2.0f);
        if (sel) { g.setColour (teal.withAlpha (0.14f)); g.fillRoundedRectangle (r, 5.0f); g.setColour (teal); g.fillRect (0.0f, 4.0f, 3.0f, (float) h - 8.0f); }
        g.setColour (sel ? juce::Colours::white : juce::Colour (0xffd4d1ca));
        g.setFont (sans (14.0f, sel));
        g.drawText (e.name, 12, 0, w - 100, h, juce::Justification::centredLeft);
        g.setColour (dim);
        g.setFont (mono (9.5f));
        g.drawText (e.category.toUpperCase().substring (0, 7), w - 84, 0, 56, h, juce::Justification::centredRight);
        const bool fav = isFav (flat);
        g.setColour (fav ? orange : juce::Colour (0xff3a3e45));
        juce::Path star;
        star.addStar ({ (float) w - 14.0f, (float) h * 0.5f }, 5, 3.0f, 6.5f);
        g.fillPath (star);
    }
    void listBoxItemClicked (int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int) rows_.size()) return;
        if (e.x > list_.getWidth() - 34) { toggleFav (rows_[(size_t) row]); return; }
        if (onPick) onPick (rows_[(size_t) row]);
    }
    void selectedRowsChanged (int) override {}

    PresetStore& store_;
    juce::TextEditor search_;
    juce::ListBox list_;
    juce::TextButton saveBtn_, initBtn_;
    std::vector<std::unique_ptr<juce::TextButton>> chips_;
    juce::StringArray names_, favs_;
    std::vector<int> rows_;
    juce::Rectangle<int> chipArea_;
    int category_ = 0, current_ = -1, chipRows_ = 2;
};

// ---------------------------------------------------------------------------------------------
// A panel with a row of tab buttons; shows one child at a time.
class TabbedDisplay : public juce::Component
{
public:
    void addTab (const juce::String& name, juce::Component* c)
    {
        auto b = std::make_unique<juce::TextButton> (name);
        b->setClickingTogglesState (true);
        b->setRadioGroupId (909);
        b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour (juce::TextButton::buttonOnColourId, teal.withAlpha (0.18f));
        b->setColour (juce::TextButton::textColourOffId, dim);
        b->setColour (juce::TextButton::textColourOnId, teal);
        const int idx = (int) tabs_.size();
        b->onClick = [this, idx] { show (idx); };
        addAndMakeVisible (*b);
        addChildComponent (c);
        tabs_.push_back ({ std::move (b), c });
        if (idx == 0) { tabs_[0].btn->setToggleState (true, juce::dontSendNotification); c->setVisible (true); }
    }

    void show (int i)
    {
        for (size_t k = 0; k < tabs_.size(); ++k) tabs_[k].comp->setVisible ((int) k == i);
        repaint();
    }

    void resized() override
    {
        int x = 10;
        for (auto& t : tabs_)
        {
            const int w = textWidth (mono (11.0f), t.btn->getButtonText()) + 22;
            t.btn->setBounds (x, 6, w, 26);
            x += w + 4;
        }
        for (auto& t : tabs_) t.comp->setBounds (getLocalBounds().withTrimmedTop (38));
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.drawHorizontalLine (38, 0.0f, (float) getWidth());
    }

private:
    struct Tab { std::unique_ptr<juce::TextButton> btn; juce::Component* comp; };
    std::vector<Tab> tabs_;
};
} // namespace fwui
