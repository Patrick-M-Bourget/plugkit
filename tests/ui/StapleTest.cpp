// Copied from eq1's tests/plugin/StapleTest.cpp (eq1 at dbb1127), without its case on eq1's editor, "The editor draws
// every stock widget with Staple's LookAndFeel, token colours and Manrope"; listed in eq1#231.

#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/staple/CachedLayer.h"
#include "plugkit/ui/staple/Fonts.h"
#include "plugkit/ui/staple/Icons.h"
#include "plugkit/ui/staple/LookAndFeel.h"
#include "plugkit/ui/staple/Painting.h"
#include "plugkit/ui/staple/TipWatchdog.h"
#include "plugkit/ui/staple/Tokens.h"
#include "plugkit/ui/staple/controls/Tween.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

using Catch::Matchers::WithinAbs;

// Under Global Bypass, what shows a plugin's work keeps 45 % of its alphas: eq1's value, which comp1 shares.
static_assert (juce::exactlyEqual (staple::tokens::motion::globalBypassScale, 0.45f));

TEST_CASE ("Staple's font is Manrope, with digits of equal width so numbers don't jitter")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const auto size = GENERATE (staple::tokens::size::fs2, staple::tokens::size::fs5);
    const auto weight = GENERATE (staple::Weight::regular, staple::Weight::semiBold);
    const auto font = staple::font (size, weight);
    CHECK (font.getTypefacePtr()->getName() == "Manrope");

    const float zero = juce::GlyphArrangement::getStringWidth (font, "0");
    CHECK (zero > 0.0f);
    for (const char* digit : { "1", "2", "3", "4", "5", "6", "7", "8", "9" })
    {
        CAPTURE (size, digit);
        CHECK_THAT (juce::GlyphArrangement::getStringWidth (font, digit), WithinAbs (zero, 1.0e-3));
    }
}

// The main curve's stroke, as HANDOFF.md §4 "Curves" gives it: a 2 px line over a 4 px halo, and an 8 px halo at half
// alpha in place of a blur.
static_assert (juce::exactlyEqual (staple::tokens::curve::mainLine, 2.0f));
static_assert (juce::exactlyEqual (staple::tokens::curve::mainHalo, 4.0f));
static_assert (juce::exactlyEqual (staple::tokens::curve::mainOuterHalo, 8.0f));
static_assert (juce::exactlyEqual (staple::tokens::curve::mainOuterHaloAlpha, 0.5f));

TEST_CASE ("The section title's font is the design system's section style: 12 px Manrope at 700, letter-spaced 0.08 em")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::SharedResourcePointer<staple::Typefaces> held; // the same bold typeface for both calls below
    const auto font = staple::sectionTitleFont();
    CHECK (font.getTypefacePtr() == staple::typeface (staple::Weight::bold));
    CHECK_THAT (font.getHeightInPoints(), WithinAbs (12.0f, 1.0e-4));
    CHECK_THAT (font.getExtraKerningFactor(), WithinAbs (0.08f, 1.0e-6));
}

TEST_CASE ("The label font is the design system's label style: 11 px Manrope at 500, letter-spaced 0.08 em")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const juce::SharedResourcePointer<staple::Typefaces> held; // the same medium typeface for both calls below
    const auto font = staple::labelFont();
    CHECK (font.getTypefacePtr() == staple::typeface (staple::Weight::medium));
    CHECK_THAT (font.getHeightInPoints(), WithinAbs (11.0f, 1.0e-4));
    CHECK_THAT (font.getExtraKerningFactor(), WithinAbs (0.08f, 1.0e-6));
}

TEST_CASE ("A toggle as wide as its ideal width draws its label whole, as a wider one does, and a pixel narrower squeezes it")
{
    juce::ScopedJuceInitialiser_GUI juce;
    staple::LookAndFeel lookAndFeel;
    const juce::String text = GENERATE ("Auto Release", "Lookahead", "M");
    CAPTURE (text);
    juce::ToggleButton toggle (text);
    toggle.setLookAndFeel (&lookAndFeel);
    const auto drawnAt = [&] (int width) {
        toggle.setSize (width, 24);
        return harness::paintImage (200, 24, [&] (juce::Graphics& g) { lookAndFeel.drawToggleButton (g, toggle, false, false); });
    };
    const int ideal = staple::LookAndFeel::idealToggleWidth (text);
    REQUIRE (ideal < 200);
    const auto wide = drawnAt (200);
    const auto fitted = drawnAt (ideal);
    const auto narrower = drawnAt (ideal - 1);
    toggle.setLookAndFeel (nullptr);

    const auto differs = [] (const juce::Image& a, const juce::Image& b) {
        for (int y = 0; y < a.getHeight(); ++y)
            for (int x = 0; x < a.getWidth(); ++x)
                if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                    return true;
        return false;
    };
    CHECK_FALSE (differs (fitted, wide));
    CHECK (differs (narrower, wide));
}

