#include "Processor.h"
#include "../UI/Editor.h"

using namespace fw;

juce::AudioProcessorValueTreeState::ParameterLayout FormwaveProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : paramTable())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.type == PT_CHOICE)
        {
            juce::StringArray names;
            for (const auto& c : d.choices) names.add (c);
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, names, (int) d.def));
        }
        else if (d.type == PT_BOOL)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid, d.name,
                juce::NormalisableRange<float> (d.min, d.max, (d.id == "oct" || d.id == "n_bursts") ? 1.0f : 0.0f, d.skew), d.def,
                juce::AudioParameterFloatAttributes().withLabel (d.unit)));
    }
    return layout;
}

FormwaveProcessor::FormwaveProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    const auto& t = paramTable();
    for (size_t i = 0; i < t.size(); ++i)
    {
        raw_[i] = apvts.getRawParameterValue (t[i].id);
        params_[i] = apvts.getParameter (t[i].id);
    }
    engine_.prepare (44100.0);
}

bool FormwaveProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FormwaveProcessor::prepareToPlay (double sr, int)
{
    engine_.prepare (sr);
}

void FormwaveProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    float values[P_COUNT];
    for (int i = 0; i < P_COUNT; ++i) values[i] = raw_[(size_t) i]->load();
    engine_.setPatch (values);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : left;

    {
        const float ub = uiBend.load(), um = uiMod.load();
        if (ub != lastUiBend_) { lastUiBend_ = ub; engine_.setBend (ub * 2.0f); }
        if (um != lastUiMod_)  { lastUiMod_ = um;  engine_.setModWheel (um); }
    }

    int pos = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { engine_.process (left + pos, right + pos, t - pos); pos = t; }
        if (m.isNoteOn())            engine_.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())      engine_.noteOff (m.getNoteNumber());
        else if (m.isPitchWheel())   engine_.setBend (((float) m.getPitchWheelValue() - 8192.0f) / 8192.0f * 2.0f);
        else if (m.isControllerOfType (1))  engine_.setModWheel ((float) m.getControllerValue() / 127.0f);
        else if (m.isControllerOfType (64)) engine_.setSustain (m.getControllerValue() >= 64);
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine_.allNotesOff();
    }
    if (pos < n) engine_.process (left + pos, right + pos, n - pos);

    if (buffer.getNumChannels() < 2) {}
    tap.push (left, right, n);
    outputPeak.store (buffer.getMagnitude (0, n));
}

void FormwaveProcessor::applyPatch (const Patch& p)
{
    for (int i = 0; i < P_COUNT; ++i)
    {
        auto* prm = params_[(size_t) i];
        if (prm == nullptr) continue;
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (p.v[(size_t) i]));
        prm->endChangeGesture();
    }
    presetName_ = juce::String (p.name);
}

Patch FormwaveProcessor::currentPatch() const
{
    Patch p;
    for (int i = 0; i < P_COUNT; ++i) p.v[(size_t) i] = raw_[(size_t) i]->load();
    p.name = presetName_.toStdString();
    return p;
}

void FormwaveProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", presetName_, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void FormwaveProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType().toString()))
        {
            const auto vt = juce::ValueTree::fromXml (*xml);
            presetName_ = vt.getProperty ("preset", "Init").toString();
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* FormwaveProcessor::createEditor() { return new FormwaveEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FormwaveProcessor(); }
