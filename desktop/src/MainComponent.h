#pragma once

#include <JuceHeader.h>
#include "PreviewEngine.h"
#include "StemEngine.h"
#include "TrackAnalyzer.h"

class NeonLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

class MainComponent final : public juce::Component,
                            public juce::FileDragAndDropTarget,
                            private juce::Button::Listener
{
public:
    explicit MainComponent (PreviewEngine* sharedPreviewEngine = nullptr);
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    static constexpr int stemCount = 4;

    void buttonClicked (juce::Button*) override;
    void chooseFile();
    void loadFile (const juce::File& file);
    void analyzeCurrentTrack();
    void installLocalEngine();
    void refreshEngineButton();
    void separateCurrentTrack();
    void cancelSeparation();
    void updateStemStatus (const StemSeparationResult& result);
    void loadPreviewFiles (const StemSeparationResult& result);
    void resetStemMixer();
    void updateStemMixerState();
    void setStatus (juce::String text, juce::Colour colour = juce::Colour (0xff86a9bc));
    static juce::String formatDuration (double seconds);
    static PreviewEngine::Track previewTrackForStem (int index);

    NeonLookAndFeel lookAndFeel;
    juce::File currentFile;
    juce::File lastStemFolder;
    std::unique_ptr<juce::FileChooser> fileChooser;
    StemEngine stemEngine;

    PreviewEngine ownedPreviewEngine;
    PreviewEngine* previewEngine = nullptr;
    bool ownsPreviewAudioDevice = false;
    juce::AudioDeviceManager previewDeviceManager;
    juce::AudioSourcePlayer previewSourcePlayer;

    juce::AudioFormatManager thumbnailFormats;
    juce::AudioThumbnailCache thumbnailCache { 6 };
    juce::AudioThumbnail thumbnail { 512, thumbnailFormats, thumbnailCache };
    juce::Rectangle<int> waveformBounds;

    juce::Label title;
    juce::Label versionLabel;
    juce::Label subtitle;
    juce::Label fileName;
    juce::Label status;
    juce::Label bpmValue;
    juce::Label keyValue;
    juce::Label durationValue;
    juce::Label sampleRateValue;
    juce::Label vocalsStatus;
    juce::Label drumsStatus;
    juce::Label bassStatus;
    juce::Label otherStatus;

    juce::TextButton chooseButton { "Choose Track" };
    juce::TextButton analyzeButton { "Analyze BPM + Key" };
    juce::TextButton engineButton { "Install Local AI" };
    juce::TextButton separateButton { "Separate 4 Stems" };
    juce::TextButton cancelButton { "Cancel" };
    juce::TextButton outputButton { "Open Output Folder" };
    juce::TextButton playOriginalButton { "Play Original" };
    juce::TextButton playStemsButton { "Play Stem Mix" };
    juce::TextButton stopPreviewButton { "Stop" };
    juce::ToggleButton maximumQuality { "Maximum quality (slower)" };

    std::array<juce::ToggleButton, stemCount> stemSoloButtons;
    std::array<juce::ToggleButton, stemCount> stemMuteButtons;
    std::array<juce::Slider, stemCount> stemVolumeSliders;

    double separationProgress = 0.0;
    juce::ProgressBar separationProgressBar { separationProgress };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
