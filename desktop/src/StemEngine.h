#pragma once

#include <JuceHeader.h>
#include <functional>

struct StemSeparationResult
{
    bool ok = false;
    juce::File outputDirectory;
    juce::File vocals;
    juce::File drums;
    juce::File bass;
    juce::File other;
    juce::String log;
    juce::String error;
};

class StemEngine : private juce::Thread
{
public:
    using Completion = std::function<void (StemSeparationResult)>;

    StemEngine();
    ~StemEngine() override;

    bool isBusy() const noexcept { return isThreadRunning(); }
    juce::String detectDemucsLauncher();
    void startSeparation (juce::File sourceFile, juce::File outputRoot, bool maximumQuality, Completion completion);
    void cancel();

private:
    void run() override;

    juce::File source;
    juce::File output;
    bool maxQuality = false;
    Completion onComplete;
    juce::String launcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StemEngine)
};
