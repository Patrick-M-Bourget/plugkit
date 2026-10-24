#pragma once

// Copied from eq1's plugin/staple/Tokens.h (eq1 at dbb1127), without eq1's own tokens: the Band palette, the window's
// size and its display's, Band panel's and Detection Range bar's layout (the window's padding and gap, the header's,
// footer's and meter rail's sizes and the menus' stay), and the dynamics, handle, dynamicRangeHandle, ghost and
// hoverCard namespaces, which eq1 keeps beside its own components; listed in eq1#231. Of the header namespace, the kit
// has the two tokens plugkit-ui's Preset and A/B widgets read. Of the curve namespace, it has the main curve's stroke
// (eq1's sum) and globalBypassScale, which comp1 shares, kept in motion beside globalBypassFadeMs; eq1's Band curves'
// strokes and alphas stay eq1's. The toggle namespace names the box staple::LookAndFeel::drawToggleButton draws.

// Staple's design tokens: the values in docs/staple-handoff/tokens.json and HANDOFF.md's visual spec,
// kept by hand. The only place in plugkit-ui a colour is written as a number. design-system/ beside this file
// documents the system for eq1 and comp1: its README, tokens.json and the component guidelines.
#include <juce_graphics/juce_graphics.h>

namespace staple::tokens
{
namespace colour
{
    inline const juce::Colour bg0 { 0xFF0B0C0F };
    inline const juce::Colour surface1 { 0x09FFFFFF };
    inline const juce::Colour fill1 { 0x0FFFFFFF };
    inline const juce::Colour fill2 { 0x1AFFFFFF };
    inline const juce::Colour fill3 { 0x26FFFFFF };
    inline const juce::Colour raised { 0xD114161A };
    inline const juce::Colour menu { 0xFF16181C };
    // Contrast on bg0: 16.4, 9.1, 5.3 and 2.6 : 1. text4 is below readable contrast: disabled states
    // and decoration only, never text a user must read.
    inline const juce::Colour text1 { 0xFFEEEBE5 };
    inline const juce::Colour text2 { 0xBDEEEBE5 };
    inline const juce::Colour text3 { 0x8AEEEBE5 };
    inline const juce::Colour text4 { 0x52EEEBE5 };
    inline const juce::Colour onLight { 0xFF121316 };
    inline const juce::Colour line1 { 0x0DFFFFFF };
    inline const juce::Colour line2 { 0x1AFFFFFF };
    inline const juce::Colour line3 { 0x2EFFFFFF };
    inline const juce::Colour hoverCardLine { 0x17FFFFFF }; // white at 9 %
    inline const juce::Colour focus { 0xBFFFFFFF };
    inline const juce::Colour curveMain { 0xFFF5B930 };
    inline const juce::Colour curveMainHalo { 0x1AF5B930 };
    inline const juce::Colour anSc { 0xFF7FCFC4 };
    inline const juce::Colour dynRange { 0xFFD6455A };
    inline const juce::Colour dynLive { 0xFFF5C451 };
    inline const juce::Colour stateOff { 0xFFE5506A };
    inline const juce::Colour stateOffBg { 0x29E5506A };
    inline const juce::Colour placeLeft { 0xFFEEEBE5 };
    inline const juce::Colour placeRight { 0xFFE5604F };
    inline const juce::Colour placeStereo { 0xFFE9B44C };
    inline const juce::Colour placeMid { 0xFF5FCB76 };
    inline const juce::Colour placeSide { 0xFF4FA9E8 };
    inline const juce::Colour meter1 { 0xFF3FC79A };
    inline const juce::Colour meter2 { 0xFFA9D66A };
    inline const juce::Colour meter3 { 0xFFE9B44C };
    inline const juce::Colour meterClip { 0xFFE5604F };
    inline const juce::Colour meterClipOff { 0x14FFFFFF }; // white 8 %, an unlit Clip Light
    inline const juce::Colour meterTrack { 0x59000000 };   // black 35 %, under the Output Meter's bars
    inline const juce::Colour edgeSelectorBase { 0xFF141519 };
    inline const juce::Colour knobFaceTop { 0xFF35373D }, knobFaceMid { 0xFF27282D }, knobFaceEdge { 0xFF1C1D21 };
    inline const juce::Colour knobRim { 0x38FFFFFF };      // white 22 %
    inline const juce::Colour knobRimSmall { 0x33FFFFFF }; // white 20 %, on knobs of 30 px and below
    inline const juce::Colour knobRingLane { 0x0FFFFFFF }; // white 6 %
    inline const juce::Colour dynRangeInner { 0xFF9C3344 }, dynLiveInner { 0xFFC49A34 };
    inline const juce::Colour ringHint { 0xCCFFFFFF };     // the ring's ▲▼ grab hint, white 80 %
    // The Threshold fader: its track, its thumb (a grey gradient with a 1 px top highlight) and the
    // thumb's line, and the Detection Range bar's pill handles.
    inline const juce::Colour faderTrack { 0x59000000 };   // black 35 %
    inline const juce::Colour thumbTop { 0xFF4A4D55 }, thumbBottom { 0xFF34373E };
    inline const juce::Colour thumbHighlight { 0x2EFFFFFF }; // white 18 %
    inline const juce::Colour thumbLine { 0xD9F1EEE8 };      // rgba (241, 238, 232, 0.85)
    inline const juce::Colour pillTop { 0xFF5A5D64 }, pillBottom { 0xFF3A3D44 };
    inline const juce::Colour shadow { 0xFF000000 };
    // The Preset browser's modal: the scrim over the editor, and the dialog's fill and edge.
    inline const juce::Colour scrim { 0x59000000 };      // black 35 %
    inline const juce::Colour dialog { 0xF0131519 };     // rgba (19, 21, 25, 0.94)
    inline const juce::Colour dialogEdge { 0x14FFFFFF }; // white 8 %
    inline const juce::Colour none { 0x00000000 };       // nothing drawn: a widget's own fill or edge turned off
    // The window's three soft neutral highlights over bg0 (HANDOFF.md §4, "Window").
    inline const juce::Colour windowHighlight1 { 0x14E2E6EE }; // rgba (226, 230, 238, 0.08)
    inline const juce::Colour windowHighlight2 { 0x0DAAB2C0 }; // rgba (170, 178, 192, 0.05)
    inline const juce::Colour windowHighlight3 { 0x0F787E8A }; // rgba (120, 126, 138, 0.06)