TEST_CASE ("Every icon the prototype uses parses to a path inside its grid")
{
    for (int i = 0; i < staple::numIcons; ++i)
    {
        const auto icon = static_cast<staple::Icon> (i);
        CAPTURE (i);
        const auto grid = staple::gridOf (icon);
        REQUIRE (! grid.isEmpty());
        CAPTURE (staple::nameOf (icon));
        const auto& path = staple::pathOf (icon);
        CHECK (! path.isEmpty());
        CHECK (! path.getBounds().isEmpty());
        CHECK (grid.contains (path.getBounds()));
    }
}

namespace
{
// The largest difference in alpha between two images, over the pixels a predicate keeps.
int largestAlphaDifference (const juce::Image& a, const juce::Image& b, const std::function<bool (juce::Point<float>)>& compared)
{
    int largest = 0;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
            if (compared ({ x + 0.5f, y + 0.5f }))
                largest = std::max (largest, std::abs (a.getPixelAt (x, y).getAlpha() - b.getPixelAt (x, y).getAlpha()));
    return largest;
}

// An icon drawn in Staple's stroke, fitted into a 64 px square offset half a pixel, so that a grid point at a
// multiple of 0.25 lands on a pixel's centre.
juce::Image drawnIcon (staple::Icon icon)
{
    return harness::paintImage (65, 65, [icon] (juce::Graphics& g) {
        staple::drawIcon (g, icon, { 0.5f, 0.5f, 64.0f, 64.0f }, juce::Colours::white);
    });
}

// The alpha at a point of a 16 px grid icon drawn as above (4 px per grid unit).
int alphaAt (const juce::Image& drawn, float gridX, float gridY)
{
    return drawn.getPixelAt (juce::roundToInt (gridX * 4.0f), juce::roundToInt (gridY * 4.0f)).getAlpha();
}
} // namespace

// The view and window-size switch's arrows, as the design system's Footer preview draws them: on the 16 px grid,
// one arrow along each diagonal, expand's heads at the corners and collapse's at the centre.
TEST_CASE ("The expand and collapse arrows point out to the corners and in to the centre, unlike any other icon")
{
    juce::ScopedJuceInitialiser_GUI juce;
    for (const auto icon : { staple::Icon::expand, staple::Icon::collapse })
    {
        CAPTURE (staple::nameOf (icon));
        CHECK (staple::gridOf (icon) == juce::Rectangle<float> (16.0f, 16.0f));
        const auto bounds = staple::pathOf (icon).getBounds();
        CHECK (bounds.getWidth() > 8.0f);
        CHECK (bounds.getHeight() > 8.0f);
        CHECK (juce::Rectangle<float> (1.0f, 1.0f, 14.0f, 14.0f).contains (bounds));
    }
    CHECK (juce::String (staple::nameOf (staple::Icon::expand)) == "expand");
    CHECK (juce::String (staple::nameOf (staple::Icon::collapse)) == "collapse");

    const auto expand = drawnIcon (staple::Icon::expand);
    const auto collapse = drawnIcon (staple::Icon::collapse);

    // Both run the diagonal from a corner towards the centre.
    for (const auto* drawn : { &expand, &collapse })
    {
        CHECK (alphaAt (*drawn, 13.0f, 3.0f) > 128);
        CHECK (alphaAt (*drawn, 3.0f, 13.0f) > 128);
    }

    // Expand's heads lie along the outer edges, by the corners.
    CHECK (alphaAt (expand, 12.0f, 2.0f) > 128);
    CHECK (alphaAt (expand, 14.0f, 4.0f) > 128);
    CHECK (alphaAt (expand, 4.0f, 14.0f) > 128);
    CHECK (alphaAt (expand, 2.0f, 12.0f) > 128);
    CHECK (alphaAt (collapse, 12.0f, 2.0f) < 32);
    CHECK (alphaAt (collapse, 2.0f, 12.0f) < 32);

    // Collapse's heads are at the centre end.
    CHECK (alphaAt (collapse, 9.5f, 4.5f) > 128);
    CHECK (alphaAt (collapse, 11.5f, 6.5f) > 128);
    CHECK (alphaAt (collapse, 6.5f, 11.5f) > 128);
    CHECK (alphaAt (collapse, 4.5f, 9.5f) > 128);
    CHECK (alphaAt (expand, 9.5f, 4.5f) < 32);
    CHECK (alphaAt (expand, 4.5f, 9.5f) < 32);

    // Drawn at the same size, each differs from every other icon by more than a faint edge somewhere.
    for (const auto icon : { staple::Icon::expand, staple::Icon::collapse })
        for (int i = 0; i < staple::numIcons; ++i)
        {
            const auto other = static_cast<staple::Icon> (i);
            if (other == icon)
                continue;
            CAPTURE (staple::nameOf (icon), staple::nameOf (other));
            CHECK (largestAlphaDifference (drawnIcon (icon), drawnIcon (other), [] (juce::Point<float>) { return true; }) > 128);
        }
}

