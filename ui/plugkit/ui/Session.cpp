// New in plugkit, lifted from eq1's plugin/PluginProcessor.cpp (eq1 at dbb1127): presetState, loadPreset, the
// window's size and UI Scale, and the save and restore of getStateInformation and setStateInformation, with comp1's
// restore rule (Session.h).

#include "Session.h"

#include "UiScale.h"

namespace plugkit::ui
{

namespace
{
const juce::Identifier versionProperty { "version" }, editorWidthProperty { "editorWidth" }, editorHeightProperty { "editorHeight" },
    uiScaleProperty { "uiScalePercent" };
} // namespace

Session::Session (juce::AudioProcessor& processor, juce::AudioProcessorValueTreeState& p, Config c)
    : parameters (p),
      config (std::move (c)),
      presetType (config.presetType.isValid() ? config.presetType : parameters.state.getType()),
      history (processor),
      compare (parameters, history, config.isPresetSetting),
      library ({ config.productName,
                 config.presetFileExtension,
                 presetType,
                 config.factoryPresets },
               config.presetFolder),
      userSettings (config.productName, config.userSettingsFile),
      editorWidth (config.window.defaultSize.x),
      editorHeight (config.window.defaultSize.y)
{
}

juce::ValueTree Session::presetState()
{
    return capturePresetSettings (parameters, config.isPresetSetting, presetType)
        .setProperty (versionProperty, config.stateVersion, nullptr);
}

bool Session::loadPreset (const juce::ValueTree& preset, const juce::String& name, const juce::String& folder)
{
    if (! preset.hasType (presetType))
        return false;
    auto settings = preset.createCopy();
    if (config.migrate)
        config.migrate (settings);
    compare.loadPreset (settings, name, folder);
    return true;
}

void Session::setEditorSize (juce::Point<int> logical)
{
    if (logical.x > 0 && logical.y > 0)
    {
        editorWidth = logical.x;
        editorHeight = logical.y;
    }
}

int Session::uiScalePercent()
{
    if (uiScale.load() == 0)
        uiScale = userSettings.uiScalePercent();
    return uiScale.load();
}

void Session::pickUiScale (int percent)
{
    if (! uiScale::isOffered (percent))
        return;
    uiScale = percent;
    userSettings.setUiScalePercent (percent);
}

juce::ValueTree Session::saved() const
{
    auto state = compare.savedState();
    state.setProperty (versionProperty, config.stateVersion, nullptr);
    state.setProperty (editorWidthProperty, editorWidth.load(), nullptr);
    state.setProperty (editorHeightProperty, editorHeight.load(), nullptr);
    if (const int percent = uiScale.load(); percent != 0)
        state.setProperty (uiScaleProperty, percent, nullptr);
    return state;
}

bool Session::restore (juce::ValueTree state, const ViewReader& readView)
{
    if (! state.hasType (parameters.state.getType()))
        return false;
    if (config.migrate)
        config.migrate (state);
    state.removeProperty (versionProperty, nullptr);
    // A session saved before them opens like a new instance; a size that isn't a finite number too, and one outside
    // the window's limits opens at the nearer limit.
    {
        const auto& [defaultSize, minimum, maximum] = config.window;
        setEditorSize ({ juce::roundToInt (restoredNumber (state.getProperty (editorWidthProperty, defaultSize.x), defaultSize.x, minimum.x, maximum.x)),
                         juce::roundToInt (restoredNumber (state.getProperty (editorHeightProperty, defaultSize.y), defaultSize.y, minimum.y, maximum.y)) });
    }
    const int percent = state.getProperty (uiScaleProperty, 0);
    uiScale = uiScale::isOffered (percent) ? percent : 0;
    for (const auto& property : { editorWidthProperty, editorHeightProperty, uiScaleProperty })
        state.removeProperty (property, nullptr);
    if (readView)
        readView (state);
    // Without one (from a newer version that saves it elsewhere), the settings are side A's.
    const auto savedCompare = state.getChildWithName (ABCompare::stateType);
    compare.restore (savedCompare);
    state.removeChild (savedCompare, nullptr);
    // A value a file holds reaches a parameter restored, and every parameter gets one.
    sanitiseSavedSettings (parameters, state);
    parameters.replaceState (state);
    history.sessionRestored();
    return true;
}

} // namespace plugkit::ui
