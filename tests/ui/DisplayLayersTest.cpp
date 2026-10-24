// Copied from eq1's tests/plugin/DisplayLayersTest.cpp (eq1 at dbb1127), against plugkit::ui::display: its grid and
// edge-fade cases, with eq1's narrow display width and fade distances passed in, which plugkit's layers take as
// arguments, a case on paintGrid's overload for any GridLines, one on spectrumYAt and one on paintAnalyzer's optional
// Sidechain. Its Band curve cases, on eq1's CurvesLayer, stay in eq1.

#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/display/AnalyzerLayer.h"
#include "plugkit/ui/display/EdgeFadeLayer.h"
#include "plugkit/ui/display/GridLayer.h"
#include "plugkit/ui/staple/Tokens.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

using Catch::Matchers::WithinAbs;
using plugkit::ui::display::DisplayGeometry;

namespace
{
const juce::String minus = juce::String::charToString (0x2212);

// eq1's display: the grid narrow below 800 px, and its edges fading over 18 px at the top, 84 at the bottom, 36 on the
// left and 56 on the right (eq1's tokens::layout).
constexpr int narrowWidth = 800;
constexpr plugkit::ui::display::EdgeFades fades { .top = 18.0f, .bottom = 84.0f, .left = 36.0f, .right = 56.0f };

std::vector<juce::String> texts (const std::vector<plugkit::ui::display::Label>& labels)
{
    std::vector<juce::String> out;
    for (const auto& label : labels)
        out.push_back (label.text);
    return out;
}

std::vector<plugkit::ui::display::Label> withColour (const std::vector<plugkit::ui::display::Label>& labels, juce::Colour colour)
{
    std::vector<plugkit::ui::display::Label> out;
    std::copy_if (labels.begin(), labels.end(), std::back_inserter (out), [&] (const auto& l) { return l.colour == colour; });
    return out;
}

// A spectrum of a full-scale sine on an FFT bin at 48 kHz, so it reads its full level over its Frequency.
struct SineSpectrum
{
    plugkit::ui::AnalyzerSpectrum spectrum;
    double frequency = 0.0;
};

void fillWithSine (SineSpectrum& sine)
{
    const auto size = plugkit::ui::AnalyzerSpectrum::fftSize (48000.0, plugkit::ui::AnalyzerResolution::high);
    sine.spectrum.prepare (48000.0, plugkit::ui::AnalyzerResolution::high);
    sine.frequency = 128.0 * 48000.0 / size;
    std::vector<float> block (static_cast<size_t> (size));
    for (size_t i = 0; i < block.size(); ++i)
        block[i] = static_cast<float> (std::sin (2.0 * std::numbers::pi * sine.frequency * static_cast<double> (i) / 48000.0));
    sine.spectrum.push (block.data(), static_cast<int> (block.size()));
    sine.spectrum.update (10.0, plugkit::ui::AnalyzerSpeed::veryFast);
}
} // namespace

TEST_CASE ("The grid's Gain lines are a third of the Display Range apart, with the 0 dB line apart from them")
{
    const auto [range, step] = GENERATE (std::pair { 6, 2.0 }, std::pair { 12, 4.0 }, std::pair { 30, 10.0 });
    const DisplayGeometry geometry { .width = 1134, .height = 612, .rangeDb = range };
    const auto lines = plugkit::ui::display::gridLines (geometry);
    std::vector<float> expected;
    for (double db : { -3.0, -2.0, -1.0, 1.0, 2.0, 3.0 })
        expected.push_back (geometry.yOf (db * step));
    REQUIRE (lines.gainY.size() == expected.size());
    for (size_t i = 0; i < expected.size(); ++i)
        CHECK_THAT (lines.gainY[i], WithinAbs (expected[i], 0.01));
    CHECK_THAT (lines.zeroY, WithinAbs (geometry.yOf (0.0), 0.01));
}

