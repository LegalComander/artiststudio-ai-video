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

    process.waitForProcessToFinish (10000);
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

juce::File StemEngine::getManagedEngineFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("ArtistStudio")
        .getChildFile ("StemLab")
        .getChildFile ("engine");
}

juce::String StemEngine::managedPythonLauncher() const
{
   #if JUCE_WINDOWS
    const auto python = getManagedEngineFolder().getChildFile ("venv").getChildFile ("Scripts").getChildFile ("python.exe");
   #else
    const auto python = getManagedEngineFolder().getChildFile ("venv").getChildFile ("bin").getChildFile ("python3");
   #endif

    return python.existsAsFile() ? quote (python.getFullPathName()) : juce::String();
}

juce::String StemEngine::detectSystemPython()
{
   #if JUCE_WINDOWS
    const auto localAppData = juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", {});
    juce::StringArray candidates;

    if (localAppData.isNotEmpty())
    {
        for (const auto& folder : { "Python313", "Python312", "Python311", "Python310" })
        {
            const auto python = juce::File (localAppData)
                                    .getChildFile ("Programs")
                                    .getChildFile ("Python")
                                    .getChildFile (folder)
                                    .getChildFile ("python.exe");
            if (python.existsAsFile())
                candidates.add (quote (python.getFullPathName()));
        }
    }

    candidates.addArray ({ "py -3.13", "py -3.12", "py -3.11", "py -3.10", "py -3", "python" });
   #else
    juce::StringArray candidates { "python3", "python" };
   #endif

    for (const auto& candidate : candidates)
        if (commandWorks (candidate + " --version"))
            return candidate;

    return {};
}

juce::String StemEngine::detectDemucsLauncher()
{
    const auto managed = managedPythonLauncher();
    if (managed.isNotEmpty() && commandWorks (managed + " -m demucs --help"))
        return managed;

   #if JUCE_WINDOWS
    const juce::StringArray candidates { "py -3.13", "py -3.12", "py -3.11", "py -3.10", "py -3", "python" };
   #else
    const juce::StringArray candidates { "python3", "python" };
   #endif

    for (const auto& candidate : candidates)
        if (commandWorks (candidate + " -m demucs --help"))
            return candidate;

    return {};
}

bool StemEngine::isEngineReady()
{
    return detectDemucsLauncher().isNotEmpty();
}

void StemEngine::startEngineSetup (SetupCompletion completion)
{
    if (isThreadRunning())
        return;

    job = Job::setup;
    onSetupComplete = std::move (completion);
    startThread();
}

void StemEngine::startSeparation (juce::File sourceFile,
                                  juce::File outputRoot,
                                  bool maximumQuality,
                                  Completion completion)
{
    if (isThreadRunning())
        return;

    source = std::move (sourceFile);
    output = std::move (outputRoot);
    maxQuality = maximumQuality;
    onComplete = std::move (completion);
    launcher = detectDemucsLauncher();
    job = Job::separation;
    startThread();
}

void StemEngine::cancel()
{
    signalThreadShouldExit();
    stopThread (3000);
}

bool StemEngine::runCommand (const juce::String& command, juce::String& log, int timeoutMs)
{
    juce::ChildProcess process;
    if (! process.start (command))
    {
        log << "\nCould not start: " << command << "\n";
        return false;
    }

    const auto started = juce::Time::getMillisecondCounterHiRes();
    while (process.isRunning() && ! threadShouldExit())
    {
        log << process.readAllProcessOutput();
        wait (120);

        if (timeoutMs > 0
            && juce::Time::getMillisecondCounterHiRes() - started > static_cast<double> (timeoutMs))
        {
            process.kill();
            log << "\nCommand timed out.\n";
            return false;
        }
    }

    if (threadShouldExit())
    {
        process.kill();
        log << "\nCommand cancelled.\n";
        return false;
    }

    process.waitForProcessToFinish (-1);
    log << process.readAllProcessOutput();
    return process.getExitCode() == 0;
}

void StemEngine::run()
{
    if (job == Job::setup)
        runSetup();
    else if (job == Job::separation)
        runSeparation();

    job = Job::none;
}

void StemEngine::runSetup()
{
    EngineSetupResult result;
    juce::String log;

    auto existing = detectDemucsLauncher();
    if (existing.isNotEmpty())
    {
        result.ok = true;
        result.launcher = existing;
        result.log = "Demucs is already available.";
    }
    else
    {
        auto root = getManagedEngineFolder();
        root.createDirectory();

        auto python = detectSystemPython();

       #if JUCE_WINDOWS
        if (python.isEmpty() && commandWorks ("winget --version"))
        {
            log << "Installing Python 3.12 with Windows Package Manager...\n";
            runCommand ("winget install --id Python.Python.3.12 -e --silent --accept-package-agreements --accept-source-agreements", log, 600000);
            python = detectSystemPython();
        }
       #endif

        if (python.isEmpty())
        {
            result.error = "Python 3.10+ was not found and could not be installed automatically. Install Python once, then press Install Local AI again.";
        }
        else
        {
            const auto venv = root.getChildFile ("venv");
            const auto createVenv = python + " -m venv " + quote (venv.getFullPathName());

            log << "Creating private ArtistStudio AI environment...\n";
            if (! runCommand (createVenv, log, 180000))
            {
                result.error = "Could not create the ArtistStudio AI environment.";
            }
            else
            {
                const auto managed = managedPythonLauncher();
                if (managed.isEmpty())
                {
                    result.error = "The private Python environment was created, but python.exe could not be found.";
                }
                else
                {
                    log << "Updating installer tools...\n";
                    const auto pipOk = runCommand (managed + " -m pip install --upgrade pip", log, 300000);

                    if (! pipOk)
                    {
                        result.error = "Could not update pip inside the ArtistStudio AI environment.";
                    }
                    else
                    {
                        log << "Installing Demucs 4.1.0 and its local AI dependencies...\n";
                        const auto installOk = runCommand (managed + " -m pip install demucs==4.1.0", log, 1200000);

                        if (! installOk)
                        {
                            result.error = "Demucs installation failed. Check your internet connection and try Install Local AI again.";
                        }
                        else if (! commandWorks (managed + " -m demucs --help"))
                        {
                            result.error = "The AI engine installed but failed its verification check.";
                        }
                        else
                        {
                            result.ok = true;
                            result.launcher = managed;
                            result.log = log;
                        }
                    }
                }
            }
        }
    }

    if (! result.ok)
        result.log = log;

    auto completion = onSetupComplete;
    juce::MessageManager::callAsync ([completion = std::move (completion), result = std::move (result)] () mutable
    {
        if (completion)
            completion (std::move (result));
    });
}

void StemEngine::runSeparation()
{
    StemSeparationResult result;

    if (! source.existsAsFile())
    {
        result.error = "Source audio file is missing.";
    }
    else if (launcher.isEmpty())
    {
        result.error = "The local AI engine is not ready. Press Install Local AI first.";
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