// A shadow that blurs renders an image on every paint; the kit's controls repaint at frame rate, so
// their shadows are drawSoftShadow's stacked shapes instead (HANDOFF.md §4). Outside the control, what
// the LookAndFeel draws is that shadow and nothing else.
// The family's one-knob size (comp1's Compact View Comp), beyond Staple's Frequency, Gain, Q and small.
TEST_CASE ("The hero knob is 120 px")
{
    CHECK (juce::exactlyEqual (staple::tokens::knob::hero, 120.0f));
}

TEST_CASE ("A knob's rim, arc and glow keep their widths up to the Gain knob's 66 px, and grow with the diameter above it; the arc is thin below 40 px")
{
    struct Row
    {
        float diameter, rim, arc, arcGlow;
    };
    const Row rows[] = {
        { 30.0f, 1.5f, 2.0f, 1.5f },
        { 36.0f, 2.0f, 2.0f, 1.5f }, // above the small rim's 30 px, below the full arc's 40 px
        { 50.0f, 2.0f, 3.0f, 1.5f },
        { 66.0f, 2.0f, 3.0f, 1.5f },
        { 120.0f, 3.636364f, 5.454545f, 2.727273f }, // 120 / 66 of the Gain knob's
    };
    for (const auto& row : rows)
    {
        CAPTURE (row.diameter);
        const auto strokes = staple::knobStrokes (row.diameter);
        CHECK_THAT (strokes.rim, WithinAbs (row.rim, 1.0e-5));
        CHECK_THAT (strokes.arc, WithinAbs (row.arc, 1.0e-5));
        CHECK_THAT (strokes.arcGlow, WithinAbs (row.arcGlow, 1.0e-5));
    }
}

TEST_CASE ("Staple's LookAndFeel draws a stock knob's and slider thumb's shadows without blur")
{
    juce::ScopedJuceInitialiser_GUI juce;
    staple::LookAndFeel lookAndFeel;
    juce::Slider slider;
    slider.setLookAndFeel (&lookAndFeel);
    slider.setRange (0.0, 1.0);

    SECTION ("A rotary slider's knob")
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        const auto drawn = harness::paintImage (100, 100, [&] (juce::Graphics& g) {
            lookAndFeel.drawRotarySlider (g, 0, 0, 100, 100, 0.0f, -2.5f, 2.5f, slider);
        });
        // drawRotarySlider insets the knob's face 10 px.
        const auto face = juce::Rectangle<float> (10.0f, 10.0f, 80.0f, 80.0f);
        const auto shadow = harness::paintImage (100, 100, [&] (juce::Graphics& g) {
            staple::drawSoftShadow (g, face, face.getWidth() / 2.0f, staple::tokens::shadow::knob);
        });
        CHECK (largestAlphaDifference (drawn, shadow, [&] (juce::Point<float> p) {
                   return p.getDistanceFrom (face.getCentre()) > face.getWidth() / 2.0f + 1.5f;
               }) <= 1);
    }

    SECTION ("A linear slider's thumb")
    {
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        const auto drawn = harness::paintImage (100, 100, [&] (juce::Graphics& g) {
            lookAndFeel.drawLinearSlider (g, 0, 0, 100, 100, 50.0f, 0.0f, 100.0f, juce::Slider::LinearHorizontal, slider);
        });
        const float thumbSize = 2.0f * static_cast<float> (lookAndFeel.getSliderThumbRadius (slider));
        const auto thumb = juce::Rectangle<float> (thumbSize, thumbSize).withCentre ({ 50.0f, 50.0f });
        const auto shadow = harness::paintImage (100, 100, [&] (juce::Graphics& g) {
            staple::drawSoftShadow (g, thumb, thumbSize / 2.0f, staple::tokens::shadow::handle);
        });
        // Away from the thumb and from the 2 px track through its centre.
        CHECK (largestAlphaDifference (drawn, shadow, [&] (juce::Point<float> p) {
                   return p.getDistanceFrom (thumb.getCentre()) > thumbSize / 2.0f + 1.5f && std::abs (p.y - 50.0f) > 2.5f;
               }) <= 1);
    }
}

