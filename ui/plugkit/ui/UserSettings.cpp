// Copied from eq1's plugin/UserSettings.cpp (eq1 at dbb1127), in namespace plugkit::ui, with the product name where
// eq1's has "eq1".

#include "UserSettings.h"

#include "UiScale.h"

namespace plugkit::ui
{

namespace
{
const juce::String uiScaleKey { "uiScalePercent" };
}

UserSettings::UserSettings (const juce::String& name, juce::File f)
    : productName (name), file (std::move (f)), lock (name + "-user-settings")
{
}

juce::File UserSettings::defaultFile (const juce::String& productName)
{
    auto folder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    folder = folder.getChildFile ("Application Support");
#endif
    return folder.getChildFile (productName).getChildFile (productName + ".settings");
}

juce::PropertiesFile::Options UserSettings::options() const
{
    juce::PropertiesFile::Options o;
    o.storageFormat = juce::PropertiesFile::storeAsXML;
    o.millisecondsBeforeSaving = -1; // saved at once, below
    o.processLock = &lock;
    return o;
}

int UserSettings::uiScalePercent() const
{
    if (file == juce::File())
        return uiScale::defaultPercent;
    const juce::PropertiesFile settings (file, options());
    const int percent = settings.getIntValue (uiScaleKey, uiScale::defaultPercent);
    return uiScale::isOffered (percent) ? percent : uiScale::defaultPercent;
}

bool UserSettings::setUiScalePercent (int percent)
{
    if (file == juce::File() || ! uiScale::isOffered (percent))
        return false;
    juce::PropertiesFile settings (file, options());
    settings.setValue (uiScaleKey, percent);
    if (settings.save())
        return true;
    juce::Logger::writeToLog (productName + ": couldn't save the user settings to " + file.getFullPathName()
                              + "; the UI Scale will be saved again on the next pick");
    return false;
}

} // namespace plugkit::ui
