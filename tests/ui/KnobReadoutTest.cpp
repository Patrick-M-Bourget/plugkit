// When a Knob Readout (staple::KnobTooltip) goes, with the pointer driven through the editor's window as the OS drives
// it (harness::HostWindow): JUCE's MouseInputSource decides which component is under the pointer and sends the enters
// and exits, so the Readout hears of the pointer only when JUCE tells it, as in a host.

#include "TestProcessor.h"
#include "plugkit/test/EditorHarness.h"
#include "plugkit/ui/staple/TipWatchdog.h"
#include "plugkit/ui/staple/controls/Knob.h"
#include "plugkit/ui/staple/controls/KnobTooltip.h"
#include "plugkit/ui/staple/controls/Overlay.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <optional>

namespace
{

using plugkit::test::TestProcessor;

// The test processor's editor in a host's window. It has a Small Knob beside its own two, in the same body, and
// another at the foot of the layer the Readouts show on, whose Readout sits above it; and a label under its Frequency
// knob, which the knob's Readout covers once labelFrequency gives the knob it.
struct Hosted : harness::OpenEditor<TestProcessor>
{
    harness::HostWindow window { *editor };
    staple::Knob small { staple::tokens::knob::small, "Small" }, atFoot { staple::tokens::knob::small, "At Foot" };
    juce::Label label { {}, "Frequency" };
    struct Lane final : staple::Knob::RingHandler
    {
        void paintRing (juce::Graphics&, staple::Knob&, juce::Point<float>, float, float) override {}
    } lane;
    // A ring with its own Readout content, wider than the Gain knob's own, as eq1's Dynamic Range ring has.
    struct WideLane final : staple::Knob::RingHandler
    {
        void paintRing (juce::Graphics&, staple::Knob&, juce::Point<float>, float, float) override {}
        juce::String ringTitle() override { return "Band 4 Dynamic Range"; }
        juce::String ringValue() override { return value; }
        juce::String value { "+6.0 dB" };
    } wideLane;

    Hosted()
    {
        auto& body = *frequency().getParentComponent();
        body.addAndMakeVisible (small);
        small.setBounds (290, 0, small.getIdealSize(), small.getIdealSize());
        REQUIRE (body.getLocalBounds().contains (small.getBounds()));
        auto& layer = staple::overlayLayerFor (small);
        layer.addAndMakeVisible (atFoot);
        // At the layer's foot as far as the window shows it, which may cut the layer short.
        const int foot = std::min (layer.getHeight(), layer.getLocalPoint (&window, juce::Point<int> (0, window.getHeight())).y);
        atFoot.setBounds (300, foot - atFoot.getIdealSize(), atFoot.getIdealSize(), atFoot.getIdealSize());
        body.addAndMakeVisible (label);
        label.setBounds (frequency().getBounds().withY (frequency().getBottom()).withHeight (16));
    }

    ~Hosted()
    {
        for (auto* c : std::initializer_list<juce::Component*> { &small, &atFoot, &label })
            c->getParentComponent()->removeChildComponent (c);
        gain().setRing (nullptr);
    }

    staple::Knob& labelFrequency()
    {
        frequency().setLabel (&label);
        return frequency();
    }

    staple::Knob& frequency() { return knobNamed ("Frequency"); }
    staple::Knob& gain() { return knobNamed ("Gain"); }

    // The Gain knob with a ring: its 12 px lane at r + 10, as eq1's and comp1's Gain knobs have.
    staple::Knob& ringedGain()
    {
        gain().setRing (&lane);
        return gain();
    }

    // The Gain knob with a ring whose Readout content is its own (wideLane).
    staple::Knob& wideRingedGain()
    {
        gain().setRing (&wideLane);
        return gain();
    }

    staple::Knob& knobNamed (const juce::String& name)
    {
        auto found = findAll<staple::Knob> ([&] (staple::Knob& k) { return k.getName() == name; });
        REQUIRE (found.size() == 1);
        return *found.front();
    }

    static juce::Component* underPointer() { return juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse(); }

