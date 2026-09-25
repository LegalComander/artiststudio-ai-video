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

struct EngineSetupResult
{
    bool ok = false;
    juce::String launcher;
    juce::String log;
    juce::String error;
};

class StemEngine : private juce::Thread
{
public:
    using Completion = std::function<void (StemSeparationResult)>;
    using SetupCompletion = std::function<void (EngineSetupResult)>;

    StemEngine();
    ~StemEngine() override;

    bool isBusy() const noexcept { return isThreadRunning(); }
    bool isEngineReady();
    juce::String detectDemucsLauncher();
    juce::File getManagedEngineFolder() const;

    void startEngineSetup (SetupCompletion completion);
    void startSeparation (juce::File sourceFile,
                          juce::File outputRoot,
                          bool maximumQuality,
                          Completion completion);
    void cancel();

private:
    enum class Job
    {
        none,
        setup,
        separation
    };

    void run() override;
    void runSetup();
    void runSeparation();

    juce::String detectSystemPython();
    juce::String managedPythonLauncher() const;
    bool runCommand (const juce::String& command,
                     juce::String& log,
                     int timeoutMs = -1);

    Job job = Job::none;
    juce::File source;
    juce::File output;
    bool maxQuality = false;
    Completion onComplete;
    SetupCompletion onSetupComplete;
    juce::String launcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StemEngine)
};
