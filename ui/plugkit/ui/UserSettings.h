#pragma once

// Copied from eq1's plugin/UserSettings.h (eq1 at dbb1127), in namespace plugkit::ui, taking the product name for its
// default file's folder and its lock's name, where eq1's are "eq1" and "eq1-user-settings".

#include <juce_data_structures/juce_data_structures.h>

namespace plugkit::ui
{

// What the plugin keeps per user, across instances, formats and hosts, in one settings file: for now, the
// UI Scale last picked, the default for new instances. Every read and write goes through an
// inter-process lock, "<productName>-user-settings", as several hosts may run the plugin at once. Message
// thread only.
class UserSettings
{
public:
    // Without a file nothing is kept: every read gives the default.
    UserSettings (const juce::String& productName, juce::File file);
    // "Application Support/<productName>/<productName>.settings" on macOS, "%APPDATA%\<productName>\..." on Windows.
    static juce::File defaultFile (const juce::String& productName);

    // The UI Scale last picked, in percent, or uiScale::defaultPercent when there is none or the
    // file can't be read.
    int uiScalePercent() const;
    // Saves percent as the UI Scale last picked. False when the file can't be written (the folder
    // can't be made, no permission, a full disk), which is logged with the file's path, and the
    // instance keeps its own UI Scale; the next pick, in any instance, writes the file afresh.
    bool setUiScalePercent (int percent);

private:
    juce::String productName;
    juce::File file;
    juce::PropertiesFile::Options options() const;
    mutable juce::InterProcessLock lock;
};

} // namespace plugkit::ui