    // The pointer on knob's face, which shows its Readout.
    void hover (staple::Knob& knob)
    {
        window.pointerAt (knob, knob.getFaceCentre());
        REQUIRE (underPointer() == &knob);
        REQUIRE (knob.isTooltipShown());
    }
};

// A point in knob's bounds but off its round face, beside it: in the margin its shadow takes round the face (20 px at a
// regular Knob, 11 px at a Small one), which its hitTest leaves to its parent.
juce::Point<float> marginBeside (staple::Knob& knob)
{
    const auto centre = knob.getFaceCentre();
    const auto margin = static_cast<float> (knob.getWidth()) - (centre.x + knob.getFaceRadius());
    const juce::Point<float> at { centre.x + knob.getFaceRadius() + margin / 2.0f, centre.y };
    REQUIRE (knob.getLocalBounds().toFloat().contains (at));
    REQUIRE_FALSE (knob.hitTest (juce::roundToInt (at.x), juce::roundToInt (at.y)));
    return at;
}

// A point right of knob, outside its bounds.
juce::Point<float> rightOf (staple::Knob& knob) { return { static_cast<float> (knob.getWidth()) + 10.0f, knob.getFaceCentre().y }; }

} // namespace

TEST_CASE ("A Knob Readout hides when the pointer leaves the knob slowly, through the margin round its face")
{
    Hosted e;
    auto& knob = GENERATE (true, false) ? e.frequency() : e.small;
    INFO (knob.getName());
    e.hover (knob);

    // Off the face into the margin: JUCE sends the knob its exit here, still inside its bounds.
    e.window.pointerAt (knob, marginBeside (knob));
    REQUIRE (Hosted::underPointer() != &knob);
    // On out of the knob: it hears nothing more.
    e.window.pointerAt (knob, rightOf (knob));
    if (&knob == &e.small)
        // The margin point, level with the centre, is on the way to the Readout (the hull of the face and the
        // Readout, which reaches out beside the face there when the Readout is the wider), so the exit kept it: the
        // watchdog hides it on its next check.
        CHECK (harness::settleUntil ([&] { return ! knob.isTooltipShown(); }));
    else
        CHECK_FALSE (knob.isTooltipShown());
}

TEST_CASE ("A Knob Readout hides when its knob, or the knob's parent, is hidden under the pointer")
{
    Hosted e;
    auto& knob = e.frequency();
    e.hover (knob);
    // As comp1's switch between its compact and expanded views hides a knob's parent, or eq1's Band panel closing.
    auto& hidden = GENERATE (true, false) ? static_cast<juce::Component&> (knob) : *knob.getParentComponent();
    INFO ((&hidden == &knob ? "the knob" : "its parent"));
    hidden.setVisible (false);
    e.window.pointerAgain();
    CHECK_FALSE (knob.getKnobTooltip()->isVisible());
}

TEST_CASE ("A Knob Readout stays while the pointer is on it, and hides when it leaves it through the knob's margin")
{
    Hosted e;
    auto& knob = e.frequency();
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();

    e.window.pointerAt (readout, readout.getLocalBounds().toFloat().getCentre());
    REQUIRE (Hosted::underPointer() == &readout);
    CHECK (knob.isTooltipShown());

    e.window.pointerAt (knob, marginBeside (knob));
    REQUIRE (Hosted::underPointer() != &readout);
    REQUIRE (Hosted::underPointer() != &knob);
    CHECK_FALSE (knob.isTooltipShown());
}

TEST_CASE ("A Knob Readout being typed into stays when the pointer leaves it and its knob")
{
    Hosted e;
    auto& knob = e.frequency();
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    e.window.pointerAt (readout, readout.getLocalBounds().toFloat().getCentre());
    readout.startEditing();
    REQUIRE (readout.isEditing());

    e.window.pointerAt (knob, rightOf (knob));
    REQUIRE (Hosted::underPointer() != &readout);
    CHECK (knob.isTooltipShown());
    CHECK (readout.isEditing());
}

// The OS sends the window no event in each case, so JUCE still has the pointer on the knob: only the watchdog, which
// reads the pointer from the OS, sees it has gone.
TEST_CASE ("A Knob Readout hides on its watchdog's check once the OS has the pointer off its knob")
{
    Hosted e;
    auto& knob = e.frequency();
    e.hover (knob);
    using Exit = harness::HostWindow::UnseenExit;
    const auto how = GENERATE (Exit::offTheControl, Exit::overAnotherWindow, Exit::windowHidden);
    INFO ("The pointer is " << harness::HostWindow::describe (how));
    e.window.pointerLeavesUnseen (how, knob, rightOf (knob));

    CHECK (harness::settleUntil ([&] { return ! knob.isTooltipShown(); }));
}

