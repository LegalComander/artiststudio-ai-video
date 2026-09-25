#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class RackDragButton final : public juce::TextButton
{
public:
    RackDragButton() { setButtonText ("DRAG"); }
    void setSlotIndex (int newIndex) noexcept { slotIndex = newIndex; }

    void mouseDrag (const juce::MouseEvent& event) override
    {
        juce::TextButton::mouseDrag (event);
        if (event.getDistanceFromDragStart() < 5)
            return;

        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor (this))
            container->startDragging (juce::var (slotIndex), this);
    }

private:
    int slotIndex = 0;
};

class NeonRackEditor final : public juce::AudioProcessorEditor,
                             public juce::DragAndDropContainer,
                             public juce::DragAndDropTarget,
                             private juce::Timer
{
public:
    explicit NeonRackEditor (NeonRackProcessor&);
    ~NeonRackEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void refreshRack();
    void selectSlot (int slot);
    void rebuildControls();
    void configureKnob (juce::Slider&, juce::Colour accent);
    void configureLabel (juce::Label&, const juce::String&,
                         juce::Justification = juce::Justification::centred);
    void configureSmallButton (juce::TextButton&);
    void configureCombo (juce::ComboBox&);
    void moveCollapsedState (int from, int to);
    int slotAtY (int y) const;

    bool isInterestedInDragSource (const SourceDetails& details) override;
    void itemDragMove (const SourceDetails& details) override;
    void itemDragExit (const SourceDetails& details) override;
    void itemDropped (const SourceDetails& details) override;
    void timerCallback() override;

    NeonRackProcessor& processor;
    int selectedSlot = 0;
    int dragTargetSlot = -1;
    NeonRackProcessor::Module selectedModule = NeonRackProcessor::Module::empty;

    juce::Label brand;
    juce::Label title;
    juce::Label subtitle;
    juce::Label presetLabel;
    juce::Label selectedTitle;
    juce::Label hint;

    juce::ComboBox presetMenu;
    std::array<juce::ComboBox, NeonRackProcessor::numRackSlots> slotMenus;
    std::array<RackDragButton, NeonRackProcessor::numRackSlots> dragButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> editButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> collapseButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> upButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> downButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> removeButtons;
    std::array<juce::ToggleButton, NeonRackProcessor::numRackSlots> bypassButtons;
    std::array<bool, NeonRackProcessor::numRackSlots> collapsed {};
    std::array<juce::Rectangle<int>, NeonRackProcessor::numRackSlots> slotCardBounds;

    juce::Slider globalMix;
    juce::Slider output;
    juce::Label globalMixLabel;
    juce::Label outputLabel;
    std::unique_ptr<SliderAttachment> globalMixAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    std::array<juce::Slider, 4> macroKnobs;
    std::array<juce::Label, 4> macroLabels;
    std::array<std::unique_ptr<SliderAttachment>, 4> macroAttachments;

    juce::ToggleButton moduleSyncToggle { "HOST SYNC" };
    juce::ComboBox moduleDivision;
    juce::Label moduleSyncLabel;
    std::unique_ptr<ButtonAttachment> moduleSyncAttachment;
    std::unique_ptr<ComboBoxAttachment> moduleDivisionAttachment;

    juce::ToggleButton lfoOn { "LFO" };
    juce::ToggleButton lfoSync { "SYNC" };
    juce::Slider lfoRate;
    juce::Slider lfoDepth;
    juce::ComboBox lfoDivision;
    juce::ComboBox lfoShape;
    juce::ComboBox lfoTarget;
    juce::Label lfoRateLabel;
    juce::Label lfoDepthLabel;
    juce::Label lfoTargetLabel;
    juce::Label lfoShapeLabel;
    std::unique_ptr<ButtonAttachment> lfoOnAttachment;
    std::unique_ptr<ButtonAttachment> lfoSyncAttachment;
    std::unique_ptr<SliderAttachment> lfoRateAttachment;
    std::unique_ptr<SliderAttachment> lfoDepthAttachment;
    std::unique_ptr<ComboBoxAttachment> lfoDivisionAttachment;
    std::unique_ptr<ComboBoxAttachment> lfoShapeAttachment;
    std::unique_ptr<ComboBoxAttachment> lfoTargetAttachment;

    std::vector<std::unique_ptr<juce::Slider>> controlKnobs;
    std::vector<std::unique_ptr<juce::Label>> controlLabels;
    std::vector<std::unique_ptr<SliderAttachment>> controlAttachments;
    std::vector<std::unique_ptr<juce::ToggleButton>> controlToggles;
    std::vector<std::unique_ptr<ButtonAttachment>> controlToggleAttachments;

    juce::Rectangle<int> lfoPanelBounds;
    juce::Rectangle<int> waveformBounds;
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeonRackEditor)
};
