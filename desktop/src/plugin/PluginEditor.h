#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../MainComponent.h"

class StemLabAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit StemLabAudioProcessorEditor (StemLabAudioProcessor& processor);
    ~StemLabAudioProcessorEditor() override = default;

    void resized() override;

private:
    StemLabAudioProcessor& processor;
    MainComponent stemLab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StemLabAudioProcessorEditor)
};