TEST_CASE ("A Knob Readout stays while the pointer rests on its knob, drags it out of the knob, or types into it")
{
    Hosted e;
    auto& knob = e.frequency();
    e.hover (knob);
    enum class Doing
    {
        rests,
        drags,
        types
    };
    const auto doing = GENERATE (Doing::rests, Doing::drags, Doing::types);
    INFO ("The pointer " << (doing == Doing::rests ? "rests" : doing == Doing::drags ? "drags" : "types"));
    std::optional<harness::HostWindow::ButtonHeld> held;
    switch (doing)
    {
        case Doing::rests: break;
        case Doing::drags:
            held.emplace (e.window, knob, knob.getFaceCentre());
            held->dragTo (knob, rightOf (knob));
            break;
        case Doing::types:
            knob.getKnobTooltip()->startEditing();
            e.window.pointerUnseen (knob, rightOf (knob));
            break;
    }
    // Through the watchdog's checks, which fire before the harness's timer as they fall due sooner.
    const harness::VisibilityChanges changes (*knob.getKnobTooltip());
    harness::settle (4 * staple::TipWatchdog::periodMs);
    CHECK (changes.hides == 0);
    CHECK (knob.isTooltipShown());
}

namespace
{
// The pointer walked from a to b, both in the window's coordinates, a few px per event, as a hand moves it slowly;
// each() runs after every step.
void walk (harness::HostWindow& window, juce::Point<float> a, juce::Point<float> b, const std::function<void()>& each)
{
    constexpr float stepPx = 3.0f;
    const int steps = std::max (1, static_cast<int> (std::ceil (a.getDistanceFrom (b) / stepPx)));
    for (int i = 1; i <= steps; ++i)
    {
        window.pointerAtWindow (a + (b - a) * (static_cast<float> (i) / static_cast<float> (steps)));
        each();
    }
}

// The knob with a label, whose Readout covers the label from its top, 20 px under the face; or without, whose Readout
// is 6 px under the face.
staple::Knob& labelledOrNot (Hosted& e, bool labelled) { return labelled ? e.labelFrequency() : e.small; }
} // namespace

TEST_CASE ("A Knob Readout stays while the pointer moves slowly from the knob's face onto it, and a double-click there types")
{
    Hosted e;
    const bool labelled = GENERATE (true, false);
    INFO ((labelled ? "a knob with a label" : "a knob without one"));
    auto& knob = labelledOrNot (e, labelled);
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    const auto onCard = readout.getLocalBounds().toFloat().getCentre();

    // Through the gap between face and Readout, where neither is under the pointer: the knob's margin, its label.
    int inGap = 0;
    walk (e.window, e.window.getLocalPoint (&knob, knob.getFaceCentre()), e.window.getLocalPoint (&readout, onCard), [&] {
        if (Hosted::underPointer() != &knob && Hosted::underPointer() != &readout)
        {
            ++inGap;
            // A pause there, which the watchdog checks.
            if (inGap == 1)
                harness::settle (2 * staple::TipWatchdog::periodMs);
        }
        CHECK (knob.isTooltipShown());
    });
    REQUIRE (inGap > 0);
    REQUIRE (Hosted::underPointer() == &readout);

    for (int click = 0; click < 2; ++click)
    {
        e.window.pointerAt (readout, onCard, juce::ModifierKeys::leftButtonModifier);
        e.window.pointerAt (readout, onCard);
    }
    CHECK (readout.isEditing());
}

TEST_CASE ("A Knob Readout hides once the pointer leaves the way from the knob's face to it sideways")
{
    Hosted e;
    const bool labelled = GENERATE (true, false);
    INFO ((labelled ? "a knob with a label" : "a knob without one"));
    auto& knob = labelledOrNot (e, labelled);
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    const auto face = e.window.getLocalPoint (&knob, knob.getFaceCentre());
    const auto card = e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat();

    // Down into the gap, 2 px above the Readout.
    const juce::Point<float> inGap { face.x, card.getY() - 2.0f };
    walk (e.window, face, inGap, [] {});
    REQUIRE (Hosted::underPointer() != &knob);
    REQUIRE (Hosted::underPointer() != &readout);
    REQUIRE (knob.isTooltipShown());

    // Sideways, past the Readout's edge.
    walk (e.window, inGap, { card.getRight() + 10.0f, inGap.y }, [] {});
    CHECK (harness::settleUntil ([&] { return ! knob.isTooltipShown(); }));
}

