#pragma once

// Copied from eq1's plugin/EditorTick.h (eq1 at dbb1127), in namespace plugkit::ui; listed in eq1#231.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <atomic>
#include <optional>

namespace plugkit::ui
{

// The editor's one tick, which PluginEditor owns. Every component that follows the processor (the
// meters, the Analyzer, what a parameter or a restored session shows) reads it on this tick, as a
// Client, rather than on a timer of its own, and repaints only what changed. It runs at activeHz
// while anything animates or meters (a Client says it is active, or a Tween is easing) and at idleHz
// otherwise. A parameter change, from an edit, Host Automation or a restored session, wakes it to
// activeHz: on the message thread at once, the timer restarted there and then. From another thread
// (the audio thread's automation) the change only sets a lock-free flag, with no message, lock or
// allocation; the next tick, within one idle interval, clears it, shows the change and comes again at
// activeHz. The Tweens (staple/controls/Tween.h) keep their own frames: they belong to the Staple
// kit, which has no editor, and run only while easing.
class EditorTick final : private juce::Timer, private juce::AudioProcessorListener
{
public:
    static constexpr int activeHz = 60, idleHz = 10;

    class Client;
    // What owns a tick: a Client joins the nearest Host at or above its component.
    struct Host
    {
        virtual ~Host() = default;
        virtual EditorTick& editorTick() = 0;
    };

    explicit EditorTick (juce::AudioProcessor& processor);
    ~EditorTick() override;

    // The rate the next tick comes at: activeHz or idleHz.
    int rateHz() const { return rate; }
    // How many ticks there have been.
    int count() const { return ticks; }
    // A parameter changed: the next tick comes at activeHz (above). Message thread.
    void wake();
    // Puts client on this tick: what a Host does for itself; a Client under a Host joins by itself.
    void add (Client& client);
    // For tests only: every tick hands its Clients exactly seconds, however much real time passed, so a
    // tick-driven value (a meter's fall, a fade) checked against the audio played doesn't depend on the
    // machine's speed. std::nullopt (the default, and what shipping code leaves it at) hands over the
    // real time since the last tick. The editor harness sets it (OpenEditor::fixTickSeconds).
    void fixTickSeconds (std::optional<double> seconds);

private:
    void timerCallback() override;
    void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override;
    void audioProcessorChanged (juce::AudioProcessor*, const juce::AudioProcessorListener::ChangeDetails&) override {}
    void runAt (int hz);

    juce::AudioProcessor& processor;
    juce::ListenerList<Client> clients;
    int rate = 0, ticks = 0;
    bool woken = false;    // a parameter changed since the last tick
    std::atomic<bool> changedOffThread { false }; // what another thread's change sets; the tick clears it
    static_assert (std::atomic<bool>::is_always_lock_free);
    double lastTickMs = 0.0; // 0 before the first
    std::optional<double> fixedSeconds; // what every tick hands over instead of real time, set by a test
};

// What the tick drives: a component that reads the processor each tick and repaints only on a
// change. It joins the Host nearest at or above its component whenever the component's place in the
// hierarchy changes, and leaves when the component goes.
class EditorTick::Client
{
public:
    explicit Client (juce::Component& component);
    virtual ~Client();

    // Each tick, with the seconds since the one before: 1/60 s on the first, and at most 0.25 s after
    // a stall, so a meter's fall doesn't jump. In a test that fixes the tick's duration
    // (EditorTick::fixTickSeconds), that duration instead.
    virtual void tick (double seconds) = 0;
    // True while it animates or meters, which keeps the tick at activeHz.
    virtual bool isActive() const { return false; }

protected:
    // Asks for the next tick at once (EditorTick::wake): what a pointer event that starts an animation
    // or a wait does, so the first frame isn't up to an idle interval late.
    void wake();

private:
    friend class EditorTick;
    // Joins the Host the component is under now, if any.
    void follow();
    void join (EditorTick* tick);
    // Watches the component's place in the hierarchy, apart from any listener the component is itself.
    struct Watch final : juce::ComponentListener
    {
        explicit Watch (Client& c) : client (c) {}
        void componentParentHierarchyChanged (juce::Component&) override { client.follow(); }
        Client& client;
    };

    juce::Component& component;
    Watch watch { *this };
    EditorTick* source = nullptr; // the tick it is on, while under a Host
};

} // namespace plugkit::ui
