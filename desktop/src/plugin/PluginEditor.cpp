#include "PluginEditor.h"

StemLabAudioProcessorEditor::StemLabAudioProcessorEditor (StemLabAudioProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      stemLab (&p.getPreviewEngine())
{
    addAndMakeVisible (stemLab);
    setResizable (true, false);
    setResizeLimits (980, 760, 1500, 1080);
    setSize (1120, 860);
}

void StemLabAudioProcessorEditor::resized()
{
    stemLab.setBounds (getLocalBounds());
}