namespace
{
// The knobs the way to the Readout is checked on: one with a label, a Small one without, a Small one with its Readout
// above it, and one with a ring, whose hit area reaches the ring's outer edge.
enum class Kind
{
    labelled,
    small,
    readoutAbove,
    ringed
};

const char* describe (Kind kind)
{
    switch (kind)
    {
        case Kind::labelled: return "a knob with a label";
        case Kind::small: return "a Small knob without one";
        case Kind::readoutAbove: return "a Small knob with its Readout above it";
        case Kind::ringed: return "a knob with a ring";
    }
    return "";
}

staple::Knob& knobOf (Hosted& e, Kind kind)
{
    switch (kind)
    {
        case Kind::labelled: return e.labelFrequency();
        case Kind::small: return e.small;
        case Kind::readoutAbove: return e.atFoot;
        case Kind::ringed: return e.ringedGain();
    }
    return e.small;
}

// Points on knob's hit area, on its half away from its Readout, off the face's centre with the Readout below (y flips
// when it's above): round the face near its edge, from level with the centre over the top; round the ring's lane where
// it has one; and the starts of two moves that lost the Readout (a Small face's top left, the ring's left).
std::vector<juce::Point<float>> startsAwayFromReadout (staple::Knob& knob, Kind kind)
{
    std::vector<juce::Point<float>> starts;
    const float r = knob.getFaceRadius();
    for (const float degrees : { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f })
    {
        const juce::Point<float> direction { std::cos (juce::degreesToRadians (degrees)), -std::sin (juce::degreesToRadians (degrees)) };
        starts.push_back (direction * (0.9f * r));
        if (kind == Kind::ringed)
            starts.push_back (direction * (r + staple::tokens::knob::ringOffset));
    }
    if (kind == Kind::small || kind == Kind::readoutAbove)
        starts.push_back ({ -13.0f, -7.0f });
    if (kind == Kind::ringed)
        starts.push_back ({ -46.0f, 5.0f });
    return starts;
}
} // namespace

TEST_CASE ("A Knob Readout stays on every straight move from its knob's face or ring, on the half away from it, to its corners and centre")
{
    Hosted e;
    const auto kind = GENERATE (Kind::labelled, Kind::small, Kind::readoutAbove, Kind::ringed);
    INFO (describe (kind));
    auto& knob = knobOf (e, kind);
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    const auto centre = e.window.getLocalPoint (&knob, knob.getFaceCentre());
    const auto card = e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat();
    const bool above = card.getCentreY() < centre.y;
    REQUIRE (above == (kind == Kind::readoutAbove));

    // 1 px in from each corner, so JUCE's whole-pixel point stays on the Readout.
    const auto inside = card.reduced (1.0f);
    for (const auto target : { inside.getTopLeft(), inside.getTopRight(), inside.getBottomLeft(), inside.getBottomRight(), card.getCentre() })
    {
        for (auto start : startsAwayFromReadout (knob, kind))
        {
            if (above)
                start.y = -start.y;
            const auto from = centre + start;
            INFO ("From " << start.toString() << " off the face's centre to " << (target - centre).toString());
            e.window.pointerAtWindow (from);
            REQUIRE (Hosted::underPointer() == &knob);
            REQUIRE (knob.isTooltipShown());
            REQUIRE (e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat() == card);
            int lostAt = 0, step = 0;
            walk (e.window, from, target, [&] {
                ++step;
                if (lostAt == 0 && ! knob.isTooltipShown())
                    lostAt = step;
            });
            CHECK (lostAt == 0);
            CHECK (Hosted::underPointer() == &readout);
        }
    }
}

TEST_CASE ("A Knob Readout stays while the pointer rests on the way from its knob's face to a corner of it")
{
    Hosted e;
    const auto kind = GENERATE (Kind::small, Kind::readoutAbove, Kind::ringed);
    INFO (describe (kind));
    auto& knob = knobOf (e, kind);
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    const auto centre = e.window.getLocalPoint (&knob, knob.getFaceCentre());
    const auto card = e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat().reduced (0.5f);
    const bool above = kind == Kind::readoutAbove;
    const auto start = startsAwayFromReadout (knob, kind).back();
    const auto from = centre + juce::Point<float> (start.x, above ? -start.y : start.y);
    const auto corner = above ? card.getBottomLeft() : card.getTopLeft();
    e.window.pointerAtWindow (from);
    REQUIRE (knob.isTooltipShown());

    // Most of the way, then a rest there, off the knob and the Readout, which the watchdog checks.
    const auto resting = from + (corner - from) * 0.8f;
    walk (e.window, from, resting, [] {});
    REQUIRE (Hosted::underPointer() != &knob);
    REQUIRE (Hosted::underPointer() != &readout);
    const harness::VisibilityChanges changes (readout);
    harness::settle (3 * staple::TipWatchdog::periodMs);
    CHECK (changes.hides == 0);
    CHECK (knob.isTooltipShown());
}

