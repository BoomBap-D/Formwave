#pragma once
#include <JuceHeader.h>

#include "../Core/Bank.h"

// Factory presets plus the user's own, saved as small text files in Documents/Formwave/Presets.
class PresetStore
{
public:
    struct Entry { juce::String name, category; fw::Patch patch; bool factory = true; juce::File file; };

    PresetStore()
    {
        for (const auto& p : fw::factoryBank())
            factory_.push_back ({ p.name, p.category, p, true, {} });
        reloadUser();
    }

    static juce::File folder()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Formwave").getChildFile ("Presets");
    }

    void reloadUser()
    {
        user_.clear();
        const auto dir = folder();
        if (! dir.isDirectory()) return;
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.fwp"))
        {
            fw::Patch p;
            if (fw::patchFromText (f.loadFileAsString().toStdString(), p))
                user_.push_back ({ juce::String (p.name), juce::String (p.category), p, false, f });
        }
        std::sort (user_.begin(), user_.end(), [] (const Entry& a, const Entry& b) { return a.name.compareIgnoreCase (b.name) < 0; });
    }

    bool save (const juce::String& name, fw::Patch p)
    {
        const auto clean = juce::File::createLegalFileName (name.trim());
        if (clean.isEmpty()) return false;
        p.name = clean.toStdString(); p.category = "User";
        const auto dir = folder();
        if (! dir.createDirectory().wasOk()) return false;
        const bool ok = dir.getChildFile (clean + ".fwp").replaceWithText (juce::String (fw::patchToText (p)));
        reloadUser();
        return ok;
    }

    const std::vector<Entry>& factory() const { return factory_; }
    const std::vector<Entry>& user() const { return user_; }

    // flat index: factory first, then user
    int count() const { return (int) (factory_.size() + user_.size()); }
    const Entry& at (int i) const { return i < (int) factory_.size() ? factory_[(size_t) i] : user_[(size_t) (i - (int) factory_.size())]; }

private:
    std::vector<Entry> factory_, user_;
};