TEST_CASE ("The grid's Frequency lines run 20 Hz to 20 kHz: majors at 20, 50, 100 ..., minors between, hidden below 800 px")
{
    const DisplayGeometry wide { .width = 1134, .height = 612, .narrowWidth = narrowWidth };
    const auto lines = plugkit::ui::display::gridLines (wide);
    REQUIRE (lines.majorX.size() == 10);
    CHECK_THAT (lines.majorX.front(), WithinAbs (wide.xOf (20.0), 0.01));
    CHECK_THAT (lines.majorX.back(), WithinAbs (wide.xOf (20000.0), 0.01));
    REQUIRE (lines.minorX.size() == 18);
    CHECK_THAT (lines.minorX.front(), WithinAbs (wide.xOf (30.0), 0.01));
    CHECK_THAT (lines.minorX.back(), WithinAbs (wide.xOf (9000.0), 0.01));

    const DisplayGeometry narrow { .width = 799, .height = 612, .narrowWidth = narrowWidth };
    CHECK (plugkit::ui::display::gridLines (narrow).minorX.empty());
    CHECK (plugkit::ui::display::gridLines (narrow).majorX.size() == 10);
}

TEST_CASE ("Any grid's lines paint where they are given, across the size given: the minor, the major and the zero line in their colours")
{
    namespace colour = staple::tokens::colour;
    // Not a Frequency grid: lines placed by hand, as a time grid places its own.
    const plugkit::ui::display::GridLines lines { .majorX = { 10.0f, 50.0f }, .minorX = { 30.0f }, .gainY = { 20.0f, 60.0f }, .zeroY = 40.0f };
    const auto image = harness::paintImage (100, 80, [&] (juce::Graphics& g) { plugkit::ui::display::paintGrid (g, lines, { 100, 80 }); });
    const auto drawn = [&] (int x, int y) { return image.getPixelAt (x, y).getAlpha() > 0; };
    const auto near = [] (juce::Colour actual, juce::Colour expected) {
        return std::abs (actual.getAlpha() - expected.getAlpha()) <= 1 && std::abs (actual.getRed() - expected.getRed()) <= 8
               && std::abs (actual.getGreen() - expected.getGreen()) <= 8 && std::abs (actual.getBlue() - expected.getBlue()) <= 8;
    };

    // Across a row between the horizontal lines, only the vertical ones.
    std::vector<int> columns, rows;
    for (int x = 0; x < 100; ++x)
        if (drawn (x, 5))
            columns.push_back (x);
    CHECK (columns == std::vector<int> { 10, 30, 50 });
    // Down a column between the vertical lines, only the horizontal ones.
    for (int y = 0; y < 80; ++y)
        if (drawn (80, y))
            rows.push_back (y);
    CHECK (rows == std::vector<int> { 20, 40, 60 });

    CHECK (near (image.getPixelAt (10, 5), colour::gridMajor));
    CHECK (near (image.getPixelAt (30, 5), colour::gridMinor));
    CHECK (near (image.getPixelAt (80, 20), colour::gridMajor));
    CHECK (near (image.getPixelAt (80, 40), colour::gridZero));
    // Edge to edge of the size.
    CHECK (drawn (50, 79));
    CHECK (drawn (99, 60));
}