namespace
{
// The radius of knob's hit area: its face's, or its ring's outer edge.
float hitRadius (staple::Knob& knob, Kind kind)
{
    return knob.getFaceRadius() + (kind == Kind::ringed ? staple::tokens::knob::ringOffset + staple::tokens::knob::ringLane / 2.0f : 0.0f);
}

// A point just inside knob's hit area, on side (-1 left, 1 right), height px from its centre's level away from its
// Readout.
juce::Point<float> justInside (Hosted& e, staple::Knob& knob, Kind kind, float side, float height)
{
    const auto centre = e.window.getLocalPoint (&knob, knob.getFaceCentre());
    const float r = hitRadius (knob, kind);
    const float awayFromReadout = kind == Kind::readoutAbove ? 1.0f : -1.0f;
    return { centre.x + side * (std::sqrt (r * r - height * height) - 1.0f), centre.y + awayFromReadout * height };
}
} // namespace

// The way to the Readout reaches out beside the hit area only below where the lines from the Readout's far corners
// touch it: above that, and level with the centre where the Readout is narrower than the hit area (a ringed knob's).
TEST_CASE ("A Knob Readout hides at once when the pointer leaves its knob sideways above where the way to it reaches out")
{
    Hosted e;
    const auto kind = GENERATE (Kind::labelled, Kind::small, Kind::readoutAbove, Kind::ringed);
    INFO (describe (kind));
    auto& knob = knobOf (e, kind);
    e.hover (knob);
    const auto side = GENERATE (-1.0f, 1.0f);
    const auto height = GENERATE (0.0f, 0.85f);
    if (height < 0.5f && kind != Kind::ringed)
        return; // the way reaches out level with the centre: the Readout is wider than the face (the case below)
    INFO ((side < 0 ? "Left, " : "Right, ") << height << " of the radius from the centre's level");
    const auto onEdge = justInside (e, knob, kind, side, height * hitRadius (knob, kind));
    e.window.pointerAtWindow (onEdge);
    REQUIRE (Hosted::underPointer() == &knob);
    REQUIRE (knob.isTooltipShown());

    // 2 px off the hit area, sideways.
    e.window.pointerAtWindow (onEdge.translated (side * 3.0f, 0.0f));
    REQUIRE (Hosted::underPointer() != &knob);
    CHECK_FALSE (knob.isTooltipShown());
}

TEST_CASE ("A Knob Readout wider than its knob's face hides on its watchdog's next check when the pointer leaves the face sideways, level with its centre")
{
    Hosted e;
    const auto kind = GENERATE (Kind::labelled, Kind::small, Kind::readoutAbove);
    INFO (describe (kind));
    auto& knob = knobOf (e, kind);
    e.hover (knob);
    const auto side = GENERATE (-1.0f, 1.0f);
    INFO ((side < 0 ? "Left" : "Right"));
    const auto onEdge = justInside (e, knob, kind, side, 0.0f);
    e.window.pointerAtWindow (onEdge);
    REQUIRE (knob.isTooltipShown());

    // Out of the face where the way to the Readout reaches out, then on out of it: the knob hears nothing more.
    e.window.pointerAtWindow (onEdge.translated (side * 3.0f, 0.0f));
    REQUIRE (Hosted::underPointer() != &knob);
    e.window.pointerAtWindow (onEdge.translated (side * 20.0f, 0.0f));
    CHECK (harness::settleUntil ([&] { return ! knob.isTooltipShown(); }));
}

namespace
{
// The shortest distance from p to the segment from a to b.
float distanceToSegment (juce::Point<float> p, juce::Point<float> a, juce::Point<float> b)
{
    const auto ab = b - a;
    const float t = juce::jlimit (0.0f, 1.0f, (p - a).getDotProduct (ab) / ab.getDotProduct (ab));
    return p.getDistanceFrom (a + ab * t);
}
} // namespace