    // The display: grid lines, and the Analyzer's spectra.
    inline const juce::Colour gridMinor { 0x06FFFFFF }; // white 2.5 %
    inline const juce::Colour gridMajor { 0x0FFFFFFF }; // white 6 %
    inline const juce::Colour gridZero { 0x38FFFFFF };  // white 22 %, the 0 dB line
    inline const juce::Colour anFillTop { 0x33C3CCE6 }, anFillMid { 0x12C3CCE6 }; // 20 % at the top, 7 % at 60 % down, then 0
    inline const juce::Colour anLine { 0x6BCBD3EA };    // the post spectrum, 42 %
    inline const juce::Colour anPeak { 0x29DDE3F5 };    // Peak Hold, 16 %
    inline const juce::Colour anPre { 0x33DDE3F5 };     // the pre spectrum beside the post one, 20 %
    inline const juce::Colour anScLine { 0x8C7FCFC4 };  // the Sidechain spectrum, anSc at 55 %

    // Band handles and the ghost Bell.
    inline const juce::Colour handleRing { 0x8C0A0B0E }; // rgba (10, 11, 14, 0.55), around an unselected handle
    inline const juce::Colour handleSelectedRing { 0xFFFFFFFF }; // and a selected one's, white
    inline const juce::Colour sheen { 0xFFFFFFFF };      // white light: a handle's sheen, the ghost Bell's line
    inline const juce::Colour ghostGlow { 0xFFFFC482 };  // rgb (255, 196, 130), at the ghost Bell's peak
} // namespace colour

namespace size
{
    constexpr float fs1 = 10.0f;
    constexpr float fs2 = 11.0f;
    constexpr float fs3 = 12.0f;
    constexpr float fs4 = 13.0f;
    constexpr float fs5 = 16.0f;
    constexpr float r1 = 4.0f;
    constexpr float r2 = 6.0f;
    constexpr float r3 = 10.0f;
    constexpr float iconStroke = 1.5f;
    constexpr float iconGrid = 16.0f;
    constexpr float focusWidth = 2.0f, focusOffset = 2.0f;
} // namespace size

// The window's areas and menus, at 100 % UI scale (HANDOFF.md §2, §4, §9; §4 "Menus / popovers / cards"): the
// window's padding and the gap between its rows, the header's and footer's height and the Output Meter rail's width.
namespace layout
{
    constexpr int outerPadding = 14, gap = 12;
    constexpr int headerHeight = 52, footerHeight = 44, meterWidth = 40;
    constexpr int menuItemHeight = 30, menuPadding = 6, menuSeparatorMargin = 5;
} // namespace layout

// Knobs and the Dynamic Range ring around the Gain knob.
namespace knob
{
    constexpr float frequency = 50.0f, gain = 66.0f, q = 50.0f, small = 30.0f;
    // The family's one-knob size: comp1's Compact View Comp, its only knob. Beyond Staple's Frequency, Gain, Q and
    // small sizes, approved for that view alone.
    constexpr float hero = 120.0f;
    constexpr float rim = 2.0f, rimSmall = 1.5f;           // rimSmall at 30 px and below
    constexpr float arc = 3.0f, arcSmall = 2.0f;           // arcSmall below 40 px
    constexpr float arcInset = 4.5f, arcInsetProportion = 0.1f; // the arc's radius is r - max (arcInset, d x arcInsetProportion)
    constexpr float sweepDegrees = 270.0f;
    constexpr float arcGlow = 1.5f, arcGlowAlpha = 0.3f;   // a wider translucent stroke under the arc
    constexpr float originStubDegrees = 0.5f;              // what the arc shows at its origin
    constexpr float dragPixels = 200.0f, fineDragPixels = 800.0f; // the full range, and with Shift
    constexpr float ringLane = 12.0f, ringOffset = 10.0f;
    constexpr float ringRangeAlpha = 0.85f, ringRangeBypassedAlpha = 0.3f; // the range arc, and under Dynamics Bypass
    constexpr float liveGainMinimum = 0.05f; // dB: the Live Gain arc hides below this movement
    constexpr float ringDbPerDrag = 60.0f;   // dB per dragPixels (fineDragPixels with Shift)
} // namespace knob

// Popovers (HANDOFF.md §4 "Menus / popovers / cards"), in px: the gap to their opener, by the side they
// open on (the footer's popovers end 38 px above the bottom of a 28 px button), and their edge, inside
// the card as a border-box border is.
namespace popover
{
    constexpr int gapAbove = 10, gapBelow = 6;
    constexpr int edge = 1;
} // namespace popover

// The main curve on a display (HANDOFF.md §4 "Curves"): a line in curveMain over its halo in curveMainHalo, and a wider,
// fainter halo in place of a blur; widths in px, the alpha 0 to 1. Copied from eq1's plugin/staple/Tokens.h, curve::sum,
// sumHalo, sumOuterHalo and sumOuterHaloAlpha (eq1 at 33fee16).
namespace curve
{
    constexpr float mainLine = 2.0f, mainHalo = 4.0f, mainOuterHalo = 8.0f, mainOuterHaloAlpha = 0.5f;
} // namespace curve

// A stock toggle (staple::LookAndFeel::drawToggleButton), in px: its box, inset from the left and centred down it, and
// the gap between the box and the label. staple::LookAndFeel::idealToggleWidth adds the label's width.
namespace toggle
{
    constexpr float inset = 2.0f, box = 14.0f, labelGap = 6.0f;
} // namespace toggle

// Shadows: CSS blur radius and offset, as juce::DropShadow takes them.
namespace shadow
{
    inline const juce::DropShadow shadow1 { colour::shadow.withAlpha (0.45f), 24, { 0, 8 } };  // menus, popovers, tooltips
    inline const juce::DropShadow shadow2 { colour::shadow.withAlpha (0.55f), 48, { 0, 18 } }; // the context menu, dialogs
    inline const juce::DropShadow knob { colour::shadow.withAlpha (0.4f), 14, { 0, 6 } };
    inline const juce::DropShadow knobSmall { colour::shadow.withAlpha (0.35f), 8, { 0, 3 } }; // 30 px and below
    inline const juce::DropShadow handle { colour::shadow.withAlpha (0.45f), 5, { 0, 0 } };
    inline const juce::DropShadow thumb { colour::shadow.withAlpha (0.5f), 5, { 0, 2 } }; // the fader's thumb, the bar's pills
    inline const juce::DropShadow panThumb { colour::shadow.withAlpha (0.5f), 3, { 0, 1 } }; // a PopoverSlider's small thumb (eq1's Output Pan's)
    inline const juce::DropShadow selectedHandle { colour::shadow.withAlpha (0.45f), 8, { 0, 0 } };
} // namespace shadow

namespace motion
{
    constexpr int dur1Ms = 120, dur2Ms = 180, dur3Ms = 240; // hover/press, menus/cards, panels
    constexpr int hoverFadeMs = 220;                        // a Band curve's hover fade, and the Hover Card's hide delay
    constexpr int globalBypassFadeMs = 150;                 // the display's curves fading into and out of Global Bypass
    constexpr float globalBypassScale = 0.45f;              // what shows the plugin's work, of its usual alphas, under Global Bypass
    // The one easing curve: cubic-bezier (0.2, 0.7, 0.2, 1).
    constexpr float easeX1 = 0.2f, easeY1 = 0.7f, easeX2 = 0.2f, easeY2 = 1.0f;
    // Menus, popovers and cards pop in from 3 px lower at 98.5 % scale.
    constexpr float popInOffset = 3.0f, popInScale = 0.985f;
    constexpr float hoverBrightness = 1.18f, pressedBrightness = 1.3f, disabledAlpha = 0.35f;
    constexpr float bypassedAlpha = 0.38f; // a Bypassed Band's Band panel and Hover Card values
} // namespace motion

// The header (HANDOFF.md §5.6), in px: the padding either side of its text buttons' text (A/B Compare,
// Copy), and the gap between the Preset name and its Modified dot (the button's 8 px gap and the dot's
// 6 px margin).
namespace header
{
    constexpr int chipPadding = 10;
    constexpr float modifiedDotGap = 14.0f;
} // namespace header
} // namespace staple::tokens