TEST_CASE ("The Frequency labels sit 10 px above the bottom, centred on their lines, the first and last pulled 12 px in")
{
    namespace colour = staple::tokens::colour;
    const DisplayGeometry geometry { .width = 1134, .height = 612, .narrowWidth = narrowWidth };
    const auto labels = withColour (plugkit::ui::display::gridLabels (geometry), colour::text2);
    // The 0 dB label is text2 too: the Frequency labels are the ones at the bottom.
    std::vector<plugkit::ui::display::Label> frequency;
    std::copy_if (labels.begin(), labels.end(), std::back_inserter (frequency), [] (const auto& l) { return l.area.getBottom() > 590.0f; });
    CHECK (texts (frequency) == std::vector<juce::String> { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" });
    for (const auto& label : frequency)
        CHECK_THAT (label.area.getBottom(), WithinAbs (602.0, 0.01));
    CHECK_THAT (frequency.front().area.getX(), WithinAbs (geometry.xOf (20.0) + 12.0f, 0.01));
    CHECK (frequency.front().justification == juce::Justification::centredLeft);
    CHECK_THAT (frequency.back().area.getRight(), WithinAbs (geometry.xOf (20000.0) - 12.0f, 0.01));
    CHECK (frequency.back().justification == juce::Justification::centredRight);
    CHECK_THAT (frequency[5].area.getCentreX(), WithinAbs (geometry.xOf (1000.0), 0.01));
    CHECK (frequency[5].justification == juce::Justification::centred);

    SECTION ("below 800 px, every other one is hidden")
    {
        const DisplayGeometry narrow { .width = 799, .height = 612, .narrowWidth = narrowWidth };
        std::vector<plugkit::ui::display::Label> shown;
        for (const auto& l : plugkit::ui::display::gridLabels (narrow))
            if (l.area.getBottom() > 590.0f)
                shown.push_back (l);
        CHECK (texts (shown) == std::vector<juce::String> { "20", "100", "500", "2k", "10k" });
    }
}

TEST_CASE ("frequencyLabels gives the Frequency labels alone, as gridLabels draws them")
{
    const auto width = GENERATE (1134, 799);
    const DisplayGeometry geometry { .width = width, .height = 612, .narrowWidth = narrowWidth };
    const auto labels = plugkit::ui::display::frequencyLabels (geometry);
    CHECK (texts (labels)
           == (width < narrowWidth ? std::vector<juce::String> { "20", "100", "500", "2k", "10k" }
                                   : std::vector<juce::String> { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" }));
    for (const auto& label : labels)
        CHECK_THAT (label.area.getBottom(), WithinAbs (602.0, 0.01));
    // gridLabels' Frequency labels are these, at the bottom, ahead of the Gain labels.
    std::vector<plugkit::ui::display::Label> bottom;
    for (const auto& l : plugkit::ui::display::gridLabels (geometry))
        if (l.area.getBottom() > 590.0f)
            bottom.push_back (l);
    CHECK (bottom == labels);
}

TEST_CASE ("The Gain labels sit 42 px from the right, signed, 0 brighter than the rest, without +/- the Display Range")
{
    namespace colour = staple::tokens::colour;
    const auto [range, expected] = GENERATE_COPY (
        std::pair { 6, std::vector<juce::String> { "+4", "+2", "0", minus + "2", minus + "4" } },
        std::pair { 12, std::vector<juce::String> { "+8", "+4", "0", minus + "4", minus + "8" } },
        std::pair { 30, std::vector<juce::String> { "+20", "+10", "0", minus + "10", minus + "20" } });
    const DisplayGeometry geometry { .width = 1134, .height = 612, .rangeDb = range };
    std::vector<plugkit::ui::display::Label> gain;
    for (const auto& l : plugkit::ui::display::gridLabels (geometry))
        if (l.area.getBottom() <= 590.0f)
            gain.push_back (l);
    CHECK (texts (gain) == expected);
    for (const auto& label : gain)
    {
        CHECK_THAT (label.area.getRight(), WithinAbs (1134.0 - 42.0, 0.01));
        CHECK (label.justification == juce::Justification::centredRight);
        CHECK (label.colour == (label.text == "0" ? colour::text2 : colour::text3));
    }
    CHECK_THAT (gain[1].area.getCentreY(), WithinAbs (geometry.yOf (range / 3.0), 0.01));
}

TEST_CASE ("The Analyzer's dB scale runs in 10 dB steps at 60 and 90 dB, 20 dB at 120, clear of the top and bottom")
{
    const DisplayGeometry geometry { .width = 1134, .height = 612 };
    const auto [range, expected] = GENERATE_COPY (
        std::pair { 60, std::vector<juce::String> { minus + "10", minus + "20", minus + "30", minus + "40", minus + "50" } },
        std::pair { 90, std::vector<juce::String> { minus + "10", minus + "20", minus + "30", minus + "40", minus + "50", minus + "60", minus + "70", minus + "80" } },
        std::pair { 120, std::vector<juce::String> { minus + "20", minus + "40", minus + "60", minus + "80", minus + "100" } });
    plugkit::ui::AnalyzerSettings settings;
    settings.rangeDb = range;
    const auto labels = plugkit::ui::display::analyzerScaleLabels (geometry, settings);
    CHECK (texts (labels) == expected);
    for (const auto& label : labels)
    {
        CHECK_THAT (label.area.getRight(), WithinAbs (1134.0 - 10.0, 0.01));
        CHECK (label.colour == staple::tokens::colour::text4);
        CHECK (label.area.getCentreY() > 40.0f);
        CHECK (label.area.getCentreY() < 612.0f - 30.0f);
    }
    // The top of the display is 0 dB on the Analyzer's scale.
    CHECK_THAT (labels[0].area.getCentreY(), WithinAbs ((range > 90 ? 20.0 : 10.0) / range * 612.0, 0.01));

    SECTION ("hidden when the Analyzer shows nothing")
    {
        settings.showPreEq = settings.showPostEq = settings.showSidechain = false;
        CHECK (plugkit::ui::display::analyzerScaleLabels (geometry, settings).empty());
    }
}

TEST_CASE ("The display's edges fade into what lies behind it over 18 px at the top, 84 at the bottom, 36 on the left and 56 on the right")
{
    const float scale = GENERATE (1.0f, 1.25f, 2.0f);
    const DisplayGeometry geometry { .width = 600, .height = 400 };
    // Behind the display: black on the left to blue on the right, so not flat.
    const auto behind = [] (juce::Graphics& g) {
        g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.0f, 0.0f, juce::Colours::blue, 600.0f, 0.0f, false));
        g.fillAll();
    };
    const auto overlay = plugkit::ui::display::edgeFadeOverlay (geometry, fades, scale, behind);
    const auto image = harness::paintImage (juce::roundToInt (600 * scale), juce::roundToInt (400 * scale), [&] (juce::Graphics& g) {
        g.addTransform (juce::AffineTransform::scale (scale));
        g.fillAll (juce::Colours::white);
        plugkit::ui::display::paintEdgeFades (g, overlay);
    });
    const auto pixel = [&] (int x, int y) { return image.getPixelAt (juce::roundToInt (static_cast<float> (x) * scale), juce::roundToInt (static_cast<float> (y) * scale)); };
    // How much of the white is left: 1 untouched, 0 covered by what lies behind (which has no red).
    const auto whiteLeft = [&] (int x, int y) { return pixel (x, y).getFloatRed(); };
    CHECK (whiteLeft (300, 200) > 0.99f);
    // Clear at each edge, half-way across each fade's distance, and untouched just past it.
    const int cx = 300, cy = 200;
    CHECK (whiteLeft (cx, 0) < 0.06f);
    CHECK_THAT (whiteLeft (cx, 9), WithinAbs (0.5, 0.08));
    CHECK (whiteLeft (cx, 19) > 0.97f);
    CHECK (whiteLeft (cx, 399) < 0.03f);
    CHECK_THAT (whiteLeft (cx, 400 - 42), WithinAbs (0.5, 0.05));
    CHECK (whiteLeft (cx, 400 - 85) > 0.97f);
    CHECK (whiteLeft (0, cy) < 0.04f);
    CHECK_THAT (whiteLeft (18, cy), WithinAbs (0.5, 0.06));
    CHECK (whiteLeft (37, cy) > 0.97f);
    CHECK (whiteLeft (599, cy) < 0.03f);
    CHECK_THAT (whiteLeft (600 - 28, cy), WithinAbs (0.5, 0.05));
    CHECK (whiteLeft (600 - 57, cy) > 0.97f);
    // At an edge, what lies behind shows as it is there: half blue in the middle, nearly all at the right.
    CHECK_THAT (pixel (cx, 0).getFloatBlue(), WithinAbs (0.5, 0.06));
    CHECK (pixel (599, cy).getFloatBlue() > 0.95f);
}

TEST_CASE ("The display's edge fades are drawn back pixel for pixel at a UI Scale whose display isn't whole device pixels")
{
    // 601 x 401 at 1.25: 751.25 x 501.25 device pixels.
    const float scale = 1.25f;
    const DisplayGeometry geometry { .width = 601, .height = 401 };
    const auto overlay = plugkit::ui::display::edgeFadeOverlay (geometry, fades, scale, [] (juce::Graphics& g) {
        g.setGradientFill (juce::ColourGradient (juce::Colours::black, 0.0f, 0.0f, juce::Colours::blue, 601.0f, 0.0f, false));
        g.fillAll();
    });
    const auto image = harness::paintImage (752, 502, [&] (juce::Graphics& g) {
        g.addTransform (juce::AffineTransform::scale (scale));
        g.fillAll (juce::Colours::white);
        plugkit::ui::display::paintEdgeFades (g, overlay);
    });
    // Each strip's pixels land on the device pixels they were made for, where the fades change fastest.
    for (const auto& strip : overlay.strips)
    {
        REQUIRE (strip.image.isValid());
        for (int y = 0; y < strip.image.getHeight(); y += 3)
            for (int x = 0; x < strip.image.getWidth(); x += 3)
            {
                const auto expected = juce::Colours::white.overlaidWith (strip.image.getPixelAt (x, y));
                const auto drawn = image.getPixelAt (strip.at.x + x, strip.at.y + y);
                CHECK (std::abs (drawn.getRed() - expected.getRed()) <= 1);
                CHECK (std::abs (drawn.getBlue() - expected.getBlue()) <= 1);
            }
    }
}

TEST_CASE ("A spectrum is drawn on the Analyzer's scale: 0 dB at the top, its range below at the bottom, with Analyzer Tilt")
{
    const DisplayGeometry geometry { .width = 1134, .height = 612 };
    plugkit::ui::AnalyzerSettings settings;
    settings.rangeDb = 60;
    settings.tiltDbPerOctave = 0.0;
    plugkit::ui::AnalyzerSpectrum spectrum;
    spectrum.prepare (48000.0, plugkit::ui::AnalyzerResolution::high);
    // Silent: the bottom, and nothing lower.
    CHECK_THAT (plugkit::ui::display::spectrumYAt (geometry, settings, spectrum, 500.0f), WithinAbs (612.0, 1.0e-3));

    // A full-scale sine, on an FFT bin so it reads its full level: at the top over its Frequency.
    SineSpectrum sine;
    fillWithSine (sine);
    const double frequency = sine.frequency;
    const float x = geometry.xOf (frequency);
    CHECK_THAT (plugkit::ui::display::spectrumYAt (geometry, settings, sine.spectrum, x), WithinAbs (0.0, 0.5));

    // Tilted to 30 dB down over its Frequency, below 1 kHz: half-way down.
    settings.tiltDbPerOctave = -30.0 / std::log2 (frequency / 1000.0);
    CHECK_THAT (plugkit::ui::display::spectrumYAt (geometry, settings, sine.spectrum, x), WithinAbs (306.0, 0.5));
}

TEST_CASE ("The Sidechain spectrum is optional: shown, it is drawn when given and nothing is drawn for it when not")
{
    namespace colour = staple::tokens::colour;
    const DisplayGeometry geometry { .width = 400, .height = 200 };
    plugkit::ui::AnalyzerSettings settings;
    settings.showPreEq = false;
    settings.showPostEq = false;
    settings.showSidechain = true;
    settings.tiltDbPerOctave = 0.0;
    plugkit::ui::AnalyzerSpectrum silent;
    silent.prepare (48000.0, plugkit::ui::AnalyzerResolution::high);
    SineSpectrum sidechain;
    fillWithSine (sidechain);
    const auto drawnPixels = [] (const juce::Image& image) {
        int drawn = 0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                drawn += image.getPixelAt (x, y).getAlpha() > 0 ? 1 : 0;
        return drawn;
    };

    SECTION ("given, as eq1 passes it: drawn in the Sidechain's tint, high over the sine")
    {
        const auto image = harness::paintImage (400, 200, [&] (juce::Graphics& g) {
            plugkit::ui::display::paintAnalyzer (g, geometry, { .settings = settings, .preEq = silent, .postEq = silent, .sidechain = sidechain.spectrum });
        });
        // The topmost pixel drawn over the sine.
        const int x = juce::roundToInt (geometry.xOf (sidechain.frequency));
        int y = 0;
        while (y < image.getHeight() && image.getPixelAt (x, y).getAlpha() == 0)
            ++y;
        REQUIRE (y < image.getHeight() / 2); // far above the silent bottom, near 0 dB
        const auto top = image.getPixelAt (x, y);
        CHECK (std::abs (top.getRed() - colour::anScLine.getRed()) <= 8);
        CHECK (std::abs (top.getGreen() - colour::anScLine.getGreen()) <= 8);
        CHECK (std::abs (top.getBlue() - colour::anScLine.getBlue()) <= 8);
    }
    SECTION ("not given: nothing drawn")
    {
        const auto image = harness::paintImage (400, 200, [&] (juce::Graphics& g) {
            plugkit::ui::display::paintAnalyzer (g, geometry, { .settings = settings, .preEq = silent, .postEq = silent });
        });
        CHECK (drawnPixels (image) == 0);
    }
}

TEST_CASE ("A spectrum's line runs across the display in one path, a point every 2 px, each on the Analyzer's scale")
{
    const DisplayGeometry geometry { .width = 1134, .height = 612 };
    plugkit::ui::AnalyzerSettings settings;
    settings.rangeDb = 60;
    settings.tiltDbPerOctave = 0.0;
    plugkit::ui::AnalyzerSpectrum spectrum;
    spectrum.prepare (48000.0, plugkit::ui::AnalyzerResolution::high);

    const auto pointsOf = [] (const juce::Path& line) {
        std::vector<juce::Point<float>> points;
        int subPaths = 0;
        for (juce::Path::Iterator it (line); it.next();)
        {
            if (it.elementType == juce::Path::Iterator::startNewSubPath)
                ++subPaths;
            points.emplace_back (it.x1, it.y1);
        }
        CHECK (subPaths == 1);
        return points;
    };

    // Silent: flat along the bottom, from x = 0 to the right edge, 2 px apart.
    const auto silent = pointsOf (plugkit::ui::display::spectrumLine (geometry, settings, spectrum));
    REQUIRE (silent.size() == 568);
    for (size_t i = 0; i < silent.size(); ++i)
    {
        CHECK_THAT (silent[i].x, WithinAbs (2.0 * static_cast<double> (i), 1.0e-3));
        CHECK_THAT (silent[i].y, WithinAbs (612.0, 1.0e-3));
    }

    // A full-scale sine: every point where spectrumYAt puts it, and the line near the top over its Frequency (within
    // 2 dB, 20 px: the 2 px steps fall either side of its peak).
    SineSpectrum sineSpectrum;
    fillWithSine (sineSpectrum);
    const auto sine = pointsOf (plugkit::ui::display::spectrumLine (geometry, settings, sineSpectrum.spectrum));
    REQUIRE (sine.size() == 568);
    for (const auto& point : sine)
        CHECK_THAT (point.y, WithinAbs (plugkit::ui::display::spectrumYAt (geometry, settings, sineSpectrum.spectrum, point.x), 1.0e-3));
    const auto top = std::min_element (sine.begin(), sine.end(), [] (auto a, auto b) { return a.y < b.y; });
    CHECK_THAT (top->y, WithinAbs (0.0, 20.0));
    CHECK_THAT (top->x, WithinAbs (geometry.xOf (sineSpectrum.frequency), 2.0));
}
