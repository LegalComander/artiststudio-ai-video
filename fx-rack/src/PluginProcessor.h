#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class NeonRackProcessor final : public juce::AudioProcessor
{
public:
    NeonRackProcessor();
    ~NeonRackProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

private:
    float value (const char* id) const;
    bool enabled (const char* id) const;
    static float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }
    static float sat (float x) { return std::tanh (x); }

    double sr = 44100.0;

    std::array<float, 2> filterLP { 0.0f, 0.0f };
    std::array<float, 2> filterPrev { 0.0f, 0.0f };
    std::array<float, 2> auraLP { 0.0f, 0.0f };
    std::array<float, 2> eqLowLP { 0.0f, 0.0f };
    std::array<float, 2> eqAirLP { 0.0f, 0.0f };

    std::vector<float> chorusL, chorusR;
    std::vector<float> delayL, delayR;
    int chorusWrite = 0;
    int delayWrite = 0;
    float chorusPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonRackProcessor)
};
