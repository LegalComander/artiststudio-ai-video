#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

class NeonRackProcessor final : public juce::AudioProcessor
{
public:
    enum class Module : int
    {
        empty = 0,
        filter,
        rift,
        aura,
        chorus,
        prismDelay,
        orbitEQ,
        photonPhaser,
        spaceReverb,
        pulseComp
    };

    static constexpr int numRackSlots = 6;

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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState state;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();

    Module getSlot (int slot) const noexcept;
    void setSlot (int slot, Module module);
    void removeSlot (int slot);
    void moveSlot (int from, int to);

    void applyFactoryPreset (int index);
    static juce::StringArray factoryPresetNames();

    static juce::String moduleName (Module);
    static juce::Colour moduleColour (Module);
    static juce::String enabledParameter (Module);
    static juce::StringArray moduleParameterIds (Module);
    static juce::String parameterLabel (const juce::String& id);
    static juce::StringArray tempoDivisionNames();
    static juce::StringArray lfoTargetNames();

    double getHostBpm() const noexcept { return hostBpm.load(); }
    float getLfoVisual() const noexcept { return lfoVisual.load(); }
    float getOutputMeter() const noexcept { return outputMeter.load(); }

private:
    float value (const char* id) const;
    bool moduleEnabled (Module module) const;
    void processModule (Module module, juce::AudioBuffer<float>& buffer);
    void restoreRackFromState();
    void storeRackToState();
    void setPlainParameter (const juce::String& id, float plainValue);
    void enableAllModules();
    void updateHostTempo();
    void updateGlobalLfo (int samples);
    float syncedMilliseconds (int divisionIndex) const;
    float syncedRateHz (int divisionIndex) const;
    float activeLfoForTarget (int targetIndex) const noexcept;

    static float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }

    std::array<std::atomic<int>, numRackSlots> rack;
    double sr = 44100.0;

    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Reverb reverb;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> eqLow;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> eqFocus;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> eqAir;

    std::array<float, 2> auraLP { 0.0f, 0.0f };
    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float delayPhase = 0.0f;
    double globalLfoPhase = 0.0;
    float globalLfoSample = 0.0f;
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> moduleDryBuffer;

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<float> lfoVisual { 0.0f };
    std::atomic<float> outputMeter { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonRackProcessor)
};
