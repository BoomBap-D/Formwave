#include "Editor.h"

using namespace fwui;

FormwaveEditor::FormwaveEditor (FormwaveProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);
    content.setSize (kW, kH);

    // ---- header ----
    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &saveBtn, &initBtn }) content.addAndMakeVisible (*b);
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    saveBtn.onClick = [this] { askSavePreset(); };
    initBtn.onClick = [this]
    {
        proc.applyPatch (fw::Patch::init());
        proc.setPresetIndex (-1);
        updatePresetLabel();
    };
    knob ("level", "LEVEL");

    // ---- visual row ----
    harm = add<HarmonicView> (proc.apvts);
    scope = add<ScopeView>();
    spectrum = add<SpectrumView>();

    // ---- oscillators ----
    for (int i = 0; i < 4; ++i)
    {
        const juce::String p = "o" + juce::String (i + 1) + "_";
        add<ParamCombo> (proc.apvts, p + "wave");
        knob ((p + "ratio").toRawUTF8(), "RATIO");
        knob ((p + "cents").toRawUTF8(), "FINE");
        knob ((p + "level").toRawUTF8(), "LEVEL");
        knob ((p + "decay").toRawUTF8(), "DECAY");
    }
    // ---- pitch / FM / noise ----
    knob ("pe_st", "PITCH ENV"); knob ("pe_t", "TIME");
    knob ("fm_r", "FM RATIO"); knob ("fm_i", "FM AMT"); knob ("fm_d", "FM DECAY");
    add<ParamCombo> (proc.apvts, "n_kind");
    add<ParamCombo> (proc.apvts, "n_ft");
    knob ("n_lvl", "LEVEL"); knob ("n_hz", "CUTOFF"); knob ("n_q", "RES"); knob ("n_att", "ATTACK"); knob ("n_dec", "DECAY");
    knob ("n_bursts", "BURSTS"); knob ("n_gap", "GAP"); knob ("n_sweep", "SWEEP TO"); knob ("n_sweept", "SWEEP T");
    // ---- filter / amp ----
    add<ParamToggle> (proc.apvts, "f_on", "FILTER");
    add<ParamCombo> (proc.apvts, "f_type");
    knob ("f_hz", "CUTOFF"); knob ("f_q", "RES"); knob ("f_env", "ENV"); knob ("f_envd", "ENV DEC"); knob ("f_kt", "KEYTRK");
    knob ("f_lfo", "LFO RATE"); knob ("f_lfod", "LFO DEPTH"); knob ("drive", "DRIVE");
    env = add<EnvView> (proc.apvts);
    knob ("amp_a", "ATTACK"); knob ("amp_d", "DECAY"); knob ("amp_s", "SUSTAIN"); knob ("amp_r", "RELEASE"); knob ("len", "LENGTH");
    add<ParamToggle> (proc.apvts, "rise", "RISE");
    // ---- voice and fx ----
    add<ParamCombo> (proc.apvts, "mode");
    add<ParamToggle> (proc.apvts, "mono", "MONO");
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
    setLookAndFeel (nullptr);
}

void FormwaveEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, kW, kH);
}

