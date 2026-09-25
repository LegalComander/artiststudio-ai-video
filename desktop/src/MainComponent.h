#pragma once

#include <JuceHeader.h>
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
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void buttonClicked (juce::Button*) override;
    void chooseFile();
    void loadFile (const juce::File& file);
    void analyzeCurrentTrack();
    void separateCurrentTrack();
    void updateStemStatus (const StemSeparationResult& result);
    void setStatus (juce::String text, juce::Colour colour = juce::Colour (0xff86a9bc));
    static juce::String formatDuration (double seconds);

    NeonLookAndFeel lookAndFeel;
    juce::File currentFile;
    juce::File lastStemFolder;
    std::unique_ptr<juce::FileChooser> fileChooser;
    StemEngine stemEngine;

    juce::Label title;
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
    juce::TextButton separateButton { "Separate 4 Stems" };
    juce::TextButton outputButton { "Open Output Folder" };
    juce::ToggleButton maximumQuality { "Maximum quality (slower)" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
