#pragma once

// Copied from eq1's plugin/PresetSettings.h (eq1 at dbb1127), in namespace plugkit::ui, with these differences:
// - which parameters are Preset settings is the plugin's (IsPresetSetting), by default every one but Global Bypass,
//   as eq1's isPresetSetting has it, and each function that visits them takes it;
// - restoredNumber keeps an infinity that is the range's own end (comp1's Ratio, ∞:1), where eq1's gives the default;
// - sanitiseSavedSettings also adds a parameter the state lacks, at its default, and leaves a value that restores as
//   itself untouched where eq1's rewrites an infinity, as comp1's restore does. eq1's leaves a missing parameter to
//   replaceState, which on JUCE 9.0.3 gives it its default too.

#include "SetTogether.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <vector>

namespace plugkit::ui
{

// The settings a Preset holds, as each side of A/B Compare does: every host parameter that affects
// the sound. Message thread only.

// Whether a parameter is a Preset setting: the plugin says, as Session::Config does.
using IsPresetSetting = std::function<bool (const juce::RangedAudioParameter&)>;
// Every parameter but Global Bypass (the ID globalBypassId), which belongs to neither a Preset nor an A/B side.
inline const juce::String globalBypassId { "global_bypass" };
bool everyParameterButGlobalBypass (const juce::RangedAudioParameter& parameter);

// A saved number as it restores, for a parameter's value and for the other numbers a session holds
// alike: itself within minimum to maximum, the nearer end when it is finite outside, an infinity that is
// minimum or maximum itself, and defaultValue when it isn't a number (nan) or is any other infinity. Read
// in double, so a value a float can't hold ("1e39") is still a finite one to clamp. Nothing a file holds
// reaches the processor or the editor unchecked.
double restoredNumber (const juce::var& saved, double defaultValue, double minimum, double maximum);

// The plain value a saved one restores to, on a session restore and on a Preset load alike:
// restoredNumber() within the parameter's range, with the parameter's default for one that isn't a
// number or is an infinity the range doesn't end at. Text that isn't a number reads as 0, as juce::var
// converts it.
float restoredPlainValue (const juce::RangedAudioParameter& parameter, const juce::var& saved);

// Readies a saved state to replace the parameters' state, in place: every PARAM child that names a
// parameter goes through restoredPlainValue(), a value that restores as itself left untouched, so it saves
// again byte for byte; each parameter the tree lacks is added at its default. Whatever else the tree holds
// (a newer version's settings) is kept as it is.
void sanitiseSavedSettings (juce::AudioProcessorValueTreeState& parameters, juce::ValueTree& tree);

// The settings a Preset holds, as a tree of the given type, with a PARAM child for each, holding
// its id and plain value as the saved state does.
juce::ValueTree capturePresetSettings (juce::AudioProcessorValueTreeState& parameters,
                                       const IsPresetSetting& isPresetSetting,
                                       const juce::Identifier& type);

// Sets the settings a Preset holds to those in tree, inside gestures. One the tree doesn't hold
// goes to its default, so the same tree always sounds the same; the other parameters are left alone.
void applyPresetSettings (juce::AudioProcessorValueTreeState& parameters, const IsPresetSetting& isPresetSetting, const juce::ValueTree& tree);

// The settings applyPresetSettings() puts on the parameters from preset, as capturePresetSettings()
// gives them: every setting a Preset holds, those preset leaves out at their defaults.
juce::ValueTree presetSettingsAsLoaded (juce::AudioProcessorValueTreeState& parameters,
                                        const IsPresetSetting& isPresetSetting,
                                        const juce::ValueTree& preset,
                                        const juce::Identifier& type);

// The settings applyPresetSettings() puts on the parameters from tree, each with its parameter and
// normalised value, in parameter order: a tree indexed once, so the Modified check compares them
// with the parameters each tick (parametersHold) without searching the tree for every setting.
using PresetSettingValues = std::vector<NewValue>;
PresetSettingValues presetSettingValues (juce::AudioProcessorValueTreeState& parameters,
                                         const IsPresetSetting& isPresetSetting,
                                         const juce::ValueTree& tree);
// Whether every parameter in values holds its value there. Linear in the settings.
bool parametersHold (const PresetSettingValues& values);

} // namespace plugkit::ui
