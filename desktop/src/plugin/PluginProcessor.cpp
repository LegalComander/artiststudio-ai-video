#include "PluginProcessor.h"
#include "PluginEditor.h"

StemLabAudioProcessor::StemLabAudioProcessor()
    : juce::AudioProcessor (juce::AudioProcessor::BusesProperties()
                                .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void StemLabAudioProcessor::prepareToPlay (double, int)
{
}

void StemLabAudioProcessor::releaseResources()
{
}

bool StemLabAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void StemLabAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* StemLabAudioProcessor::createEditor()
{
    return new StemLabAudioProcessorEditor (*this);
}

const juce::String StemLabAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

void StemLabAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    stream.writeString ("ArtistStudio Stem Lab VST3 v0.1");
}

void StemLabAudioProcessor::setStateInformation (const void*, int)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StemLabAudioProcessor();
}
