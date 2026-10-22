// Copied from eq1's plugin/PluginEditor.cpp (eq1 at dbb1127), in namespace plugkit::ui: its construction, UI Scale,
// keys, tick, background and layout, without eq1's EQ display, Band panel, Detection Range bar, Display Range, Output
// Meter and footer, which the plugin adds and lays out (layOutBody, open). The window's limits are the Session's, and
// Cmd-B sets the Global Bypass parameter (setTogether), where eq1's toggles its footer's button.

#include "EditorShell.h"

#include "Session.h"
#include "SetTogether.h"
#include "staple/Tokens.h"
#include "staple/WindowBackground.h"
#include "staple/controls/Overlay.h"

#include <algorithm>

namespace plugkit::ui
{

namespace layout = staple::tokens::layout;

EditorShell::EditorShell (juce::AudioProcessor& p, Session& s, Config c)
    : AudioProcessorEditor (p), EditorTick::Client (static_cast<juce::Component&> (*this)), session (s), config (std::move (c)), ticker (p),
      headerBar (s, config.wordmark), keyboard (s.editHistory())
{
    // Popovers and knob tooltips float in it, scaled with everything else.
    staple::markOverlayLayer (contentComponent);
    contentComponent.addAndMakeVisible (headerBar);
    contentComponent.addChildComponent (headerBar.presetBrowser());
    // Tab's order: the header, and the Preset browser while open, before the plugin's components.
    int order = 0;
    for (juce::Component* child : std::initializer_list<juce::Component*> { &headerBar, &headerBar.presetBrowser() })
        child->setExplicitFocusOrder (++order);
    jassert (order + 1 == firstFocusOrder);

    ticker.add (*this);
}

void EditorShell::open (juce::Component& plugins)
{
    footer = &plugins;
    // After every component of the plugin's in Tab's order.
    int last = 0;
    for (auto* child : contentComponent.getChildren())
        last = std::max (last, child->getExplicitFocusOrder());
    footer->setExplicitFocusOrder (last + 1);
    contentComponent.addAndMakeVisible (*footer);
    // The Preset browser, a modal over everything.
    headerBar.presetBrowser().toFront (false);

    // Once every component is in place: each one that keeps something from its look (a Slider's
    // text box, a ComboBox's label colours) takes it again from this.
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (contentComponent);
    keyboard.adopt (*this);
    addMouseListener (this, true);

    setResizable (true, true);
    applyUiScale();
}

void EditorShell::applyUiScale()
{
    // In logical pixels.
    const auto& [defaultSize, minimum, maximum] = session.window();
    const auto stored = session.editorSize();
    const juce::Point<int> size { juce::jlimit (minimum.x, maximum.x, stored.x), juce::jlimit (minimum.y, maximum.y, stored.y) };
    const int percent = session.uiScalePercent();
    shownScalePercent = percent;
    scale = static_cast<float> (percent) / 100.0f;
    contentComponent.setTransform (juce::AffineTransform::scale (scale));
    const auto scaled = [this] (int logical) { return juce::roundToInt (static_cast<float> (logical) * scale); };
    // New limits can resize the window on the way to its size: keep that from the Session.
    applyingScale = true;
    setResizeLimits (scaled (minimum.x), scaled (minimum.y), scaled (maximum.x), scaled (maximum.y));
    setSize (scaled (size.x), scaled (size.y));
    applyingScale = false;
    resized();
}

EditorShell::~EditorShell()
{
    setLookAndFeel (nullptr);
}

void EditorShell::mouseDown (const juce::MouseEvent&)
{
    lookAndFeel.showFocusRing (false);
}

bool EditorShell::keyPressed (const juce::KeyPress& key)
{
    if (key.isKeyCode (juce::KeyPress::tabKey) && ! isParentOf (getCurrentlyFocusedComponent()))
    {
        const auto all = createKeyboardFocusTraverser()->getAllComponents (this);
        if (all.empty())
            return false;
        lookAndFeel.showFocusRing (true);
        (key.getModifiers().isShiftDown() ? all.back() : all.front())->grabKeyboardFocus();
        return true;
    }
    const auto command = juce::ModifierKeys::commandModifier;
    if (key == juce::KeyPress ('b', command, 0))
    {
        toggleGlobalBypass();
        return true;
    }
    if (key == juce::KeyPress ('z', command, 0))
    {
        headerBar.undo();
        return true;
    }
    if (key == juce::KeyPress ('z', command | juce::ModifierKeys::shiftModifier, 0) || key == juce::KeyPress ('y', command, 0))
    {
        headerBar.redo();
        return true;
    }
    return false;
}

void EditorShell::toggleGlobalBypass()
{
    for (auto* parameter : processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter); ranged != nullptr && ranged->getParameterID() == config.globalBypassId)
            setTogether ({ { ranged, ranged->getValue() >= 0.5f ? 0.0f : 1.0f } });
}

void EditorShell::tick (double)
{
    if (session.uiScalePercent() != shownScalePercent || session.editorSize() != shownSize)
        applyUiScale();
}

void EditorShell::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scale));
    const auto window = contentComponent.getLocalBounds().toFloat();
    background.draw (g, {}, window, 0, juce::Image::RGB, [&] (juce::Graphics& into) { staple::paintWindowBackground (into, window); });
}

void EditorShell::resized()
{
    const juce::Point<int> logical { juce::roundToInt (static_cast<float> (getWidth()) / scale), juce::roundToInt (static_cast<float> (getHeight()) / scale) };
    if (! applyingScale)
    {
        session.setEditorSize (logical);
        shownSize = session.editorSize();
    }
    contentComponent.setBounds (0, 0, logical.x, logical.y);

    // Padded all round, with a gap between the rows.
    auto area = contentComponent.getLocalBounds().reduced (layout::outerPadding);
    headerBar.setBounds (area.removeFromTop (layout::headerHeight));
    area.removeFromTop (layout::gap);
    // The Preset browser, a modal over everything.
    headerBar.presetBrowser().setBounds (contentComponent.getLocalBounds());
    if (footer == nullptr)
        return;
    footer->setBounds (area.removeFromBottom (layout::footerHeight));
    area.removeFromBottom (layout::gap);
    layOutBody (area);
}

} // namespace plugkit::ui
