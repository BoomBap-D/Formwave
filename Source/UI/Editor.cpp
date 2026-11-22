#include "Editor.h"

using namespace fwui;

FormwaveEditor::FormwaveEditor (FormwaveProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), browser (p.presets),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);
    content.setSize (kW, kH);

    // ---- top bar ----
    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &abBtn, &menuBtn }) content.addAndMakeVisible (*b);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    presetBtn.onClick = [this] { showMainMenu(); };
    abBtn.onClick = [this] { toggleAB(); };
    menuBtn.onClick = [this] { showMainMenu(); };
    knob ("level", "MASTER");

    // ---- browser, status bar, wheels ----
    content.addAndMakeVisible (browser);
    browser.onPick = [this] (int i) { choosePreset (i); };
    browser.onSave = [this] { askSavePreset(); };
    browser.onInit = [this]
    {
        proc.applyPatch (fw::Patch::init());
        proc.setPresetIndex (-1);
        updatePresetLabel();
    };
    content.addAndMakeVisible (hint);
    bendWheel.onChange = [this] (float v) { proc.uiBend.store (v); hint.setHint ("PITCH BEND", "drag the wheel - it springs back to centre"); };
    modWheel.onChange  = [this] (float v) { proc.uiMod.store (v);  hint.setHint ("MOD WHEEL", "adds vibrato (and cutoff, if set in the Modulation tab)"); };
    content.addAndMakeVisible (bendWheel);
    content.addAndMakeVisible (modWheel);

    // ---- displays ----
    content.addAndMakeVisible (tabs);
    harm = add<HarmonicView> (proc.apvts);
    scope = add<ScopeView>();
    spectrum = add<SpectrumView>();
    tabs.addTab ("Harmonics", harm);
    tabs.addTab ("Waveform", scope);
    tabs.addTab ("Spectrum", spectrum);
    modPanel = add<ModPanel> (proc.apvts, [this] (const juce::String& a, const juce::String& b) { hint.setHint (a, b); });
    tabs.addTab ("Modulation", modPanel);
    filterCurve = add<FilterCurveView> (proc.apvts);
    env = add<EnvView> (proc.apvts);

    // ---- oscillators ----
    for (int i = 0; i < 4; ++i)
    {
        const juce::String pre = "o" + juce::String (i + 1) + "_";
        waves[(size_t) i] = add<WaveSelector> (proc.apvts, pre + "wave");
        waves[(size_t) i]->onHint = [this, i] (const juce::String& n) { n.isEmpty() ? hint.setHint ({}, {}) : hint.setHint ("OSC " + juce::String (i + 1) + " WAVE", n); };
        knob ((pre + "ratio").toRawUTF8(), "RATIO");
        knob ((pre + "cents").toRawUTF8(), "FINE");
        knob ((pre + "level").toRawUTF8(), "LEVEL");
        knob ((pre + "decay").toRawUTF8(), "DECAY");
    }
    // ---- pitch / FM / noise ----
    knob ("pe_st", "PITCH ENV"); knob ("pe_t", "TIME");
    knob ("fm_r", "FM RATIO"); knob ("fm_i", "FM AMT"); knob ("fm_d", "FM DECAY");
    combo ("n_kind");
    combo ("n_ft");
    knob ("n_lvl", "LEVEL"); knob ("n_hz", "CUTOFF"); knob ("n_q", "RES"); knob ("n_att", "ATTACK"); knob ("n_dec", "DECAY");
    knob ("n_bursts", "BURSTS"); knob ("n_gap", "GAP"); knob ("n_sweep", "SWEEP TO"); knob ("n_sweept", "SWEEP T");
    // ---- filter / amp ----
    toggle ("f_on", "ON");
    combo ("f_type");
    knob ("f_hz", "CUTOFF"); knob ("f_q", "RES"); knob ("f_env", "ENV"); knob ("f_envd", "ENV DEC"); knob ("f_kt", "KEYTRK");
    knob ("f_lfo", "LFO RATE"); knob ("f_lfod", "LFO DEPTH"); knob ("drive", "DRIVE");
    knob ("amp_a", "ATTACK"); knob ("amp_d", "DECAY"); knob ("amp_s", "SUSTAIN"); knob ("amp_r", "RELEASE"); knob ("len", "LENGTH");
    toggle ("rise", "RISE");
    // ---- voice and fx ----
    combo ("mode");
    toggle ("mono", "MONO");
    knob ("base", "BASE HZ"); knob ("oct", "OCTAVE"); knob ("glide", "GLIDE");
    knob ("vib_hz", "VIB RATE"); knob ("vib_dep", "VIB DEPTH"); knob ("vib_dly", "VIB DELAY");
    knob ("ch_mix", "MIX"); knob ("ch_rate", "RATE"); knob ("ch_dep", "DEPTH");
    knob ("dl_time", "TIME"); knob ("dl_fb", "FEEDBACK"); knob ("dl_mix", "MIX");
    knob ("rv_size", "SIZE"); knob ("rv_damp", "DAMP"); knob ("rv_mix", "MIX");

    keyboard.setAvailableRange (24, 108);
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (24.0f);
    keyboard.setOctaveForMiddleC (4);
    keyboard.setKeyPressBaseOctave (5);   // computer keys A W S E D F ... play from middle C
    content.addAndMakeVisible (keyboard);

    layout();
    updatePresetLabel();

    setResizable (true, true);
    setResizeLimits (kW / 2, kH / 2, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / kH);
    setSize (kW, kH);
    startTimerHz (30);
}