// The ring's Readout (eq1's Dynamic Range one, 136 px) is wider than the knob's own (86 px), and the way to it is built
// from it: the content that showed as the pointer left the ring stays until the pointer is back on the face or ring,
// read again as the knob's value and the ring's move on the way (automation, eq1's Dynamic Range).
TEST_CASE ("A ring's own Knob Readout keeps its content on every straight move from the ring, past the face, to its corners")
{
    Hosted e;
    auto& knob = e.wideRingedGain();
    const auto centre = e.window.getLocalPoint (&knob, knob.getFaceCentre());
    const float r = knob.getFaceRadius();
    e.hover (knob);
    auto& readout = *knob.getKnobTooltip();
    const auto ownCard = e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat();
    REQUIRE (readout.titleText() == "Gain");

    int moves = 0;
    for (const float degrees : { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f })
    {
        const juce::Point<float> direction { std::cos (juce::degreesToRadians (degrees)), -std::sin (juce::degreesToRadians (degrees)) };
        const auto from = centre + direction * (r + staple::tokens::knob::ringOffset);
        e.window.pointerAtWindow (from);
        REQUIRE (Hosted::underPointer() == &knob);
        REQUIRE (knob.isTooltipShown());
        REQUIRE (readout.titleText() == "Band 4 Dynamic Range");
        const auto card = e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat();
        REQUIRE (card.getWidth() > ownCard.getWidth());

        const auto inside = card.reduced (1.0f);
        for (const auto target : { inside.getTopLeft(), inside.getTopRight(), inside.getBottomLeft(), inside.getBottomRight() })
        {
            // Past the face, not over it: back on the face, the Readout is the knob's own again.
            if (distanceToSegment (centre, from, target) <= r + 2.0f)
                continue;
            ++moves;
            INFO ("From " << (from - centre).toString() << " off the face's centre to " << (target - centre).toString());
            e.window.pointerAtWindow (from);
            REQUIRE (readout.titleText() == "Band 4 Dynamic Range");
            const auto gainBefore = knob.getValue();
            int lostAt = 0, changedAt = 0, step = 0;
            walk (e.window, from, target, [&] {
                ++step;
                if (step == 4)
                    knob.setValue (gainBefore + 1.0);
                if (step == 6)
                {
                    e.wideLane.value = "+7.0 dB";
                    knob.ringChanged();
                }
                if (lostAt == 0 && ! knob.isTooltipShown())
                    lostAt = step;
                if (changedAt == 0 && e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat() != card)
                    changedAt = step;
            });
            REQUIRE (step > 6);
            CHECK (lostAt == 0);
            CHECK (changedAt == 0);
            CHECK (Hosted::underPointer() == &readout);
            CHECK (readout.titleText() == "Band 4 Dynamic Range");
            CHECK (readout.valueText() == "+7.0 dB");
            knob.setValue (gainBefore);
            e.wideLane.value = "+6.0 dB";
        }
    }
    REQUIRE (moves >= 4);

    // Back on the face: the knob's own.
    e.window.pointerAtWindow (centre);
    CHECK (readout.titleText() == "Gain");
    CHECK (e.window.getLocalArea (&readout, readout.getLocalBounds()).toFloat() == ownCard);
}

// Closely packed knobs, as on eq1's Band panel: the way to one knob's Readout crosses its neighbour's face.
TEST_CASE ("One Knob Readout shows at a time: a neighbour's hides it on the way to it, unless it is being typed into")
{
    Hosted e;
    auto& a = e.small;
    // Its face's centre 30 px right of a's and 18 px down: on the way to a's Readout, beside the Readout's top.
    staple::Knob b { staple::tokens::knob::small, "Neighbour" };
    a.getParentComponent()->addAndMakeVisible (b);
    b.setBounds (a.getBounds().translated (30, 18));
    REQUIRE (b.getParentComponent()->getLocalBounds().contains (b.getBounds()));

    const bool typing = GENERATE (false, true);
    INFO ((typing ? "a's Readout is being typed into" : "a's Readout is not being typed into"));
    e.hover (a);
    auto& readoutA = *a.getKnobTooltip();
    if (typing)
    {
        readoutA.startEditing();
        REQUIRE (readoutA.isEditing());
    }

    const auto onB = e.window.getLocalPoint (&b, b.getFaceCentre());
    int both = 0, step = 0;
    walk (e.window, e.window.getLocalPoint (&a, a.getFaceCentre()), onB, [&] {
        ++step;
        if (! typing && both == 0 && a.isTooltipShown() && b.isTooltipShown())
            both = step;
    });
    REQUIRE (Hosted::underPointer() == &b);
    CHECK (b.isTooltipShown());
    CHECK (both == 0);
    CHECK (a.isTooltipShown() == typing);

    // Through the watchdogs' checks.
    harness::settle (3 * staple::TipWatchdog::periodMs);
    CHECK (b.isTooltipShown());
    CHECK (a.isTooltipShown() == typing);
    if (typing)
        CHECK (readoutA.isEditing());
}
