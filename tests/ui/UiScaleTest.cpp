// Copied from eq1's tests/plugin/UiScaleTest.cpp (eq1 at dbb1127), against plugkit's test processor and its editor
// (TestProcessor.h), an EditorShell, whose window is eq1's. A version 1 state written here stands in for eq1's
// state-v2.xml fixture.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/UiScaleMenu.h"
#include "plugkit/ui/UserSettings.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <memory>
#include <vector>

using plugkit::test::TestProcessor;

namespace
{

plugkit::ui::UiScaleMenu& uiScaleMenu (juce::AudioProcessorEditor& editor)
{
    auto* menu = harness::findChild<plugkit::ui::UiScaleMenu> (editor, [] (plugkit::ui::UiScaleMenu& c) { return c.getTitle() == "UI Scale"; });
    REQUIRE (menu != nullptr);
    return *menu;
}

// The UI Scale menu's items, in order, after its "UI Scale" title.
std::vector<juce::String> uiScaleItems (plugkit::ui::UiScaleMenu& menu)
{
    std::vector<juce::String> items;
    const auto popup = menu.menu();
    for (juce::PopupMenu::MenuItemIterator it (popup); it.next();)
        if (! it.getItem().isSectionHeader)
            items.push_back (it.getItem().text);
    return items;
}

void pickUiScale (juce::AudioProcessorEditor& editor, int percent)
{
    auto& menu = uiScaleMenu (editor);
    const auto items = uiScaleItems (menu);
    REQUIRE (std::find (items.begin(), items.end(), juce::String (percent) + "%") != items.end());
    menu.pick (percent);
}

std::unique_ptr<juce::AudioProcessorEditor> openEditor (TestProcessor& processor)
{
    return std::unique_ptr<juce::AudioProcessorEditor> (processor.createEditor());
}

void reload (TestProcessor& into, TestProcessor& from)
{
    juce::MemoryBlock state;
    from.getStateInformation (state);
    into.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
}

// A session saved before the window's size and UI Scale were.
void restoreOlderSession (TestProcessor& processor)
{
    const auto older = juce::parseXML (R"(<plugkit_test version="1"><PARAM id="gain" value="-4.5"/></plugkit_test>)");
    REQUIRE (older != nullptr);
    harness::restoreState (processor, *older);
}

} // namespace

TEST_CASE ("A new instance opens at 1200 x 760 and 100%, with a UI Scale menu of 75, 100, 125, 150 and 200%")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    const auto editor = openEditor (processor);
    CHECK (editor->getWidth() == 1200);
    CHECK (editor->getHeight() == 760);
    auto& menu = uiScaleMenu (*editor);
    CHECK (menu.shownPercent() == 100);
    CHECK (uiScaleItems (menu) == std::vector<juce::String> { "75%", "100%", "125%", "150%", "200%" });
}

TEST_CASE ("At each UI Scale the editor is its logical size times the scale, and so are its limits and what it draws")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    const auto editor = openEditor (processor);
    auto& menu = uiScaleMenu (*editor);
    const auto atHundred = editor->getLocalArea (&menu, menu.getLocalBounds());

    const int percent = GENERATE (75, 100, 125, 150, 200);
    CAPTURE (percent);
    pickUiScale (*editor, percent);
    const auto scaled = [percent] (int logical) { return juce::roundToInt (logical * percent / 100.0); };
    CHECK (editor->getWidth() == scaled (1200));
    CHECK (editor->getHeight() == scaled (760));
    const auto* constrainer = editor->getConstrainer();
    REQUIRE (constrainer != nullptr);
    CHECK (constrainer->getMinimumWidth() == scaled (960));
    CHECK (constrainer->getMinimumHeight() == scaled (600));
    CHECK (constrainer->getMaximumWidth() == scaled (TestProcessor::window.maximum.x));
    CHECK (constrainer->getMaximumHeight() == scaled (TestProcessor::window.maximum.y));
    // A control sits where it did at 100%, scaled, and is that much larger.
    const auto drawn = editor->getLocalArea (&menu, menu.getLocalBounds());
    CHECK (drawn == atHundred.transformedBy (juce::AffineTransform::scale (percent / 100.0f)));
}

