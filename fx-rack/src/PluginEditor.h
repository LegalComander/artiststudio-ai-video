#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Host-safe editor for NEONRACK. The first public beta used a continuously
// refreshing, resizable drag UI. Some hosts (and pluginval's editor lifecycle
// test) can create/destroy VST3 views in unusual orders, so this editor keeps
// ownership and callbacks deliberately simple.
class NeonRackEditor final : public juce::AudioProcessorEditor
{
public:
    explicit NeonRackEditor (NeonRackProcessor&);
    ~NeonRackEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void refreshRack();
    void selectSlot (int slot);
    void rebuildControls();
    void configureKnob (juce::Slider&, juce::Colour accent);
    void configureLabel (juce::Label&, const juce::String&, juce::Justification = juce::Justification::centred);
    void configureSmallButton (juce::Button&);

    NeonRackProcessor& processor;
    int selectedSlot = 0;
    NeonRackProcessor::Module selectedModule = NeonRackProcessor::Module::empty;

    juce::Label brand;
    juce::Label title;
    juce::Label subtitle;
    juce::Label presetLabel;
    juce::Label selectedTitle;
    juce::Label hint;

    juce::ComboBox presetMenu;
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
    std::unique_ptr<SliderAttachment> globalMixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::array<juce::Slider, 4> macroKnobs;
    std::array<juce::Label, 4> macroLabels;
    std::array<std::unique_ptr<SliderAttachment>, 4> macroAttachments;

    std::vector<std::unique_ptr<juce::Slider>> controlKnobs;
    std::vector<std::unique_ptr<juce::Label>> controlLabels;
    std::vector<std::unique_ptr<SliderAttachment>> controlAttachments;
    std::vector<std::unique_ptr<juce::ToggleButton>> controlToggles;
    std::vector<std::unique_ptr<ButtonAttachment>> controlToggleAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonRackEditor)
};
