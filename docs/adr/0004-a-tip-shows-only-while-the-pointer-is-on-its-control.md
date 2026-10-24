# A tip shows only while the pointer is really on its control

A tip (the editor's Tooltip window, `staple::TooltipWindow`, or a Knob Readout, `staple::KnobTooltip`) shows only while the pointer is on the control it is for, as the OS has the pointer, not as the last mouse event left it. A tip left up after the pointer has gone reads as a stuck control, and a plugin's window misses pointer events often enough (another window over it, the host hiding it) that the events alone can't decide it. The rules:

- **The hit area, not the bounds.** A control is under the pointer where its hit area is: `TipWatchdog::isPointerOver` asks `Component::contains`, which tests the hit area of the control and each parent and then asks the OS whether the control's window is the one under the point. A Knob's hit area is its face's circle, or its ring's outer edge (`Knob::hitTest`), not the shadow margin round it. The Tooltip window asks the same before showing a tip (`TooltipWindow::getTipFor`).
- **The way to the Readout.** Off the face, a Knob Readout stays while the pointer crosses to it: the way is the convex hull of the hit area's circle and the Readout's card (`wayToReadout` in `Knob.cpp`, `Knob::isOnWayToReadout`), so every straight move from the knob onto the card stays on it. The way keeps the Readout only while the pointer crosses it: once the watchdog finds the pointer at one whole pixel there for `Knob::restOnWayChecks` (6) checks after the first, 480 ms, it hides the Readout (`Knob::watchTooltip`).
- **The watchdog.** While a tip shows, its `TipWatchdog` reads the OS's pointer every `TipWatchdog::periodMs` (80 ms) and hides the tip unless the pointer is still on its control (and, for a Readout, on the Readout or the way to it). A Tooltip fades out over `TooltipWindow::fadeOutMs`; a Readout being dragged, or typed into, stays.
- **No reshow after a click or wheel turn.** On the Tooltip window, a click or a wheel turn hides the tip at once, and the control it happened on has no tip until the pointer moves more than `TooltipWindow::dismissalMovePx` or onto another component, and JUCE's 500 ms of showing a tip again at once have passed: the tip comes back only after a fresh 700 ms rest (`TooltipWindow::dismiss`, `isDismissed`). A press on a Knob shows its Readout, which follows the drag.
- **One Readout at a time.** A knob showing its Readout hides any other knob's on the same overlay layer, unless a value is being typed into that one (`Knob::showTooltip`).
- **Typing goes to what the Readout showed.** A double-click on a Readout opens a field that types into what the Readout showed as it opened, the ring's content or the knob's, and the Readout keeps that content until the field closes, wherever the pointer goes (`KnobTooltip::startEditing`, `KnobTooltip::refresh`).

The code is in `ui/plugkit/ui/staple/` (`TooltipWindow`, `TipWatchdog`, `controls/Knob`, `controls/KnobTooltip`), and `tests/ui/TooltipHostedTest.cpp` and `tests/ui/KnobReadoutTest.cpp` pin each rule.

## Consequences

- **A late hide of up to 80 ms beside a face narrower than its card.** Where the card is wider than the face, the way reaches out beside the face level with its centre. A pointer leaving the face sideways there is still on the way, so the Readout hides on the watchdog's next check, up to `TipWatchdog::periodMs` later, rather than at once. Accepted: cutting the way back there would hide the Readout on some straight moves from the face to the card's corners.
- **A Readout behind a neighbour is unreachable.** On closely packed knobs, the way to one knob's Readout can cross a neighbour's face. The pointer reaching that face shows the neighbour's Readout, which hides the first, so the first can't be reached that way. Accepted: the alternative is two Readouts at once, the neighbour's over the first. A Readout already being typed into stays.
- **Each tip needs a pointer to test against.** A test moves the OS's pointer with `TipWatchdog::ScopedPointerSource`, since it can't move the real mouse, and a check on the pointer goes through the editor harness (`CODING_STANDARDS.md`, "Editor behaviour").

## Considered Options

- **Trusting the pointer's events.** Rejected: the OS sends a plugin's window none when another window comes over it under the pointer, or when the host hides the window, so a tip stays up with JUCE still having the pointer on its control.
- **A way to the Readout that holds it while the pointer rests there.** Rejected: a pointer resting beside a small face, off the face and the card, kept the Readout until it next moved.