namespace
{
// The largest difference of any colour channel between two images of the same size.
int largestColourDifference (const juce::Image& a, const juce::Image& b)
{
    int largest = 0;
    for (int y = 0; y < a.getHeight(); ++y)
        for (int x = 0; x < a.getWidth(); ++x)
        {
            const auto ca = a.getPixelAt (x, y), cb = b.getPixelAt (x, y);
            largest = std::max ({ largest, std::abs (ca.getRed() - cb.getRed()), std::abs (ca.getGreen() - cb.getGreen()), std::abs (ca.getBlue() - cb.getBlue()) });
        }
    return largest;
}
} // namespace

// What doesn't change between frames is drawn once into an image and from it after (staple::CachedLayer,
// staple::SoftShadowCache): it looks as drawn directly, at 1x and 2x, anywhere within a pixel, and
// whatever colour the last drawing left the Graphics with. Onto the platform's own images, which a
// window draws with and the layers draw into.
TEST_CASE ("A cached layer and a cached soft shadow draw as their drawing does, at 1x and 2x and anywhere within a pixel")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const float scale = GENERATE (1.0f, 2.0f);
    const float offset = GENERATE (0.0f, 0.3f, 0.5f, 0.77f);
    INFO ("scale " << scale << ", offset " << offset);
    const auto text = [&] (juce::Graphics& g) {
        g.setFont (staple::font (staple::tokens::size::fs2));
        g.setColour (staple::tokens::colour::text2);
        g.drawText ("20k", juce::Rectangle<float> (20.0f + offset, 30.0f + offset, 48.0f, 14.0f), juce::Justification::centred, false);
    };
    const auto handle = juce::Rectangle<float> (22.0f, 22.0f).withCentre ({ 50.0f + offset, 20.0f + offset });
    const auto glow = juce::DropShadow { juce::Colour (0xff4ab8f0).withAlpha (0.4f), 6, {} };
    const auto frame = [&] (auto&& draw) {
        return harness::paintImage (juce::roundToInt (100 * scale), juce::roundToInt (60 * scale), [&] (juce::Graphics& g) {
            g.fillAll (juce::Colour (0xffc08020));
            g.addTransform (juce::AffineTransform::scale (scale));
            // Left translucent by the drawing before, as a faded curve leaves it.
            g.setColour (juce::Colours::white.withAlpha (0.2f));
            draw (g);
        }, juce::NativeImageType()); // as a window and the layers' own images are
    };

    const auto direct = frame ([&] (juce::Graphics& g) {
        text (g);
        staple::drawSoftShadow (g, handle, 11.0f, staple::tokens::shadow::selectedHandle);
        staple::drawSoftShadow (g, handle.expanded (1.0f), 12.0f, glow);
    });
    staple::CachedLayer<int> layer;
    staple::SoftShadowCache shadows;
    const auto cached = [&] (juce::Graphics& g) {
        layer.draw (g, {}, { 10.0f, 25.0f, 70.0f, 25.0f }, 0, juce::Image::ARGB, text);
        shadows.draw (g, {}, handle, 11.0f, staple::tokens::shadow::selectedHandle);
        // The selected handle's glow is drawn directly, as the display draws it: from an image as well,
        // its rounding would add to the shadow's beneath it.
        staple::drawSoftShadow (g, handle.expanded (1.0f), 12.0f, glow);
    };
    CHECK (largestColourDifference (direct, frame (cached)) <= 2);
    // Drawn again, from the images.
    CHECK (largestColourDifference (direct, frame (cached)) <= 2);
}

