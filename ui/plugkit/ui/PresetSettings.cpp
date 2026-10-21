// Copied from eq1's plugin/PresetSettings.cpp (eq1 at dbb1127), in namespace plugkit::ui, with the differences its
// header lists: the plugin's IsPresetSetting, an infinity at the range's end kept, and a parameter the saved state
// lacks added at its default.

#include "PresetSettings.h"

#include <cmath>
#include <unordered_map>

namespace plugkit::ui
{

namespace
{
const juce::Identifier parameterType { "PARAM" }, idProperty { "id" }, valueProperty { "value" };

template <typename Visit>
void forEachPresetSetting (juce::AudioProcessorValueTreeState& parameters, const IsPresetSetting& isPresetSetting, Visit&& visit)
{
    for (auto* p : parameters.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p); ranged != nullptr && isPresetSetting (*ranged))
            visit (*ranged);
}

// Visits every setting a Preset holds with the normalised value tree sets it to: its own, or the
// default. The tree's settings are indexed by id first (the first of an id counts, as
// getChildWithProperty would find it), so a visit of them all is linear in their number.
template <typename Visit>
void forEachPresetSettingIn (juce::AudioProcessorValueTreeState& parameters,
                             const IsPresetSetting& isPresetSetting,
                             const juce::ValueTree& tree,
                             Visit&& visit)
{
    std::unordered_map<juce::String, juce::var> saved;
    for (const auto& setting : tree)
        if (setting.hasType (parameterType))
            saved.try_emplace (setting.getProperty (idProperty).toString(), setting.getProperty (valueProperty));
    forEachPresetSetting (parameters, isPresetSetting, [&] (juce::RangedAudioParameter& p) {
        const auto found = saved.find (p.getParameterID());
        visit (p, found != saved.end() ? p.convertTo0to1 (restoredPlainValue (p, found->second)) : p.getDefaultValue());
    });
}

juce::ValueTree settingOf (const juce::RangedAudioParameter& p, float value)
{
    return juce::ValueTree (parameterType)
        .setProperty (idProperty, p.getParameterID(), nullptr)
        .setProperty (valueProperty, p.convertFrom0to1 (value), nullptr);
}

double defaultPlainValue (const juce::RangedAudioParameter& parameter)
{
    return static_cast<double> (parameter.convertFrom0to1 (parameter.getDefaultValue()));
}
} // namespace

bool everyParameterButGlobalBypass (const juce::RangedAudioParameter& parameter)
{
    return parameter.getParameterID() != globalBypassId;
}

double restoredNumber (const juce::var& saved, double defaultValue, double minimum, double maximum)
{
    const double value = static_cast<double> (saved);
    if (std::isinf (value) && (juce::exactlyEqual (value, minimum) || juce::exactlyEqual (value, maximum)))
        return value;
    return std::isfinite (value) ? juce::jlimit (minimum, maximum, value) : defaultValue;
}

namespace
{
// restoredPlainValue() in double, so a saved value that restores as itself compares equal to its text.
double restoredPlainNumber (const juce::RangedAudioParameter& parameter, const juce::var& saved)
{
    const auto& range = parameter.getNormalisableRange();
    return restoredNumber (saved, defaultPlainValue (parameter), static_cast<double> (range.start), static_cast<double> (range.end));
}
} // namespace

float restoredPlainValue (const juce::RangedAudioParameter& parameter, const juce::var& saved)
{
    return static_cast<float> (restoredPlainNumber (parameter, saved));
}

void sanitiseSavedSettings (juce::AudioProcessorValueTreeState& parameters, juce::ValueTree& tree)
{
    for (auto* p : parameters.processor.getParameters())
    {
        const auto* ranged = dynamic_cast<const juce::RangedAudioParameter*> (p);
        if (ranged == nullptr)
            continue;
        auto setting = tree.getChildWithProperty (idProperty, ranged->getParameterID());
        if (! setting.hasType (parameterType))
        {
            tree.appendChild (juce::ValueTree (parameterType)
                                  .setProperty (idProperty, ranged->getParameterID(), nullptr)
                                  .setProperty (valueProperty, defaultPlainValue (*ranged), nullptr),
                              nullptr);
            continue;
        }
        const auto saved = setting.getProperty (valueProperty);
        if (const double restored = restoredPlainNumber (*ranged, saved);
            ! setting.hasProperty (valueProperty) || ! juce::exactlyEqual (restored, static_cast<double> (saved)))
            setting.setProperty (valueProperty, restored, nullptr);
    }
}

juce::ValueTree capturePresetSettings (juce::AudioProcessorValueTreeState& parameters,
                                       const IsPresetSetting& isPresetSetting,
                                       const juce::Identifier& type)
{
    juce::ValueTree tree (type);
    forEachPresetSetting (parameters, isPresetSetting, [&tree] (juce::RangedAudioParameter& p) {
        tree.appendChild (settingOf (p, p.getValue()), nullptr);
    });
    return tree;
}

void applyPresetSettings (juce::AudioProcessorValueTreeState& parameters, const IsPresetSetting& isPresetSetting, const juce::ValueTree& tree)
{
    std::vector<NewValue> changes;
    forEachPresetSettingIn (parameters, isPresetSetting, tree, [&changes] (juce::RangedAudioParameter& p, float value) {
        if (! juce::exactlyEqual (value, p.getValue()))
            changes.push_back ({ &p, value });
    });
    setTogether (changes);
}

juce::ValueTree presetSettingsAsLoaded (juce::AudioProcessorValueTreeState& parameters,
                                        const IsPresetSetting& isPresetSetting,
                                        const juce::ValueTree& preset,
                                        const juce::Identifier& type)
{
    juce::ValueTree tree (type);
    forEachPresetSettingIn (parameters, isPresetSetting, preset, [&tree] (juce::RangedAudioParameter& p, float value) {
        tree.appendChild (settingOf (p, value), nullptr);
    });
    return tree;
}

PresetSettingValues presetSettingValues (juce::AudioProcessorValueTreeState& parameters,
                                         const IsPresetSetting& isPresetSetting,
                                         const juce::ValueTree& tree)
{
    PresetSettingValues values;
    forEachPresetSettingIn (parameters, isPresetSetting, tree, [&values] (juce::RangedAudioParameter& p, float value) {
        values.push_back ({ &p, value });
    });
    return values;
}

bool parametersHold (const PresetSettingValues& values)
{
    for (const auto& [parameter, value] : values)
        if (! juce::exactlyEqual (value, parameter->getValue()))
            return false;
    return true;
}

} // namespace plugkit::ui