FormwaveEditor::~FormwaveEditor()
{
    proc.uiBend.store (0.0f);
    proc.uiMod.store (0.0f);
    setLookAndFeel (nullptr);
}

void FormwaveEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, kW, kH);
}

// All positions are in the 1280 x 800 design space; the whole editor scales to the window.
void FormwaveEditor::layout()
{
    panels.clear();
    captions.clear();

    // ---- top bar ----
    presetBtn.setBounds (416, 14, 400, 30);
    prevBtn.setBounds (378, 14, 32, 30);
    nextBtn.setBounds (822, 14, 32, 30);
    abBtn.setBounds (866, 14, 44, 30);
    menuBtn.setBounds (1232, 14, 32, 30);
    knobs["level"]->setBounds (1176, 0, 52, 56);
    meterArea = { 1096, 14, 70, 28 };

    // ---- left: browser ----
    browser.setBounds (0, 56, 248, 616);

    // ---- display row ----
    tabs.setBounds (260, 68, 656, 244);
    displayRight = { 926, 68, 342, 244 };
    filterCurve->setBounds (938, 98, 318, 100);
    env->setBounds (938, 218, 318, 82);
    captions.push_back ({ 938, 78, "FILTER RESPONSE  -  drag the dot" });
    captions.push_back ({ 938, 202, "AMP ENVELOPE" });

    // ---- modules ----
    const int my = 322, mh = 246;
    panels.push_back ({ { 260, my, 424, mh }, "OSCILLATORS" });
    for (int i = 0; i < 4; ++i)
    {
        const int y = my + 22 + i * 55;
        waves[(size_t) i]->setBounds (298, y + 14, 156, 28);
        const juce::String pre = "o" + juce::String (i + 1) + "_";
        knobRow ({ (pre + "ratio").toRawUTF8(), (pre + "cents").toRawUTF8(), (pre + "level").toRawUTF8(), (pre + "decay").toRawUTF8() },
                 { 460, y, 216, 55 });
        captions.push_back ({ 274, y + 18, juce::String (i + 1) });
    }

    panels.push_back ({ { 694, my, 250, mh }, "PITCH  -  FM" });
    knobRow ({ "pe_st", "pe_t", "fm_r", "fm_i", "fm_d" }, { 700, my + 22, 238, 56 });
    captions.push_back ({ 706, my + 84, "NOISE" });
    combos["n_kind"]->setBounds (702, my + 100, 112, 22);
    combos["n_ft"]->setBounds (820, my + 100, 118, 22);
    knobRow ({ "n_lvl", "n_hz", "n_q", "n_att", "n_dec" }, { 700, my + 126, 238, 56 });
    knobRow ({ "n_bursts", "n_gap", "n_sweep", "n_sweept" }, { 700, my + 182, 190, 56 });

    panels.push_back ({ { 954, my, 314, mh }, "FILTER" });
    toggles["f_on"]->setBounds (1100, my + 4, 50, 18);
    combos["f_type"]->setBounds (1156, my + 3, 104, 20);
    knobRow ({ "f_hz", "f_q", "f_env", "f_envd", "f_kt" }, { 960, my + 26, 302, 56 });
    knobRow ({ "f_lfo", "f_lfod", "drive" }, { 960, my + 84, 180, 56 });
    captions.push_back ({ 966, my + 146, "AMP" });
    toggles["rise"]->setBounds (1206, my + 144, 54, 18);
    knobRow ({ "amp_a", "amp_d", "amp_s", "amp_r", "len" }, { 960, my + 164, 302, 56 });

    // ---- voice and effects strip ----
    const int fy = 578, fh = 82;
    panels.push_back ({ { 260, fy, 424, fh }, "VOICE" });
    combos["mode"]->setBounds (268, fy + 24, 88, 22);
    toggles["mono"]->setBounds (268, fy + 52, 88, 22);
    knobRow ({ "base", "oct", "glide", "vib_hz", "vib_dep", "vib_dly" }, { 364, fy + 18, 312, 60 });
    const int cw = 184;
    panels.push_back ({ { 694, fy, cw, fh }, "CHORUS" });
    knobRow ({ "ch_mix", "ch_rate", "ch_dep" }, { 700, fy + 18, cw - 12, 60 });
    panels.push_back ({ { 694 + cw + 10, fy, cw, fh }, "DELAY" });
    knobRow ({ "dl_time", "dl_fb", "dl_mix" }, { 700 + cw + 10, fy + 18, cw - 12, 60 });
    panels.push_back ({ { 694 + 2 * (cw + 10), fy, 1268 - (694 + 2 * (cw + 10)), fh }, "REVERB" });
    knobRow ({ "rv_size", "rv_damp", "rv_mix" }, { 700 + 2 * (cw + 10), fy + 18, 1268 - (700 + 2 * (cw + 10)) - 6, 60 });

    // ---- keyboard row ----
    bendWheel.setBounds (14, 680, 30, 88);
    modWheel.setBounds (50, 680, 30, 88);
    keyboard.setBounds (92, 680, 1176, 90);
    hint.setBounds (0, 776, kW, 24);
}

void FormwaveEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (bg);
    // top bar
    g.setColour (panel);
    g.fillRect (0, 0, kW, 56);
    g.setColour (line);
    g.drawHorizontalLine (56, 0.0f, (float) kW);
    g.setColour (teal);
    juce::Path w;
    for (int i = 0; i <= 40; ++i)
    {
        const float x = 18.0f + (float) i * 0.8f, y = 28.0f - std::sin ((float) i / 40.0f * 6.2832f * 2.0f) * (12.0f * (0.35f + 0.65f * (float) i / 40.0f));
        i == 0 ? w.startNewSubPath (x, y) : w.lineTo (x, y);
    }
    g.strokePath (w, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (text);
    g.setFont (sans (20.0f, true));
    g.drawText ("FORMWAVE", 60, 12, 160, 32, juce::Justification::left);
    g.setColour (dim);
    g.setFont (mono (9.5f));
    g.drawText ("SOUND DESIGN SYNTH", 60, 36, 160, 12, juce::Justification::left);

    // preset pill backdrop
    g.setColour (ink);
    g.fillRoundedRectangle (presetBtn.getBounds().toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff2f343b));
    g.drawRoundedRectangle (presetBtn.getBounds().toFloat().reduced (0.5f), 8.0f, 1.0f);

    // panels
    g.setColour (panel);
    g.fillRoundedRectangle (displayRight.toFloat(), 8.0f);
    g.setColour (line);
    g.drawRoundedRectangle (displayRight.toFloat().reduced (0.5f), 8.0f, 1.0f);
    for (const auto& p : panels)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (p.r.toFloat(), 8.0f);
        g.setColour (line);
        g.drawRoundedRectangle (p.r.toFloat().reduced (0.5f), 8.0f, 1.0f);
        if (p.title.isNotEmpty())
        {
            g.setColour (dim);
            g.setFont (mono (10.0f));
            g.drawText (p.title, p.r.getX() + 12, p.r.getY() + 5, p.r.getWidth() - 24, 14, juce::Justification::left);
        }
    }
    for (const auto& c : captions)
    {
        const bool badge = c.text.length() == 1;
        if (badge)
        {
            g.setColour (panel2);
            g.fillEllipse ((float) c.x - 3.0f, (float) c.y - 1.0f, 20.0f, 20.0f);
            g.setColour (text);
            g.setFont (mono (11.0f).boldened());
            g.drawText (c.text, c.x - 3, c.y - 1, 20, 20, juce::Justification::centred);
        }
        else
        {
            g.setColour (dim);
            g.setFont (mono (10.0f));
            g.drawText (c.text, c.x, c.y, 300, 14, juce::Justification::left);
        }
    }

    // meter
    const float peak = juce::jlimit (0.0f, 1.0f, proc.outputPeak.load());
    for (int i = 0; i < 10; ++i)
    {
        const bool lit = peak > (float) i / 10.0f;
        g.setColour (lit ? (i >= 8 ? orange : teal) : juce::Colour (0xff2a2d33));
        const int h = 6 + i * 2;
        g.fillRoundedRectangle ((float) meterArea.getX() + (float) i * 7.0f, (float) meterArea.getBottom() - (float) h, 5.0f, (float) h, 1.0f);
    }
}

