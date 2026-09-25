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

class RackDragHandle final : public juce::Component
{
public:
    std::function<void (const juce::MouseEvent&)> onDown;
    std::function<void (const juce::MouseEvent&)> onDrag;
    std::function<void (const juce::MouseEvent&)> onUp;

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff6fd9ef));
        const auto c = getLocalBounds().getCentre();
        for (int y = -6; y <= 6; y += 6)
            g.drawLine ((float) c.x - 7.0f, (float) c.y + (float) y,
                        (float) c.x + 7.0f, (float) c.y + (float) y, 1.5f);
    }

    void mouseDown (const juce::MouseEvent& e) override { if (onDown) onDown (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (onDrag) onDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (onUp) onUp (e); }
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
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void refreshRack();
    void selectSlot (int slot);
    void rebuildControls();
    void beginRackDrag (int slot);
    void updateRackDrag (juce::Point<int> editorPoint);
    void endRackDrag();
    int slotAtPoint (juce::Point<int>) const;
    void styleKnob (juce::Slider&, juce::Colour accent);
    void styleSmallLabel (juce::Label&, const juce::String& text);

    NeonRackProcessor& processor;
    NeonRackLookAndFeel lookAndFeel;
    int selectedSlot = 0;
    NeonRackProcessor::Module selectedModule = NeonRackProcessor::Module::empty;
    int dragSource = -1;
    int dragTarget = -1;

    juce::Label brand;
    juce::Label title;
    juce::Label subtitle;
    juce::Label selectedTitle;
    juce::Label hint;
    juce::Label presetLabel;

    juce::ComboBox presetMenu;
    std::array<juce::ComboBox, NeonRackProcessor::numRackSlots> slotMenus;
    std::array<std::unique_ptr<RackDragHandle>, NeonRackProcessor::numRackSlots> dragHandles;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> editButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> upButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> downButtons;
    std::array<juce::TextButton, NeonRackProcessor::numRackSlots> removeButtons;
    std::array<juce::ToggleButton, NeonRackProcessor::numRackSlots> bypassButtons;
    std::array<juce::Rectangle<int>, NeonRackProcessor::numRackSlots> slotBounds;

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
