#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>

class PreviewEngine final : public juce::AudioSource
{
public:
    enum class Track : int
    {
        original = 0,
        vocals,
        drums,
        bass,
        other,
        count
    };

    PreviewEngine();
    ~PreviewEngine() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    bool loadTrack (Track track, const juce::File& file);
    void clearTrack (Track track);
    void clearStems();

    void playOriginal();
    void playStemMix();
    void stop();
    bool isPlaying() const noexcept { return playing.load(); }
    bool isPlayingOriginal() const noexcept { return mode.load() == Mode::original; }

    void setStemGain (Track track, float gain01);
    void setStemMute (Track track, bool shouldMute);
    void setStemSolo (Track track, bool shouldSolo);
    bool getStemMute (Track track) const;
    bool getStemSolo (Track track) const;
    float getStemGain (Track track) const;

private:
    enum class Mode : int
    {
        stopped = 0,
        original,
        stems
    };

    struct Player
    {
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        juce::AudioTransportSource transport;
        juce::File file;
    };

    static constexpr int trackCount = static_cast<int> (Track::count);
    static constexpr int firstStem = static_cast<int> (Track::vocals);

    int indexFor (Track track) const noexcept { return static_cast<int> (track); }
    bool anyStemSoloed() const noexcept;
    void stopAllTransports();
    void rewindStemTransports();

    juce::AudioFormatManager formats;
    std::array<Player, trackCount> players;
    std::array<std::atomic<float>, trackCount> gains;
    std::array<std::atomic<bool>, trackCount> mutes;
    std::array<std::atomic<bool>, trackCount> solos;

    std::atomic<Mode> mode { Mode::stopped };
    std::atomic<bool> playing { false };
    juce::CriticalSection loadLock;
    double preparedSampleRate = 44100.0;
    int preparedBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreviewEngine)
};