// An image is drawn back pixel for pixel only where its drawing starts on a whole device pixel. The EQ
// display starts 78 px down the window: at 75% or 125% on a 1x screen that is half a device pixel, where
// an image would be drawn back resampled, softer. There the layers draw directly, as sharp as ever.
TEST_CASE ("Starting half a device pixel in, at 75% and 125% on a 1x screen, a cached layer and a cached soft shadow draw directly")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const float scale = GENERATE (0.75f, 1.25f);
    INFO ("scale " << scale);
    const juce::Point<float> origin { 0.0f, 78.0f }; // where the drawing starts in the window
    const auto text = [&] (juce::Graphics& g) {
        g.setFont (staple::font (staple::tokens::size::fs2));
        g.setColour (staple::tokens::colour::text2);
        g.drawText ("20k", juce::Rectangle<float> (20.0f, 30.0f, 48.0f, 14.0f), juce::Justification::centred, false);
    };
    const auto handle = juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ 50.0f, 20.0f });
    const auto frame = [&] (auto&& draw) {
        return harness::paintImage (juce::roundToInt (100 * scale), juce::roundToInt (160 * scale), [&] (juce::Graphics& g) {
            g.fillAll (juce::Colour (0xffc08020));
            g.addTransform (juce::AffineTransform::translation (origin).scaled (scale));
            draw (g);
        }, juce::NativeImageType());
    };

    const auto direct = frame ([&] (juce::Graphics& g) {
        text (g);
        staple::drawSoftShadow (g, handle, 8.0f, staple::tokens::shadow::handle);
    });
    staple::CachedLayer<int> layer;
    staple::SoftShadowCache shadows;
    const auto cached = [&] (juce::Graphics& g) {
        layer.draw (g, origin, { 10.0f, 25.0f, 70.0f, 25.0f }, 0, juce::Image::ARGB, text);
        shadows.draw (g, origin, handle, 8.0f, staple::tokens::shadow::handle);
    };
    CHECK (largestColourDifference (direct, frame (cached)) <= 2);
    CHECK (largestColourDifference (direct, frame (cached)) <= 2);
}

TEST_CASE ("A Tween counts as easing from towards() until it lands, and not after a jump or once it is gone")
{
    juce::ScopedJuceInitialiser_GUI juce;
    CHECK (staple::Tween::numberEasing() == 0);
    {
        staple::Tween a (100, 0.0f), b (100, 0.0f);
        a.towards (1.0f);
        b.towards (1.0f);
        CHECK (staple::Tween::numberEasing() == 2);
        a.jump (1.0f);
        CHECK (staple::Tween::numberEasing() == 1);
        CHECK (harness::settleUntil ([&b] { return b.value() == 1.0f; }));
        CHECK (staple::Tween::numberEasing() == 0);
        b.towards (0.0f);
        b.towards (0.0f); // already heading there
        CHECK (staple::Tween::numberEasing() == 1);
    }
    CHECK (staple::Tween::numberEasing() == 0);
}

TEST_CASE ("Stand-ins for the OS's pointer nest: the innermost answers, and each going hands back to the one outside it")
{
    juce::ScopedJuceInitialiser_GUI juce;
    using Source = staple::TipWatchdog::ScopedPointerSource;
    const juce::Point<float> outerAt { -1000.0f, -1000.0f }, innerAt { -2000.0f, -2000.0f };
    {
        const Source outer ([outerAt] { return outerAt; });
        CHECK (staple::TipWatchdog::pointer() == outerAt);
        {
            const Source inner ([innerAt] { return innerAt; });
            CHECK (staple::TipWatchdog::pointer() == innerAt);
        }
        CHECK (staple::TipWatchdog::pointer() == outerAt);
    }
    // The OS's pointer again, which is on a screen.
    CHECK (staple::TipWatchdog::pointer() != outerAt);
}

// A Tween's frames run on a repeating timer, which a loaded machine's stalled timer thread fires once for all the frames
// it missed: the bound is in frames, not wall time. (The Tooltip's fade, over TooltipWindow::fadeOutMs, is one.)
TEST_CASE ("A Tween lands on its target on its first frame after its duration, however late that frame runs")
{
    juce::ScopedJuceInitialiser_GUI juce;
    const bool stalled = GENERATE (false, true);
    INFO ((stalled ? "its frames held up past its duration" : "its frames on time"));
    constexpr int durationMs = 100;
    staple::Tween tween (durationMs, 0.0f);
    struct Frame
    {
        double at;
        float value;
        bool easing;
    };
    std::vector<Frame> frames;
    tween.apply = [&frames] (float value) {
        frames.push_back ({ juce::Time::getMillisecondCounterHiRes(), value, staple::Tween::numberEasing() > 0 });
    };
    const auto began = juce::Time::getMillisecondCounterHiRes();
    tween.towards (1.0f);
    if (stalled)
        juce::Thread::sleep (2 * durationMs); // the message thread busy: no frame runs meanwhile

    REQUIRE (harness::settleUntil ([] { return staple::Tween::numberEasing() == 0; }));
    REQUIRE_FALSE (frames.empty());
    CHECK (juce::exactlyEqual (frames.back().value, 1.0f));
    CHECK_FALSE (frames.back().easing);
    CHECK (frames.back().at - began >= durationMs);
    for (size_t i = 0; i + 1 < frames.size(); ++i)
        CHECK (frames[i].easing);
    if (stalled)
        CHECK (frames.size() == 1);
}
