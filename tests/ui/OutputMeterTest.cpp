// Copied from eq1's tests/plugin/OutputMeterTest.cpp (eq1 at dbb1127), against plugkit::ui. Its processor cases are
// ClipLights cases here; its meter cases read a test source, MeterProcessor, which sets each channel's Output Level as
// eq1's Engine measures the audio eq1's cases play. With them, the Output Meter's cases from eq1's
// tests/plugin/KeyboardControlTest.cpp ("Space or Return on the Output Meter puts out its Clip Lights") and
// tests/plugin/AccessibilityTest.cpp ("Each Clip Light reads Lit or Off, and pressing either puts out both"). The cases
// on eq1's Engine, editor and session (what the Engine measures, Global Bypass, the output popover's toggle, restoring
// a session, and the Analyzer taps of an editor opening) stay in eq1.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/ClipLights.h"
#include "plugkit/ui/EditorTick.h"
#include "plugkit/ui/OutputMeter.h"
#include "plugkit/ui/staple/Tokens.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

using Catch::Matchers::WithinAbs;
using plugkit::OutputLevel;
using plugkit::levelFloorDb;
using plugkit::ui::ClipLights;
using plugkit::ui::OutputMeter;
using plugkit::ui::OutputMeterChannel;

namespace
{

// A plugin's processor as the Output Meter reads it: each channel's Output Level is what the test plays, the peak
// held until it is read, as an Engine holds it; the Clip Lights are a ClipLights; and its editor is the Output Meter
// in its 40 px rail on the window's background, with the editor's tick.
class MeterProcessor final : public plugkit::test::TestProcessor, public plugkit::ui::OutputLevelSource
{
public:
    int channels = 2;

    // A signal on channel peaking at peakDb, its RMS now rmsDb.
    void play (int channel, double peakDb, double rmsDb)
    {
        auto& level = levels[static_cast<size_t> (channel)];
        level.peakDb = std::max (level.peakDb, peakDb);
        level.rmsDb = rmsDb;
    }
    // The same on every channel.
    void play (double peakDb, double rmsDb)
    {
        for (int ch = 0; ch < channels; ++ch)
            play (ch, peakDb, rmsDb);
    }

    int outputLevelChannels() const override { return channels; }
    OutputLevel readOutputLevel (int channel) override
    {
        auto& level = levels[static_cast<size_t> (channel)];
        const auto read = level;
        level.peakDb = levelFloorDb;
        return clipLights.note (channel, read);
    }
    bool isClipLit (int channel) const override { return clipLights.isLit (channel); }
    void clearClipLights() override { clipLights.clear(); }

    juce::AudioProcessorEditor* createEditor() override;

private:
    std::array<OutputLevel, 2> levels {};
    ClipLights clipLights;
};

class MeterEditor final : public juce::AudioProcessorEditor, public plugkit::ui::EditorTick::Host
{
public:
    explicit MeterEditor (MeterProcessor& p) : AudioProcessorEditor (p), tick (p), meter (p)
    {
        addAndMakeVisible (meter);
        setSize (160, 500);
    }

    plugkit::ui::EditorTick& editorTick() override { return tick; }
    void paint (juce::Graphics& g) override { g.fillAll (staple::tokens::colour::bg0); }
    void resized() override { meter.setBounds (getLocalBounds().withSizeKeepingCentre (40, getHeight())); }

private:
    plugkit::ui::EditorTick tick;
    OutputMeter meter;
};

juce::AudioProcessorEditor* MeterProcessor::createEditor() { return new MeterEditor (*this); }

using Host = harness::OpenEditor<MeterProcessor>;

// A sine's peak and RMS in dBFS, at amplitude.
double peakOf (double amplitude) { return 20.0 * std::log10 (amplitude); }
double rmsOf (double amplitude) { return peakOf (amplitude / std::sqrt (2.0)); }

// The Output Level of an over by a float step: a sample at the next float above full scale.
const double justOver = peakOf (std::nextafter (1.0f, 2.0f));

// Reads every channel's Output Level, as the Output Meter does each frame.
void readAll (MeterProcessor& processor)
{
    for (int ch = 0; ch < processor.outputLevelChannels(); ++ch)
        processor.readOutputLevel (ch);
}

} // namespace