// All positions below are in the 1180 x 760 design space; the whole editor scales to the window.
void FormwaveEditor::layout()
{
    panels.clear();
    const int kh = 66;
    auto place = [&] (const char* id, int x, int y, int w = 62, int h = 66) { if (auto* k = knobs[id]) k->setBounds (x, y, w, h); };
    auto comp = [&] (juce::Component* c, int x, int y, int w, int h) { c->setBounds (x, y, w, h); };

    // header
    comp (&prevBtn, 360, 14, 28, 28);
    comp (&presetBtn, 392, 14, 330, 28);
    comp (&nextBtn, 726, 14, 28, 28);
    comp (&saveBtn, 764, 14, 60, 28);
    comp (&initBtn, 828, 14, 52, 28);
    place ("level", 1030, -2, 62, 56);
    meterArea = { 1110, 12, 58, 30 };

    // visuals
    panels.push_back ({ { 12, 60, 420, 192 }, {} });
    panels.push_back ({ { 440, 60, 360, 192 }, {} });
    panels.push_back ({ { 808, 60, 360, 192 }, {} });
    comp (harm, 12, 60, 420, 192);
    comp (scope, 440, 60, 360, 192);
    comp (spectrum, 808, 60, 360, 192);

    // oscillators
    const int ry = 260, rh = 290;
    panels.push_back ({ { 12, ry, 480, rh }, "OSCILLATORS" });
    for (int i = 0; i < 4; ++i)
    {
        const int y = ry + 26 + i * 65;
        // wave combos are the first four ParamCombo instances
        int seen = 0;
        for (auto& c : owned)
            if (auto* cb = dynamic_cast<ParamCombo*> (c.get())) { if (seen++ == i) { comp (cb, 24, y + 22, 96, 24); break; } }
        const juce::String p = "o" + juce::String (i + 1) + "_";
        place ((p + "ratio").toRawUTF8(), 132, y); place ((p + "cents").toRawUTF8(), 198, y);
        place ((p + "level").toRawUTF8(), 264, y); place ((p + "decay").toRawUTF8(), 330, y);
    }

    // pitch, FM, noise
    panels.push_back ({ { 500, ry, 330, rh }, "PITCH  ·  FM  ·  NOISE" });
    place ("pe_st", 508, ry + 22); place ("pe_t", 572, ry + 22);
    place ("fm_r", 636, ry + 22); place ("fm_i", 700, ry + 22); place ("fm_d", 764, ry + 22);
    // noise: combos then two knob rows
    int combos = 0;
    for (auto& c : owned)
        if (auto* cb = dynamic_cast<ParamCombo*> (c.get()))
        {
            ++combos;
            if (combos == 5) comp (cb, 512, ry + 100, 150, 24);     // noise type
            if (combos == 6) comp (cb, 668, ry + 100, 150, 24);     // noise filter
        }
    place ("n_lvl", 508, ry + 130); place ("n_hz", 572, ry + 130); place ("n_q", 636, ry + 130); place ("n_att", 700, ry + 130); place ("n_dec", 764, ry + 130);
    place ("n_bursts", 508, ry + 202); place ("n_gap", 572, ry + 202); place ("n_sweep", 636, ry + 202); place ("n_sweept", 700, ry + 202);

    // filter and amp
    panels.push_back ({ { 838, ry, 330, rh }, "FILTER  ·  AMP" });
    {
        int seen = 0;
        for (auto& c : owned)
        {
            if (auto* cb = dynamic_cast<ParamCombo*> (c.get())) { if (++seen == 7) comp (cb, 920, ry + 24, 110, 24); }
            if (auto* tb = dynamic_cast<ParamToggle*> (c.get())) { if (tb->getButtonText() == "FILTER") comp (tb, 846, ry + 24, 66, 24); }
        }
    }
    place ("f_hz", 846, ry + 54); place ("f_q", 910, ry + 54); place ("f_env", 974, ry + 54); place ("f_envd", 1038, ry + 54); place ("f_kt", 1102, ry + 54);
    place ("f_lfo", 846, ry + 122); place ("f_lfod", 910, ry + 122); place ("drive", 974, ry + 122);
    comp (env, 1042, ry + 130, 118, 52);
    place ("amp_a", 846, ry + 208); place ("amp_d", 910, ry + 208); place ("amp_s", 974, ry + 208); place ("amp_r", 1038, ry + 208); place ("len", 1102, ry + 208);
    for (auto& c : owned)
        if (auto* tb = dynamic_cast<ParamToggle*> (c.get()))
            if (tb->getButtonText() == "RISE") comp (tb, 1110, ry + 188, 50, 20);

    // voice + fx strip
    const int fy = 558, fh = 92;
    panels.push_back ({ { 12, fy, 1156, fh }, {} });
    int combo8 = 0;
    for (auto& c : owned)
    {
        if (auto* cb = dynamic_cast<ParamCombo*> (c.get())) { if (++combo8 == 8) comp (cb, 24, fy + 26, 90, 24); }
        if (auto* tb = dynamic_cast<ParamToggle*> (c.get())) { if (tb->getButtonText() == "MONO") comp (tb, 24, fy + 56, 90, 24); }
    }
    const int sx = 124, sw = 60;
    const char* vids[6] { "base", "oct", "glide", "vib_hz", "vib_dep", "vib_dly" };
    for (int i = 0; i < 6; ++i) place (vids[i], sx + i * sw, fy + 14, sw, kh);
    const char* fx[9] { "ch_mix", "ch_rate", "ch_dep", "dl_time", "dl_fb", "dl_mix", "rv_size", "rv_damp", "rv_mix" };
    const int fx0 = sx + 6 * sw + 24;
    for (int i = 0; i < 9; ++i) place (fx[i], fx0 + i * sw + (i / 3) * 18, fy + 14, sw, kh);

    comp (&keyboard, 12, 658, 1156, 92);
}

void FormwaveEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (bg);
    // header
    g.setColour (panel);
    g.fillRect (0, 0, kW, 54);
    g.setColour (line);
    g.drawHorizontalLine (54, 0.0f, (float) kW);
    g.setColour (teal);
    juce::Path w;
    for (int i = 0; i <= 40; ++i)
    {
        const float x = 18.0f + (float) i, y = 27.0f - std::sin ((float) i / 40.0f * 6.2832f * 2.0f) * (14.0f * (0.35f + 0.65f * (float) i / 40.0f));
        i == 0 ? w.startNewSubPath (x, y) : w.lineTo (x, y);
    }
    g.strokePath (w, juce::PathStrokeType (2.2f));
    g.setColour (text);
    g.setFont (sans (22.0f, true));
    g.drawText ("FORMWAVE", 70, 10, 200, 32, juce::Justification::left);
    g.setColour (dim);
    g.setFont (mono (10.0f));
    g.drawText ("SOUND DESIGN SYNTH", 232, 16, 120, 20, juce::Justification::left);

    for (const auto& p : panels)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (p.r.toFloat(), 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (p.r.toFloat().reduced (0.5f), 6.0f, 1.0f);
        if (p.title.isNotEmpty())
        {
            g.setColour (dim);
            g.setFont (mono (10.5f));
            g.drawText (p.title, p.r.getX() + 12, p.r.getY() + 6, p.r.getWidth() - 24, 16, juce::Justification::left);
        }
    }
    // group labels on the voice and fx strip
    g.setColour (dim);
    g.setFont (mono (9.5f));
    const int fy = 558, sx = 124, sw = 60, fx0 = sx + 6 * sw + 24;
    g.drawText ("VOICE", 24, fy + 4, 90, 12, juce::Justification::left);
    g.drawText ("PITCH  ·  VIBRATO", sx, fy + 2, 6 * sw, 12, juce::Justification::centred);
    g.drawText ("CHORUS", fx0, fy + 2, 3 * sw, 12, juce::Justification::centred);
    g.drawText ("DELAY", fx0 + 3 * sw + 18, fy + 2, 3 * sw, 12, juce::Justification::centred);
    g.drawText ("REVERB", fx0 + 6 * sw + 36, fy + 2, 3 * sw, 12, juce::Justification::centred);

    // meter
    const float peak = juce::jlimit (0.0f, 1.0f, proc.outputPeak.load());
    for (int i = 0; i < 8; ++i)
    {
        const bool lit = peak > (float) i / 8.0f;
        g.setColour (lit ? (i >= 6 ? orange : teal) : juce::Colour (0xff2a2d33));
        const int h = 6 + i * 3;
        g.fillRect (meterArea.getX() + i * 7, meterArea.getBottom() - h, 5, h);
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
    presetBtn.setButtonText (proc.presetName() + (modified() ? "  *" : "") + "   ·   " + cat);
}

void FormwaveEditor::choosePreset (int i)
{
    if (i < 0 || i >= proc.presets.count()) return;
    proc.applyPatch (proc.presets.at (i).patch);
    proc.setPresetIndex (i);
    updatePresetLabel();
}

void FormwaveEditor::stepPreset (int delta)
{
    const int n = proc.presets.count();
    if (n == 0) return;
    const int cur = proc.presetIndex() < 0 ? (delta > 0 ? -1 : 0) : proc.presetIndex();
    choosePreset ((cur + delta + n) % n);
}

void FormwaveEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    const auto& f = proc.presets.factory();
    std::vector<std::string> cats;
    for (const auto& e : f) { const std::string c = e.category.toStdString(); if (std::find (cats.begin(), cats.end(), c) == cats.end()) cats.push_back (c); }
    for (const auto& c : cats)
    {
        juce::PopupMenu sub;
        for (size_t i = 0; i < f.size(); ++i)
            if (f[i].category.toStdString() == c) sub.addItem ((int) i + 1, f[i].name, true, (int) i == proc.presetIndex());
        menu.addSubMenu (juce::String (c), sub);
    }
    const auto& u = proc.presets.user();
    if (! u.empty())
    {
        juce::PopupMenu sub;
        for (size_t i = 0; i < u.size(); ++i) sub.addItem ((int) (f.size() + i) + 1, u[i].name, true, (int) (f.size() + i) == proc.presetIndex());
        menu.addSeparator();
        menu.addSubMenu ("User", sub);
    }
    menu.addSeparator();
    menu.addItem (100000, "Save preset...");
    menu.addItem (100001, "Rescan user presets");
    menu.addItem (100002, "Open preset folder");

    juce::Component::SafePointer<FormwaveEditor> self (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [self] (int r)
    {
        if (self == nullptr || r == 0) return;
        if (r == 100000) self->askSavePreset();
        else if (r == 100001) { self->proc.presets.reloadUser(); }
        else if (r == 100002) { PresetStore::folder().createDirectory(); PresetStore::folder().revealToUser(); }
        else self->choosePreset (r - 1);
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

    // refresh the "shape of the sound" picture when any parameter changed
    int h = 0;
    for (int i = 0; i < fw::P_COUNT; ++i)
        h = h * 31 + (int) (proc.apvts.getRawParameterValue (fw::paramTable()[(size_t) i].id)->load() * 1000.0f);
    if (h != lastHash) { lastHash = h; previewCountdown = 4; updatePresetLabel(); }
    if (previewCountdown > 0 && --previewCountdown == 0)
        scope->setPreview (fw::renderPatch (proc.currentPatch(), 60, 1.0f, 0.6f), 44100.0);
}
