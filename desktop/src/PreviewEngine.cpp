#include "PreviewEngine.h"

PreviewEngine::PreviewEngine()
{
    formats.registerBasicFormats();

    for (int i = 0; i < trackCount; ++i)
    {
        gains[(size_t) i].store (1.0f);
        mutes[(size_t) i].store (false);
        solos[(size_t) i].store (false);
    }
}

PreviewEngine::~PreviewEngine()
{
    stop();
    releaseResources();

    const juce::ScopedLock lock (loadLock);
    for (auto& player : players)
    {
        player.transport.setSource (nullptr);
        player.readerSource.reset();
    }
}

void PreviewEngine::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    preparedBlockSize = juce::jmax (64, samplesPerBlockExpected);
    preparedSampleRate = sampleRate > 1000.0 ? sampleRate : 44100.0;
    scratch.setSize (2, preparedBlockSize, false, true, true);

    const juce::ScopedLock lock (loadLock);
    for (auto& player : players)
        player.transport.prepareToPlay (preparedBlockSize, preparedSampleRate);
}

void PreviewEngine::releaseResources()
{
    const juce::ScopedLock lock (loadLock);
    for (auto& player : players)
        player.transport.releaseResources();
    scratch.setSize (0, 0);
}

bool PreviewEngine::loadTrack (Track track, const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    auto reader = std::unique_ptr<juce::AudioFormatReader> (formats.createReaderFor (file));
    if (reader == nullptr)
        return false;

    const auto sourceRate = reader->sampleRate;
    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);
    const auto index = indexFor (track);

    const juce::ScopedLock lock (loadLock);
    auto& player = players[(size_t) index];
    player.transport.stop();
    player.transport.setSource (nullptr);
    player.readerSource.reset();
    player.readerSource = std::move (newSource);
    player.file = file;
    player.transport.setSource (player.readerSource.get(), 0, nullptr, sourceRate, 2);
    player.transport.prepareToPlay (preparedBlockSize, preparedSampleRate);
    player.transport.setPosition (0.0);
    return true;
}

void PreviewEngine::clearTrack (Track track)
{
    const auto index = indexFor (track);
    const juce::ScopedLock lock (loadLock);
    auto& player = players[(size_t) index];
    player.transport.stop();
    player.transport.setSource (nullptr);
    player.readerSource.reset();
    player.file = {};
}

void PreviewEngine::clearStems()
{
    stop();
    for (int i = firstStem; i < trackCount; ++i)
        clearTrack ((Track) i);
}

void PreviewEngine::stopAllTransports()
{
    for (auto& player : players)
        player.transport.stop();
}

void PreviewEngine::rewindStemTransports()
{
    for (int i = firstStem; i < trackCount; ++i)
        if (players[(size_t) i].readerSource != nullptr)
            players[(size_t) i].transport.setPosition (0.0);
}

void PreviewEngine::playOriginal()
{
    const juce::ScopedLock lock (loadLock);
    stopAllTransports();

    auto& player = players[(size_t) indexFor (Track::original)];
    if (player.readerSource == nullptr)
    {
        mode.store (Mode::stopped);
        playing.store (false);
        return;
    }

    player.transport.setPosition (0.0);
    player.transport.start();
    mode.store (Mode::original);
    playing.store (true);
}

void PreviewEngine::playStemMix()
{
    const juce::ScopedLock lock (loadLock);
    stopAllTransports();

    bool hasAnyStem = false;
    rewindStemTransports();
    for (int i = firstStem; i < trackCount; ++i)
    {
        auto& player = players[(size_t) i];
        if (player.readerSource != nullptr)
        {
            player.transport.start();
            hasAnyStem = true;
        }
    }

    mode.store (hasAnyStem ? Mode::stems : Mode::stopped);
    playing.store (hasAnyStem);
}

void PreviewEngine::stop()
{
    const juce::ScopedLock lock (loadLock);
    stopAllTransports();
    mode.store (Mode::stopped);
    playing.store (false);
}

void PreviewEngine::setStemGain (Track track, float gain01)
{
    const auto index = indexFor (track);
    if (index >= firstStem && index < trackCount)
        gains[(size_t) index].store (juce::jlimit (0.0f, 1.25f, gain01));
}

void PreviewEngine::setStemMute (Track track, bool shouldMute)
{
    const auto index = indexFor (track);
    if (index >= firstStem && index < trackCount)
        mutes[(size_t) index].store (shouldMute);
}

void PreviewEngine::setStemSolo (Track track, bool shouldSolo)
{
    const auto index = indexFor (track);
    if (index >= firstStem && index < trackCount)
        solos[(size_t) index].store (shouldSolo);
}

bool PreviewEngine::getStemMute (Track track) const
{
    const auto index = indexFor (track);
    return index >= firstStem && index < trackCount ? mutes[(size_t) index].load() : false;
}

bool PreviewEngine::getStemSolo (Track track) const
{
    const auto index = indexFor (track);
    return index >= firstStem && index < trackCount ? solos[(size_t) index].load() : false;
}

float PreviewEngine::getStemGain (Track track) const
{
    const auto index = indexFor (track);
    return index >= firstStem && index < trackCount ? gains[(size_t) index].load() : 1.0f;
}

bool PreviewEngine::anyStemSoloed() const noexcept
{
    for (int i = firstStem; i < trackCount; ++i)
        if (solos[(size_t) i].load())
            return true;
    return false;
}

void PreviewEngine::getNextAudioBlock (const juce::AudioSourceChannelInfo& info)
{
    if (info.buffer == nullptr || info.numSamples <= 0)
        return;

    const auto currentMode = mode.load();
    if (currentMode == Mode::stopped)
    {
        info.clearActiveBufferRegion();
        return;
    }

    if (currentMode == Mode::original)
    {
        auto& player = players[(size_t) indexFor (Track::original)];
        if (player.readerSource == nullptr)
        {
            info.clearActiveBufferRegion();
            playing.store (false);
            mode.store (Mode::stopped);
            return;
        }

        player.transport.getNextAudioBlock (info);
        if (player.transport.hasStreamFinished())
        {
            player.transport.stop();
            playing.store (false);
            mode.store (Mode::stopped);
        }
        return;
    }

    info.clearActiveBufferRegion();
    const bool soloActive = anyStemSoloed();
    bool allFinished = true;

    if (scratch.getNumChannels() < info.buffer->getNumChannels()
        || scratch.getNumSamples() < info.numSamples)
        scratch.setSize (juce::jmax (2, info.buffer->getNumChannels()), info.numSamples, false, false, true);

    for (int i = firstStem; i < trackCount; ++i)
    {
        auto& player = players[(size_t) i];
        if (player.readerSource == nullptr)
            continue;

        scratch.clear();
        juce::AudioSourceChannelInfo stemInfo (&scratch, 0, info.numSamples);
        player.transport.getNextAudioBlock (stemInfo);

        if (! player.transport.hasStreamFinished())
            allFinished = false;

        const bool audible = ! mutes[(size_t) i].load()
                          && (! soloActive || solos[(size_t) i].load());
        if (! audible)
            continue;

        const float gain = gains[(size_t) i].load();
        const int channels = juce::jmin (info.buffer->getNumChannels(), scratch.getNumChannels());
        for (int ch = 0; ch < channels; ++ch)
            info.buffer->addFrom (ch, info.startSample, scratch, ch, 0, info.numSamples, gain);
    }

    if (allFinished)
    {
        stopAllTransports();
        playing.store (false);
        mode.store (Mode::stopped);
    }
}
