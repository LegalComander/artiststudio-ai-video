#include "StemEngine.h"

namespace
{
juce::String quote (const juce::String& text)
{
    return "\"" + text.replace ("\"", "\\\"") + "\"";
}

bool commandWorks (const juce::String& command, int timeoutMs = 10000)
{
    juce::ChildProcess process;
    if (! process.start (command))
        return false;

    const bool finished = process.waitForProcessToFinish (timeoutMs);
    if (! finished)
    {
        process.kill();
        return false;
    }

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

    // Prefer Python 3.12 for the current Demucs/PyTorch Windows stack.
    if (localAppData.isNotEmpty())
    {
        for (const auto& folder : { "Python312", "Python311", "Python310", "Python313" })
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

    candidates.addArray ({ "py -3.12", "py -3.11", "py -3.10", "py -3.13", "py -3", "python" });
   #else
    juce::StringArray candidates { "python3", "python" };
   #endif

    for (const auto& candidate : candidates)
        if (commandWorks (candidate + " --version", 15000))
            return candidate;

    return {};
}

juce::String StemEngine::detectDemucsLauncher()
{
    const auto managed = managedPythonLauncher();
    if (managed.isNotEmpty() && commandWorks (managed + " -m demucs --help", 120000))
        return managed;

   #if JUCE_WINDOWS
    const juce::StringArray candidates { "py -3.12", "py -3.11", "py -3.10", "py -3.13", "py -3", "python" };
   #else
    const juce::StringArray candidates { "python3", "python" };
   #endif

    for (const auto& candidate : candidates)
        if (commandWorks (candidate + " -m demucs --help", 120000))
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
                                  ProgressCallback progress,
                                  Completion completion)
{
    if (isThreadRunning())
        return;

    source = std::move (sourceFile);
    output = std::move (outputRoot);
    maxQuality = maximumQuality;
    onProgress = std::move (progress);
    onComplete = std::move (completion);
    launcher = detectDemucsLauncher();
    job = Job::separation;
    startThread();
}

void StemEngine::requestCancel()
{
    signalThreadShouldExit();
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
        result.log = "Demucs is already available and verified.";
    }
    else
    {
        auto root = getManagedEngineFolder();
        root.createDirectory();

        auto python = detectSystemPython();

       #if JUCE_WINDOWS
        if (python.isEmpty() && commandWorks ("winget --version", 15000))
        {
            log << "Installing Python 3.12 with Windows Package Manager...\n";
            runCommand ("winget install --id Python.Python.3.12 -e --silent --accept-package-agreements --accept-source-agreements", log, 600000);
            python = detectSystemPython();
        }
       #endif

        if (python.isEmpty())
        {
            result.error = "Python 3.10+ was not found and could not be installed automatically. Install Python 3.12, then press Install Local AI again.";
        }
        else
        {
            // An interrupted or incompatible first install can leave a venv that
            // exists but cannot start Demucs. Rebuild it cleanly before retrying.
            const auto venv = root.getChildFile ("venv");
            if (venv.exists())
            {
                log << "Removing incomplete ArtistStudio AI environment...\n";
                venv.deleteRecursively();
            }

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
                    const auto pipOk = runCommand (managed + " -m pip install --upgrade pip setuptools wheel", log, 300000);

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
                        else
                        {
                            log << "Verifying Demucs. First launch can take up to two minutes on Windows...\n";
                            juce::String verifyLog;
                            const auto verified = runCommand (managed + " -m demucs --help", verifyLog, 120000);
                            log << verifyLog;

                            if (! verified)
                            {
                                const auto detail = verifyLog.substring (juce::jmax (0, verifyLog.length() - 900)).trim();
                                result.error = "The AI engine installed but could not start Demucs."
                                             + (detail.isNotEmpty() ? " Details: " + detail : juce::String());
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

void StemEngine::publishProgress (double progress, const juce::String& message)
{
    auto callback = onProgress;
    if (! callback)
        return;

    const auto clamped = juce::jlimit (0.0, 1.0, progress);
    juce::MessageManager::callAsync ([callback = std::move (callback), clamped, message]
    {
        callback (clamped, message);
    });
}

double StemEngine::extractProgress (const juce::String& text)
{
    double best = -1.0;
    for (int percent = 0; percent < text.length(); ++percent)
    {
        if (text[percent] != '%')
            continue;

        int begin = percent - 1;
        while (begin >= 0 && juce::CharacterFunctions::isWhitespace (text[begin]))
            --begin;

        const int end = begin;
        while (begin >= 0 && juce::CharacterFunctions::isDigit (text[begin]))
            --begin;

        const auto digits = text.substring (begin + 1, end + 1);
        if (digits.isNotEmpty())
        {
            const int value = digits.getIntValue();
            if (value >= 0 && value <= 100)
                best = juce::jmax (best, value / 100.0);
        }
    }
    return best;
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
        const juce::String model = maxQuality ? "htdemucs_ft" : "htdemucs";
        const auto command = launcher
                           + " -m demucs -n " + model
                           + " --out " + quote (output.getFullPathName())
                           + " " + quote (source.getFullPathName());

        publishProgress (0.02, juce::String ("Starting ") + model + " separation...");

        juce::ChildProcess process;
        if (! process.start (command))
        {
            result.error = "Could not start the local Demucs process.";
        }
        else
        {
            juce::String log;
            double lastProgress = 0.02;

            while (process.isRunning() && ! threadShouldExit())
            {
                const auto chunk = process.readAllProcessOutput();
                if (chunk.isNotEmpty())
                {
                    log << chunk;
                    const auto parsed = extractProgress (chunk);
                    if (parsed >= 0.0 && parsed > lastProgress + 0.005)
                    {
                        lastProgress = juce::jmin (0.97, parsed);
                        publishProgress (lastProgress,
                                         juce::String ("Separating stems... ")
                                             + juce::String ((int) std::round (lastProgress * 100.0)) + "%");
                    }
                }
                wait (150);
            }

            if (threadShouldExit())
            {
                process.kill();
                result.error = "Stem separation cancelled.";
                publishProgress (0.0, "Separation cancelled.");
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
                    publishProgress (0.99, "Finalizing WAV stems...");
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
                    else
                        publishProgress (1.0, "Four stems ready.");
                }
            }
        }
    }

    auto completion = onComplete;
    onProgress = {};
    juce::MessageManager::callAsync ([completion = std::move (completion), result = std::move (result)] () mutable
    {
        if (completion)
            completion (std::move (result));
    });
}