TEST_CASE ("An over lights its channel's Clip Light, which stays lit until cleared, and clearing puts out both")
{
    ClipLights lights;
    // A sample at full scale is not an over; one a float step beyond it is.
    lights.note (0, { .peakDb = 0.0, .rmsDb = -3.0 });
    lights.note (1, { .peakDb = 0.0, .rmsDb = -3.0 });
    CHECK_FALSE (lights.isLit (0));
    CHECK_FALSE (lights.isLit (1));
    lights.note (0, { .peakDb = justOver, .rmsDb = -3.0 });
    CHECK (lights.isLit (0));
    lights.clear();

    lights.note (0, {});
    lights.note (1, { .peakDb = peakOf (1.5), .rmsDb = rmsOf (1.5) });
    CHECK_FALSE (lights.isLit (0));
    CHECK (lights.isLit (1));

    // Quiet again, read again: still lit.
    lights.note (0, { .peakDb = peakOf (0.1), .rmsDb = rmsOf (0.1) });
    lights.note (1, { .peakDb = peakOf (0.1), .rmsDb = rmsOf (0.1) });
    CHECK (lights.isLit (1));

    lights.note (0, { .peakDb = peakOf (1.5), .rmsDb = rmsOf (1.5) });
    REQUIRE (lights.isLit (0));
    lights.clear();
    CHECK_FALSE (lights.isLit (0));
    CHECK_FALSE (lights.isLit (1));
}

TEST_CASE ("An over while nothing reads the Output Level, as with the editor closed, lights the Clip Light on the next read")
{
    juce::ScopedJuceInitialiser_GUI juce;
    MeterProcessor processor;
    processor.play (peakOf (2.0), rmsOf (2.0));
    processor.play (peakOf (0.1), rmsOf (0.1));
    CHECK_FALSE (processor.isClipLit (0));
    readAll (processor);
    CHECK (processor.isClipLit (0));
    CHECK (processor.isClipLit (1));
}

