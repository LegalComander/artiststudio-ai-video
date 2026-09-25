#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class NeonRackLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    NeonRackLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

class NeonRackEditor final : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit NeonRackEditor (NeonRackProcessor&);
    ~NeonRackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshRack();
    void selectSlot (int slot);
    void rebuildControls();
    void setBypassForSelected (bool bypassed);

    NeonRackProcessor& processor;
    NeonRackLookAndFeel lookAndFeel;
    int selectedSlot = 0;
    NeonRackProcessor::Module selectedModule = NeonRackProcessor::Module::empty;

    juce::Label brand;
    juce::Label title;
    juce::Label subtitle;
    juce::Label selectedTitle;
    juce::Label hint;

    std::array<juce::ComboBox, NeonRackProcessor::numRackSlots> slotMenus;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> editButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> upButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> downButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> removeButtons;
    std::array<juce::ToggleButton, NeonRackProcessor::numRackSlots> bypassButtons;

    juce::Slider globalMix;
    juce::Slider output;
    juce::Label globalMixLabel;
    juce::Label outputLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> globalMixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::vector<std::unique_ptr<juce::Slider>> controlKnobs;
    std::vector<std::unique_ptr<juce::Label>> controlLabels;
    std::vector<std::unique_ptr<SliderAttachment>> controlAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonRackEditor)
};
