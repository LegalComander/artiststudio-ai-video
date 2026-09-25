#include "StemEngine.h"

namespace
{
juce::String quote (const juce::String& text)
{
    return "\"" + text.replace ("\"", "\\\"") + "\"";
}

bool commandWorks (const juce::String& command)
{
    juce::ChildProcess process;
    if (! process.start (command))
        return false;

    process.waitForProcessToFinish (5000);
    return process.getExitCode() == 0;
}
}

StemEngine::StemEngine()
    : juce::Thread ("ArtistStudioStemEngine")
{
}

StemEngine::~StemEngine()
{
    cancel();
}

juce::String StemEngine::detectDemucsLauncher()
{
   #if JUCE_WINDOWS
    const juce::StringArray candidates { "py -3", "python", "python3" };
   #else
    const juce::StringArray candidates { "python3", "python" };
   #endif

    for (const auto& candidate : candidates)
        if (commandWorks (candidate + " -m demucs --help"))
            return candidate;

    return {};
}

void StemEngine::startSeparation (juce::File sourceFile, juce::File outputRoot, bool maximumQuality, Completion completion)
{
    if (isThreadRunning())
        return;

    source = std::move (sourceFile);
    output = std::move (outputRoot);
    maxQuality = maximumQuality;
    onComplete = std::move (completion);
    launcher = detectDemucsLauncher();
    startThread();
}

void StemEngine::cancel()
{
    signalThreadShouldExit();
    stopThread (3000);
}

void StemEngine::run()
{
    StemSeparationResult result;

    if (! source.existsAsFile())
    {
        result.error = "Source audio file is missing.";
    }
    else if (launcher.isEmpty())
    {
        result.error = "Demucs was not found. Install Python 3 and run: pip install -U demucs";
    }
    else
    {
        output.createDirectory();
        const auto model = maxQuality ? "htdemucs_ft" : "htdemucs";
        const auto command = launcher
                           + " -m demucs -n " + model
                           + " --out " + quote (output.getFullPathName())
                           + " " + quote (source.getFullPathName());

        juce::ChildProcess process;
        if (! process.start (command))
        {
            result.error = "Could not start the local Demucs process.";
        }
        else
        {
            juce::String log;
            while (process.isRunning() && ! threadShouldExit())
            {
                log << process.readAllProcessOutput();
                wait (150);
            }

            if (threadShouldExit())
            {
                process.kill();
                result.error = "Stem separation cancelled.";
            }
            else
            {
                process.waitForProcessToFinish (-1);
                log << process.readAllProcessOutput();
                result.log = log;

                if (process.getExitCode() != 0)
                {
                    result.error = "Demucs failed.\n\n" + log.substring (juce::jmax (0, log.length() - 2200));
                }
                else
                {
                    const auto songFolder = output.getChildFile (model).getChildFile (source.getFileNameWithoutExtension());
                    result.outputDirectory = songFolder;
                    result.vocals = songFolder.getChildFile ("vocals.wav");
                    result.drums = songFolder.getChildFile ("drums.wav");
                    result.bass = songFolder.getChildFile ("bass.wav");
                    result.other = songFolder.getChildFile ("other.wav");
                    result.ok = result.vocals.existsAsFile()
                             && result.drums.existsAsFile()
                             && result.bass.existsAsFile()
                             && result.other.existsAsFile();

                    if (! result.ok)
                        result.error = "Demucs finished, but one or more expected WAV stems were not found.";
                }
            }
        }
    }

    auto completion = onComplete;
    juce::MessageManager::callAsync ([completion = std::move (completion), result = std::move (result)] () mutable
    {
        if (completion)
            completion (std::move (result));
    });
}