TEST_CASE ("A Clip Light lit on one thread reads lit on another")
{
    ClipLights lights;
    std::atomic<bool> stop { false };
    // One thread lights it over and over while another reads it.
    std::thread lighting ([&] {
        while (! stop)
            lights.note (1, { .peakDb = 6.0 });
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds (10);
    while (! lights.isLit (1) && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    CHECK (lights.isLit (1));
    stop = true;
    lighting.join();
    CHECK_FALSE (lights.isLit (0));
}

TEST_CASE ("An Output Meter bar's peak rises at once and falls at 20 dB/s; its RMS is the RMS read")
{
    OutputMeterChannel channel;
    channel.update ({ .peakDb = -6.0, .rmsDb = -9.0 }, 1.0 / 60.0);
    CHECK_THAT (channel.peakDb(), WithinAbs (-6.0, 1.0e-9));
    CHECK_THAT (channel.rmsDb(), WithinAbs (-9.0, 1.0e-9));
    channel.update ({ .peakDb = levelFloorDb, .rmsDb = -20.0 }, 0.5);
    CHECK_THAT (channel.peakDb(), WithinAbs (-16.0, 1.0e-9));
    CHECK_THAT (channel.rmsDb(), WithinAbs (-20.0, 1.0e-9));
}

TEST_CASE ("An Output Meter bar's held-peak tick holds the highest peak for 1 s, then falls at 20 dB/s, never below the peak")
{
    OutputMeterChannel channel;
    const auto quiet = OutputLevel {};
    channel.update ({ .peakDb = -3.0, .rmsDb = -6.0 }, 0.1);
    for (int frame = 0; frame < 9; ++frame)
        channel.update (quiet, 0.1);
    CHECK_THAT (channel.heldPeakDb(), WithinAbs (-3.0, 1.0e-9)); // 0.9 s on
    CHECK_THAT (channel.peakDb(), WithinAbs (-21.0, 1.0e-9));
    for (int frame = 0; frame < 6; ++frame)
        channel.update (quiet, 0.1);
    CHECK_THAT (channel.heldPeakDb(), WithinAbs (-13.0, 1.0e-6)); // 1.5 s on: falling for 0.5 s

    // A louder peak holds again from where it is.
    channel.update ({ .peakDb = -1.0, .rmsDb = -6.0 }, 0.1);
    channel.update (quiet, 0.9);
    CHECK_THAT (channel.heldPeakDb(), WithinAbs (-1.0, 1.0e-9));
    // Falling, it stays on the peak once it meets it.
    channel.update ({ .peakDb = -2.0, .rmsDb = -6.0 }, 0.5);
    channel.update ({ .peakDb = -2.0, .rmsDb = -6.0 }, 0.5);
    CHECK_THAT (channel.heldPeakDb(), WithinAbs (-2.0, 1.0e-9));
}

TEST_CASE ("The Output Meter repaints only while its levels move: not on silence, and at 60 Hz while a signal's peaks fall")
{
    Host host;
    auto& meter = host.showing<OutputMeter>();
    // Silent since it opened: at the floor, with nothing to redraw.
    REQUIRE (host.settleUntil ([&] { return host.editorTick().rateHz() == plugkit::ui::EditorTick::idleHz; }));
    auto& repaints = harness::countRepaintsOf (meter);
    host.settle (300);
    CHECK (repaints.count == 0);
    REQUIRE (host.editorTick().rateHz() == plugkit::ui::EditorTick::idleHz);

    // A sine, then silence: the peaks fall at 20 dB/s, redrawn at 60 Hz as they fall.
    host.processor.play (peakOf (0.5), rmsOf (0.5));
    host.settle (100);
    const int falling = repaints.count;
    CHECK (falling > 0);
    CHECK (host.editorTick().rateHz() == plugkit::ui::EditorTick::activeHz);
    host.settle (100);
    CHECK (repaints.count > falling);
    meter.setCachedComponentImage (nullptr);
}

TEST_CASE ("The Output Meter's scale runs linearly in dB from -60 dBFS at the bottom to +6 dBFS at the top")
{
    CHECK_THAT (OutputMeter::position (-60.0), WithinAbs (0.0, 1.0e-9));
    CHECK_THAT (OutputMeter::position (6.0), WithinAbs (1.0, 1.0e-9));
    CHECK_THAT (OutputMeter::position (0.0), WithinAbs (60.0 / 66.0, 1.0e-9));
    CHECK_THAT (OutputMeter::position (-27.0), WithinAbs (0.5, 1.0e-9));
    CHECK_THAT (OutputMeter::position (-90.0), WithinAbs (0.0, 1.0e-9));
    CHECK_THAT (OutputMeter::position (levelFloorDb), WithinAbs (0.0, 1.0e-9));
    CHECK_THAT (OutputMeter::position (12.0), WithinAbs (1.0, 1.0e-9));
}

namespace
{

// The Output Meter, height px tall in its 40 px rail, as it paints after reading the Output Level once.
juce::Image paintMeter (OutputMeter& meter, int height)
{
    meter.setBounds (0, 0, 40, height);
    return harness::paintImage (40, height, [&] (juce::Graphics& g) { meter.paintEntireComponent (g, false); });
}

// The columns of row y painted in colour, give or take tolerance on each channel and alpha.
std::vector<int> columnsIn (const juce::Image& image, int y, juce::Colour colour, int tolerance = 4)
{
    std::vector<int> columns;
    for (int x = 0; x < image.getWidth(); ++x)
    {
        const auto pixel = image.getPixelAt (x, y);
        if (std::abs (pixel.getAlpha() - colour.getAlpha()) <= tolerance && std::abs (pixel.getRed() - colour.getRed()) <= tolerance
            && std::abs (pixel.getGreen() - colour.getGreen()) <= tolerance && std::abs (pixel.getBlue() - colour.getBlue()) <= tolerance)
            columns.push_back (x);
    }
    return columns;
}

bool near (juce::Colour actual, juce::Colour expected, int tolerance = 4)
{
    return std::abs (actual.getAlpha() - expected.getAlpha()) <= tolerance && std::abs (actual.getRed() - expected.getRed()) <= tolerance
           && std::abs (actual.getGreen() - expected.getGreen()) <= tolerance && std::abs (actual.getBlue() - expected.getBlue()) <= tolerance;
}

// Where the rail draws db: its bars run from 22 px down (10 padding, the 4 px Clip Lights, an 8 px
// gap) to 12 px above the bottom.
float yOf (double db, int height)
{
    const float top = 22.0f, bottom = static_cast<float> (height - 12);
    return bottom - static_cast<float> (OutputMeter::position (db)) * (bottom - top);
}

} // namespace

TEST_CASE ("The Output Meter's rail: a 6 x 4 Clip Light at the top and a 6 px bar on a dark track per channel, 3 px apart and centred")
{
    juce::ScopedJuceInitialiser_GUI juce;
    MeterProcessor processor;
    processor.channels = GENERATE (1, 2);
    OutputMeter meter (processor);
    const auto image = paintMeter (meter, 500);

    const std::vector<int> expected = processor.channels == 1 ? std::vector<int> { 17, 18, 19, 20, 21, 22 }
                                                              : std::vector<int> { 12, 13, 14, 15, 16, 17, 21, 22, 23, 24, 25, 26 };
    // Silent, the bars show only their track, and the Clip Lights are off.
    CHECK (columnsIn (image, 300, staple::tokens::colour::meterTrack) == expected);
    CHECK (columnsIn (image, 12, staple::tokens::colour::meterClipOff) == expected);
    CHECK (columnsIn (image, 8, staple::tokens::colour::meterClipOff).empty());
    CHECK (columnsIn (image, 15, staple::tokens::colour::meterClipOff).empty());
    // No background.
    CHECK (image.getPixelAt (2, 300).getAlpha() == 0);
}

TEST_CASE ("The Output Meter's colours are fixed to the scale: -10 dBFS RMS draws in the meter2-meter3 blend at any rail height")
{
    juce::ScopedJuceInitialiser_GUI juce;
    MeterProcessor processor;
    // A sine with RMS -10 dBFS (peak -7 dBFS).
    const double amplitude = std::sqrt (2.0) * std::pow (10.0, -0.5);
    processor.play (peakOf (amplitude), rmsOf (amplitude));
    OutputMeter meter (processor);
    // -10 dBFS is 60 % of the way from meter2 at -16 dBFS to meter3 at -6 dBFS.
    const auto expected = staple::tokens::colour::meter2.interpolatedWith (staple::tokens::colour::meter3, 0.6f);
    for (const int height : { 300, 612 })
    {
        CAPTURE (height);
        const auto image = paintMeter (meter, height);
        const auto pixel = image.getPixelAt (14, juce::roundToInt (yOf (-10.0, height)) + 1);
        CAPTURE (pixel.toString(), expected.toString());
        CHECK (near (pixel, expected, 6));
        // Lower down, the colour is the scale's, not the same as at -10 dBFS.
        CHECK_FALSE (near (image.getPixelAt (14, juce::roundToInt (yOf (-40.0, height))), expected, 20));
    }
}

TEST_CASE ("A lit Clip Light draws in meterClip, and clicking either Clip Light puts out both")
{
    juce::ScopedJuceInitialiser_GUI juce;
    MeterProcessor processor;
    processor.play (1, peakOf (2.0), rmsOf (2.0));
    OutputMeter meter (processor);
    auto image = paintMeter (meter, 500);
    CHECK (columnsIn (image, 12, staple::tokens::colour::meterClip) == std::vector<int> { 21, 22, 23, 24, 25, 26 });
    CHECK (columnsIn (image, 12, staple::tokens::colour::meterClipOff) == std::vector<int> { 12, 13, 14, 15, 16, 17 });

    // A click a few px off the unlit left light, within its 10 px tall click area.
    meter.mouseDown (harness::mouseEvent (meter, { 14.0f, 16.0f }, juce::ModifierKeys::leftButtonModifier));
    CHECK_FALSE (processor.isClipLit (0));
    CHECK_FALSE (processor.isClipLit (1));
    image = paintMeter (meter, 500);
    CHECK (columnsIn (image, 12, staple::tokens::colour::meterClip).empty());
}

TEST_CASE ("Output Meter snapshots: a playing signal, and a lit Clip Light", "[.screens]")
{
    Host host;
    const auto write = [&] (const char* name) {
        harness::settle (51); // three frames
        harness::writeSnapshot (*host.editor, juce::String ("meter-") + name);
    };
    // Peaks at -4 dBFS on the left and -7 dBFS on the right.
    host.processor.play (0, peakOf (0.63), rmsOf (0.63));
    host.processor.play (1, peakOf (0.45), rmsOf (0.45));
    write ("playing");
    // An over on the right, then quieter.
    host.processor.play (1, peakOf (1.5), rmsOf (1.5));
    host.processor.play (peakOf (0.5), rmsOf (0.5));
    write ("clip");
}

TEST_CASE ("An Output Meter opening after an over and then silence draws no peak, only its lit Clip Lights")
{
    juce::ScopedJuceInitialiser_GUI juce;
    MeterProcessor processor;
    // An over, then silence for longer than the RMS window: the peak is held until read.
    processor.play (peakOf (2.0), levelFloorDb);
    OutputMeter meter (processor);
    const auto image = paintMeter (meter, 500);
    const std::vector<int> bars { 12, 13, 14, 15, 16, 17, 21, 22, 23, 24, 25, 26 };
    CHECK (columnsIn (image, 300, staple::tokens::colour::meterTrack) == bars);
    CHECK (columnsIn (image, 12, staple::tokens::colour::meterClip) == bars);
}

TEST_CASE ("Space or Return on the Output Meter puts out its Clip Lights")
{
    Host host;
    for (const auto key : { juce::KeyPress (juce::KeyPress::spaceKey), juce::KeyPress (juce::KeyPress::returnKey) })
    {
        host.processor.play (0, peakOf (2.0), rmsOf (2.0));
        readAll (host.processor);
        REQUIRE (host.processor.isClipLit (0));

        host.showing ("Output Meter").grabKeyboardFocus();
        CHECK (host.press (key));
        CHECK_FALSE (host.processor.isClipLit (0));
    }
}

TEST_CASE ("Each Clip Light reads Lit or Off, and pressing either puts out both")
{
    Host host;
    host.processor.play (1, peakOf (2.0), rmsOf (2.0));
    host.settle();
    const auto value = [&] (const juce::String& title) {
        return host.showing (title).getAccessibilityHandler()->getValueInterface()->getCurrentValueAsString();
    };
    CHECK (host.showing ("Clip Light Left").getAccessibilityHandler()->getRole() == juce::AccessibilityRole::button);
    CHECK (value ("Clip Light Left") == "Off");
    CHECK (value ("Clip Light Right") == "Lit");

    auto* left = host.showing ("Clip Light Left").getAccessibilityHandler();
    REQUIRE (left->getActions().invoke (juce::AccessibilityActionType::press));
    CHECK_FALSE (host.processor.isClipLit (1));
    CHECK (value ("Clip Light Right") == "Off");
}
