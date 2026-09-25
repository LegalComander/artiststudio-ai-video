#include "PluginEditor.h"

StemLabAudioProcessorEditor::StemLabAudioProcessorEditor (StemLabAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (stemLab);
    setResizable (true, false);
    setResizeLimits (920, 660, 1440, 980);
    setSize (1120, 780);
}

void StemLabAudioProcessorEditor::resized()
{
    stemLab.setBounds (getLocalBounds());
}