TEST_CASE ("A screen reader reads the UI Scale menu as a button whose value is the UI Scale")
{
    harness::OpenEditor<TestProcessor> host;
    auto& menu = uiScaleMenu (*host.editor);
    pickUiScale (*host.editor, 125);
    auto* handler = menu.getAccessibilityHandler();
    REQUIRE (handler != nullptr);
    CHECK (handler->getRole() == juce::AccessibilityRole::button);
    CHECK (handler->getTitle() == "UI Scale");
    REQUIRE (handler->getValueInterface() != nullptr);
    CHECK (handler->getValueInterface()->getCurrentValueAsString() == "125%");
}

TEST_CASE ("A pick in the UI Scale menu is the session's UI Scale, ticked in the menu, and told to the editor")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    const auto editor = openEditor (processor);
    auto& menu = uiScaleMenu (*editor);
    int told = 0;
    menu.onPicked = [&told] (int percent) { told = percent; };
    const int percent = GENERATE (75, 125, 150, 200);
    CAPTURE (percent);
    pickUiScale (*editor, percent);
    CHECK (processor.sessionState().uiScalePercent() == percent);
    CHECK (menu.shownPercent() == percent);
    CHECK (told == percent);
    for (juce::PopupMenu::MenuItemIterator it (menu.menu()); it.next();)
        CHECK (it.getItem().isTicked == (it.getItem().itemID == percent));
}

TEST_CASE ("The window's size and UI Scale survive closing and reopening the editor")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    {
        const auto editor = openEditor (processor);
        pickUiScale (*editor, 150);
        editor->setSize (1800, 1200); // 1200 x 800 logical
    }
    const auto editor = openEditor (processor);
    CHECK (uiScaleMenu (*editor).shownPercent() == 150);
    CHECK (editor->getWidth() == 1800);
    CHECK (editor->getHeight() == 1200);
}

TEST_CASE ("The window's size and UI Scale are saved with the session, not in a Preset, and change neither the undo history nor Modified")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor saved;
    auto& session = saved.sessionState();
    // With a Loaded Preset, a change to any sound setting would make the side Modified.
    REQUIRE (session.loadPreset (session.presetState(), "Current"));
    const int steps = saved.editHistory().undoSteps();
    const auto preset = session.presetState().createXml()->toString();
    {
        const auto editor = openEditor (saved);
        pickUiScale (*editor, 75);
        editor->setSize (900, 600); // 1200 x 800 logical
    }
    CHECK_FALSE (session.isLoadedPresetModified());
    CHECK (saved.editHistory().undoSteps() == steps);
    CHECK (session.presetState().createXml()->toString() == preset);
    CHECK (harness::savedState (saved).getProperty ("version") == juce::var (TestProcessor::stateVersion));

    TestProcessor restored;
    reload (restored, saved);
    const auto editor = openEditor (restored);
    CHECK (uiScaleMenu (*editor).shownPercent() == 75);
    CHECK (editor->getWidth() == 900);
    CHECK (editor->getHeight() == 600);
}

TEST_CASE ("A session restored while the editor is open shows its UI Scale on the next tick")
{
    harness::OpenEditor<TestProcessor> host;
    TestProcessor saved;
    saved.sessionState().pickUiScale (125);
    REQUIRE (uiScaleMenu (*host.editor).shownPercent() == 100);
    harness::restoreState (host.processor, harness::savedBlock (saved));
    host.settle (300);
    CHECK (uiScaleMenu (*host.editor).shownPercent() == 125);
}

TEST_CASE ("A session saved before the window's size and UI Scale opens like a new instance, with the state version unchanged")
{
    juce::ScopedJuceInitialiser_GUI juce;
    TestProcessor processor;
    {
        const auto editor = openEditor (processor);
        pickUiScale (*editor, 200);
        editor->setSize (2600, 1400);
    }
    restoreOlderSession (processor);

    const auto editor = openEditor (processor);
    CHECK (uiScaleMenu (*editor).shownPercent() == 100);
    CHECK (editor->getWidth() == 1200);
    CHECK (editor->getHeight() == 760);
    CHECK (TestProcessor::stateVersion == 2);
}