bool FormwaveEditor::modified() const
{
    const int idx = proc.presetIndex();
    if (idx < 0 || idx >= proc.presets.count()) return false;
    const auto& ref = proc.presets.at (idx).patch;
    for (int i = 0; i < fw::P_COUNT; ++i)
        if (std::fabs (proc.apvts.getRawParameterValue (fw::paramTable()[(size_t) i].id)->load() - ref.v[(size_t) i]) > 1e-3f * std::max (1.0f, std::fabs (ref.v[(size_t) i])))
            return true;
    return false;
}

void FormwaveEditor::updatePresetLabel()
{
    const int idx = proc.presetIndex();
    juce::String cat = idx >= 0 && idx < proc.presets.count() ? proc.presets.at (idx).category : juce::String ("User");
    presetBtn.setButtonText (proc.presetName() + (modified() ? "  (edited)" : "") + "     " + cat.toUpperCase());
    browser.setCurrent (idx);
}

void FormwaveEditor::choosePreset (int i)
{
    if (i < 0 || i >= proc.presets.count()) return;
    proc.applyPatch (proc.presets.at (i).patch);
    proc.setPresetIndex (i);
    updatePresetLabel();
    abUsed = false;
}

void FormwaveEditor::stepPreset (int delta)
{
    const int n = proc.presets.count();
    if (n == 0) return;
    const int cur = proc.presetIndex() < 0 ? (delta > 0 ? -1 : 0) : proc.presetIndex();
    choosePreset ((cur + delta + n) % n);
}

void FormwaveEditor::toggleAB()
{
    abSlot[abActive] = proc.currentPatch();
    abActive ^= 1;
    if (! abUsed) { abSlot[abActive] = abSlot[abActive ^ 1]; abUsed = true; }
    proc.applyPatch (abSlot[abActive]);
    abBtn.setButtonText (abActive == 0 ? "A" : "B");
    hint.setHint ("A / B", abActive == 0 ? "slot A - press again to compare with slot B" : "slot B - make changes here, press again to hear A");
    updatePresetLabel();
}

void FormwaveEditor::showMainMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "Save preset...");
    menu.addItem (2, "Rescan user presets");
    menu.addItem (3, "Open preset folder");
    menu.addSeparator();
    menu.addItem (4, "Next preset");
    menu.addItem (5, "Previous preset");
    juce::Component::SafePointer<FormwaveEditor> self (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&menuBtn), [self] (int r)
    {
        if (self == nullptr || r == 0) return;
        if (r == 1) self->askSavePreset();
        else if (r == 2) { self->proc.presets.reloadUser(); self->browser.reload(); }
        else if (r == 3) { PresetStore::folder().createDirectory(); PresetStore::folder().revealToUser(); }
        else if (r == 4) self->stepPreset (+1);
        else if (r == 5) self->stepPreset (-1);
    });
}

void FormwaveEditor::askSavePreset()
{
    auto* aw = new juce::AlertWindow ("Save preset", "Name this sound:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", proc.presetName() == "Init" ? juce::String ("My Sound") : proc.presetName());
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<FormwaveEditor> self (this);
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([self, aw] (int r)
    {
        if (self == nullptr || r != 1) return;
        const auto name = aw->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        auto patch = self->proc.currentPatch();
        if (self->proc.presets.save (name, patch))
        {
            self->proc.setPresetName (name);
            const auto& f = self->proc.presets.factory();
            const auto& u = self->proc.presets.user();
            for (size_t i = 0; i < u.size(); ++i) if (u[i].name == name) self->proc.setPresetIndex ((int) (f.size() + i));
            self->browser.reload();
            self->updatePresetLabel();
        }
    }), true);
}

void FormwaveEditor::timerCallback()
{
    proc.tap.pull();
    scope->setWindow (proc.tap.window(), AudioTap::kWindow);
    spectrum->update (proc.tap.window(), proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0);
    content.repaint (meterArea);
    hint.setRight (juce::String (proc.getSampleRate() > 0 ? (int) proc.getSampleRate() : 44100) + " Hz   -   " + juce::String (proc.presets.count()) + " presets");

    // refresh the "shape of the sound" picture when any parameter changed
    int h = 0;
    for (int i = 0; i < fw::P_COUNT; ++i)
        h = h * 31 + (int) (proc.apvts.getRawParameterValue (fw::paramTable()[(size_t) i].id)->load() * 1000.0f);
    if (h != lastHash) { lastHash = h; previewCountdown = 4; updatePresetLabel(); }
    if (previewCountdown > 0 && --previewCountdown == 0)
        scope->setPreview (fw::renderPatch (proc.currentPatch(), 60, 1.0f, 0.6f), 44100.0);
}
