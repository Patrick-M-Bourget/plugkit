#pragma once

// A model of a Band, the one the tests share: plugkit's pieces put together as eq1's Band puts them (eq1 at dbb1127:
// engine/src/Band.cpp, Band::prepare, Band::startRun, Band::Chain::runPlaced, runMidSide, run and runLanes), for a
// test that needs what a Band does with them over time rather than one piece on its own.
//
// Modelled: the Band's Shape at its Gain (designBand), Frequency, Gain and Q gliding on the run grid (a Smoother each,
// Frequency and Q in logs, the filter designed once a run and each section's coefficients interpolated by position
// in the run), and its Stereo Placement (routePlacement, a Mid or Side part encoded, filtered and decoded). A Stereo
// Band on stereo runs both channels in one loop, a Double2 lane each, as runLanes does; every other part runs a
// channel at a time. Left out: a change of structure (Shape, Slope or Brickwall), which a Band crossfades; skipping
// identity sections; dynamics; Mix.

#include "plugkit/BandShape.h"
#include "plugkit/Biquad.h"
#include "plugkit/Double2.h"
#include "plugkit/Placement.h"
#include "plugkit/RunGrid.h"
#include "plugkit/ShapeDesign.h"
#include "plugkit/Smoother.h"

#include <array>
#include <cassert>
#include <cmath>
#include <vector>

namespace plugkit::test
{

class BandModel
{
public:
    BandModel (double rate, int channels, StereoPlacement where = StereoPlacement::Stereo)
        : sampleRate (rate), placement (where), states (static_cast<size_t> (channels))
    {
        // As eq1's Band::prepare.
        const double timeConstant = glideTimeConstantSeconds * sampleRate;
        logFrequency.configure (timeConstant, 1.0e-5);
        gain.configure (timeConstant, 1.0e-4);
        logQ.configure (timeConstant, 1.0e-5);
    }

    // The first shape is taken at once, as a silent Band takes it; later ones glide. A later shape keeps the first's
    // structure (structureOf): a Band crossfades a new one, which isn't modelled.
    void setShape (const BandShape& shape)
    {
        if (cascade.count == 0)
        {
            target = shape;
            logFrequency.reset (std::log (shape.frequency));
            gain.reset (shape.gain);
            logQ.reset (std::log (shape.q));
            design();
            return;
        }
        assert (structureOf (shape) == structureOf (target));
        target = shape;
        logFrequency.setTarget (std::log (shape.frequency));
        gain.setTarget (shape.gain);
        logQ.setTarget (std::log (shape.q));
    }

    // Filters count samples of each channel of signal in place from start, a whole number of runs.
    void process (std::vector<std::vector<float>>& signal, size_t start, size_t count)
    {
        assert (signal.size() == states.size() && count % runLength == 0);
        for (size_t run = start; run < start + count; run += runLength)
        {
            startRun();
            routePlacement (
                placement,
                static_cast<int> (signal.size()),
                [&] (int, int ch) {
                    if (placement == StereoPlacement::Stereo && signal.size() == 2)
                    {
                        if (ch == 0)
                            runLanes (signal[0].data() + run, signal[1].data() + run);
                        return;
                    }
                    runLine (signal[static_cast<size_t> (ch)].data() + run, static_cast<size_t> (ch));
                },
                [&] (bool mid) { runMidSide (signal[0].data() + run, signal[1].data() + run, mid); });
        }
    }

    void process (std::vector<std::vector<float>>& signal) { process (signal, 0, signal[0].size()); }

private:
    using States = std::array<BiquadState, maxSections>;
    static constexpr auto runLength = static_cast<size_t> (runSamples);

    void design()
    {
        auto shape = target;
        shape.frequency = std::exp (logFrequency.value());
        shape.gain = gain.value();
        shape.q = std::exp (logQ.value());
        cascade = designBand (shape, sampleRate);
    }

    // As eq1's Band::startRun: a gliding run designs its filter at the run's end and glides into it from the last.
    void startRun()
    {
        gliding = logFrequency.isMoving() || gain.isMoving() || logQ.isMoving();
        if (gliding)
        {
            from = cascade;
            logFrequency.skip (runSamples);
            gain.skip (runSamples);
            logQ.skip (runSamples);
            design();
        }
    }

    BiquadCoefficients coefficients (size_t section, size_t i) const
    {
        const auto& to = cascade.sections[section];
        return gliding ? interpolate (from.sections[section], to, (static_cast<double> (i) + 1.0) / runSamples) : to;
    }

    // One run of one line (a channel, or a Mid or Side part) through the filter, with the states of channel ch.
    void runLine (float* samples, size_t ch)
    {
        for (size_t s = 0; s < static_cast<size_t> (cascade.count); ++s)
            for (size_t i = 0; i < runLength; ++i)
                samples[i] = states[ch][s].process (coefficients (s, i), samples[i]);
    }

    // One run of Left and Right through the filter in one loop, a Double2 lane each, as eq1's Band::Chain::runLanes.
    void runLanes (float* left, float* right)
    {
        for (size_t s = 0; s < static_cast<size_t> (cascade.count); ++s)
        {
            auto& state0 = states[0][s];
            auto& state1 = states[1][s];
            Double2 s1 = make2<Double2> (state0.s1, state1.s1), s2 = make2<Double2> (state0.s2, state1.s2);
            for (size_t i = 0; i < runLength; ++i)
            {
                const auto c = coefficients (s, i);
                const Double2 y = biquadStep (fill2<Double2> (c.b0), fill2<Double2> (c.b1), fill2<Double2> (c.b2), fill2<Double2> (c.a1),
                                              fill2<Double2> (c.a2), make2<Double2> (left[i], right[i]), s1, s2);
                left[i] = static_cast<float> (lane0 (y));
                right[i] = static_cast<float> (lane1 (y));
            }
            state0 = { lane0 (s1), lane0 (s2) };
            state1 = { lane1 (s1), lane1 (s2) };
        }
    }

    // One run of a Mid or Side part, encoded from Left and Right, filtered with channel 0's states, and decoded back.
    void runMidSide (float* left, float* right, bool mid)
    {
        std::array<float, runSamples> midSamples, sideSamples;
        for (size_t i = 0; i < runLength; ++i)
        {
            midSamples[i] = encodeMid (left[i], right[i]);
            sideSamples[i] = encodeSide (left[i], right[i]);
        }
        runLine (mid ? midSamples.data() : sideSamples.data(), 0);
        for (size_t i = 0; i < runLength; ++i)
        {
            left[i] = decodeLeft (midSamples[i], sideSamples[i]);
            right[i] = decodeRight (midSamples[i], sideSamples[i]);
        }
    }

    double sampleRate;
    StereoPlacement placement;
    std::vector<States> states;
    Smoother logFrequency, gain, logQ;
    BandShape target;
    Cascade cascade, from;
    bool gliding = false;
};

} // namespace plugkit::test