TEST_CASE ("Picking a UI Scale in one instance makes a new instance open at it; instances already open keep their own")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::TemporaryFile settings;
    TestProcessor first (settings.getFile()), open (settings.getFile());
    const auto openEditorOfOpen = openEditor (open);
    {
        const auto editor = openEditor (first);
        pickUiScale (*editor, 125);
    }
    CHECK (uiScaleMenu (*openEditorOfOpen).shownPercent() == 100);

    TestProcessor next (settings.getFile());
    const auto editor = openEditor (next);
    CHECK (uiScaleMenu (*editor).shownPercent() == 125);
}

TEST_CASE ("A new instance opens at 100% with no settings file, or one it can't read")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::TemporaryFile settings;
    SECTION ("none") { REQUIRE_FALSE (settings.getFile().exists()); }
    SECTION ("unreadable") { REQUIRE (settings.getFile().replaceWithText ("not settings")); }
    SECTION ("not a UI Scale")
    {
        TestProcessor picker (settings.getFile());
        pickUiScale (*openEditor (picker), 150);
        REQUIRE (settings.getFile().loadFileAsString().contains ("150"));
        REQUIRE (settings.getFile().replaceWithText (settings.getFile().loadFileAsString().replace ("150", "110")));
    }
    TestProcessor processor (settings.getFile());
    CHECK (uiScaleMenu (*openEditor (processor)).shownPercent() == 100);
}

TEST_CASE ("An instance takes the per-user UI Scale when its editor first opens and keeps its own from then on")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::TemporaryFile settings;
    TestProcessor other (settings.getFile()), processor (settings.getFile());
    pickUiScale (*openEditor (other), 150);
    openEditor (processor).reset();
    pickUiScale (*openEditor (other), 75);
    CHECK (uiScaleMenu (*openEditor (processor)).shownPercent() == 150);

    SECTION ("and an older session takes the per-user UI Scale")
    {
        restoreOlderSession (processor);
        CHECK (uiScaleMenu (*openEditor (processor)).shownPercent() == 75);
    }
}

TEST_CASE ("A user settings save that fails is reported, keeps this instance's UI Scale, and is retried on the next pick")
{
    juce::ScopedJuceInitialiser_GUI juce;
    // The settings file's folder is a file: nothing can be written there.
    juce::TemporaryFile blocking;
    REQUIRE (blocking.getFile().replaceWithText ("in the way"));
    const auto file = blocking.getFile().getChildFile ("plugkit test.settings");
    // Collects what the plugin logs while it lives, so a failed check leaves no dangling logger behind.
    struct Log final : juce::Logger
    {
        juce::StringArray lines;
        Log() { juce::Logger::setCurrentLogger (this); }
        ~Log() override { juce::Logger::setCurrentLogger (nullptr); }
        void logMessage (const juce::String& message) override { lines.add (message); }
    } log;

    plugkit::ui::UserSettings settings ("plugkit test", file);
    CHECK_FALSE (settings.setUiScalePercent (150));
    REQUIRE (log.lines.size() == 1);
    CHECK (log.lines[0].contains (file.getFullPathName()));
    CHECK (log.lines[0].startsWith ("plugkit test: "));
    CHECK (plugkit::ui::UserSettings ("plugkit test", file).uiScalePercent() == 100);

    // The instance keeps the UI Scale it picked.
    TestProcessor processor (file);
    pickUiScale (*openEditor (processor), 125);
    CHECK (processor.sessionState().uiScalePercent() == 125);
    CHECK (log.lines.size() == 2);

    // Writable now: the next pick saves.
    REQUIRE (blocking.getFile().deleteFile());
    CHECK (settings.setUiScalePercent (150));
    CHECK (plugkit::ui::UserSettings ("plugkit test", file).uiScalePercent() == 150);
    CHECK (log.lines.size() == 2);
    file.getParentDirectory().deleteRecursively();
}

TEST_CASE ("The per-user settings file is in Application Support/<product name> on macOS and %APPDATA%/<product name> on Windows")
{
    const auto file = plugkit::ui::UserSettings::defaultFile ("plugkit test");
    CHECK (file.getFileName() == "plugkit test.settings");
    const auto folder = file.getParentDirectory();
    CHECK (folder.getFileName() == "plugkit test");
#if JUCE_MAC
    CHECK (folder.getParentDirectory() == juce::File ("~/Library/Application Support"));
#elif JUCE_WINDOWS
    CHECK (folder.getParentDirectory() == juce::File (juce::SystemStats::getEnvironmentVariable ("APPDATA", {})));
#endif
}
