#include "PluginEditor.h"

namespace
{
const auto bg = juce::Colour (0xff02060c);
const auto panel = juce::Colour (0xff07121f);
const auto panel2 = juce::Colour (0xff091827);
const auto panel3 = juce::Colour (0xff0b1d2e);
const auto line = juce::Colour (0xff17384e);
const auto cyan = juce::Colour (0xff43e9ff);
const auto blue = juce::Colour (0xff3a7cff);
const auto muted = juce::Colour (0xff86a8bb);
const auto white = juce::Colour (0xffeefbff);

float previewWave (int shape, float phase)
{
    phase -= std::floor (phase);
    switch (shape)
    {
        case 1: return 1.0f - 4.0f * std::abs (phase - 0.5f);
        case 2: return phase < 0.5f ? 1.0f : -1.0f;
        case 3: return phase * 2.0f - 1.0f;
        default: return std::sin (phase * juce::MathConstants<float>::twoPi);
    }
}
}

NeonRackEditor::NeonRackEditor (NeonRackProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Fixed outer size keeps the host-safe behaviour validated in Ableton and
    // pluginval. The rack itself is now draggable/collapsible inside the view.
    setSize (1180, 800);
    setOpaque (true);

    configureLabel (brand, "ARTISTSTUDIO / FX LAB", juce::Justification::centredLeft);
    brand.setColour (juce::Label::textColourId, muted);
    brand.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));

    configureLabel (title, "NEONRACK FX", juce::Justification::centredLeft);
    title.setColour (juce::Label::textColourId, white);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));

    configureLabel (subtitle, "MODULAR MULTI-FX / DRAG - MODULATE - SYNC", juce::Justification::centredLeft);
    subtitle.setColour (juce::Label::textColourId, cyan);
    subtitle.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));

    configureLabel (presetLabel, "FACTORY PRESET", juce::Justification::centredLeft);
    configureCombo (presetMenu);
    presetMenu.addItemList (NeonRackProcessor::factoryPresetNames(), 1);
    presetMenu.setSelectedId (1, juce::dontSendNotification);
    presetMenu.onChange = [this]
    {
        const auto index = presetMenu.getSelectedItemIndex();
        if (index < 0)
            return;

        processor.applyFactoryPreset (index);
        collapsed.fill (false);
        refreshRack();
        selectSlot (0);
    };

    configureLabel (selectedTitle, "", juce::Justification::centredLeft);
    selectedTitle.setColour (juce::Label::textColourId, white);
    selectedTitle.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));

    configureLabel (hint,
                    "MODULE CONTROL  |  Drag a rack card to reorder. Collapse cards you are not editing.",
                    juce::Justification::centredLeft);
    hint.setColour (juce::Label::textColourId, muted);
    hint.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto& menu = slotMenus[(size_t) i];
        configureCombo (menu);
        menu.addItem ("+ ADD FX", 1);
        for (int m = (int) NeonRackProcessor::Module::filter;
             m <= (int) NeonRackProcessor::Module::pulseComp; ++m)
            menu.addItem (NeonRackProcessor::moduleName ((NeonRackProcessor::Module) m), m + 1);

        menu.onChange = [this, i]
        {
            const auto selected = slotMenus[(size_t) i].getSelectedId();
            if (selected <= 0)
                return;

            processor.setSlot (i, (NeonRackProcessor::Module) (selected - 1));
            collapsed[(size_t) i] = false;
            refreshRack();
            selectSlot (i);
        };

        dragButtons[(size_t) i].setSlotIndex (i);
        dragButtons[(size_t) i].setTooltip ("Drag this effect to another rack position");
        editButtons[(size_t) i].setButtonText ("EDIT");
        collapseButtons[(size_t) i].setButtonText ("-");
        upButtons[(size_t) i].setButtonText ("UP");
        downButtons[(size_t) i].setButtonText ("DN");
        removeButtons[(size_t) i].setButtonText ("X");
        bypassButtons[(size_t) i].setButtonText ("BYP");

        upButtons[(size_t) i].setTooltip ("Move effect up");
        downButtons[(size_t) i].setTooltip ("Move effect down");
        removeButtons[(size_t) i].setTooltip ("Remove effect from rack");
        bypassButtons[(size_t) i].setTooltip ("Bypass this effect without removing it");
        collapseButtons[(size_t) i].setTooltip ("Collapse or expand this rack card");

        configureSmallButton (dragButtons[(size_t) i]);
        configureSmallButton (editButtons[(size_t) i]);
        configureSmallButton (collapseButtons[(size_t) i]);
        configureSmallButton (upButtons[(size_t) i]);
        configureSmallButton (downButtons[(size_t) i]);
        configureSmallButton (removeButtons[(size_t) i]);

        bypassButtons[(size_t) i].setColour (juce::ToggleButton::textColourId, white);
        bypassButtons[(size_t) i].setColour (juce::ToggleButton::tickColourId, cyan);
        bypassButtons[(size_t) i].setColour (juce::ToggleButton::tickDisabledColourId, muted);
        addAndMakeVisible (bypassButtons[(size_t) i]);

        editButtons[(size_t) i].onClick = [this, i] { selectSlot (i); };
        collapseButtons[(size_t) i].onClick = [this, i]
        {
            collapsed[(size_t) i] = ! collapsed[(size_t) i];
            collapseButtons[(size_t) i].setButtonText (collapsed[(size_t) i] ? "+" : "-");
            resized();
            repaint();
        };
        upButtons[(size_t) i].onClick = [this, i]
        {
            if (i <= 0)
                return;
            processor.moveSlot (i, i - 1);
            moveCollapsedState (i, i - 1);
            refreshRack();
            selectSlot (i - 1);
        };
        downButtons[(size_t) i].onClick = [this, i]
        {
            if (i + 1 >= NeonRackProcessor::numRackSlots)
                return;
            processor.moveSlot (i, i + 1);
            moveCollapsedState (i, i + 1);
            refreshRack();
            selectSlot (i + 1);
        };
        removeButtons[(size_t) i].onClick = [this, i]
        {
            processor.removeSlot (i);
            collapsed[(size_t) i] = false;
            refreshRack();
            selectSlot (i);
        };
        bypassButtons[(size_t) i].onClick = [this, i]
        {
            const auto module = processor.getSlot (i);
            if (module == NeonRackProcessor::Module::empty)
                return;

            const auto id = NeonRackProcessor::enabledParameter (module);
            if (id.isEmpty())
                return;

            if (auto* param = processor.state.getParameter (id))
                param->setValueNotifyingHost (bypassButtons[(size_t) i].getToggleState() ? 0.0f : 1.0f);
        };
    }

    configureKnob (globalMix, cyan);
    configureLabel (globalMixLabel, "GLOBAL MIX");
    globalMixAttachment = std::make_unique<SliderAttachment> (processor.state, "globalMix", globalMix);

    configureKnob (output, juce::Colour (0xff63efb0));
    configureLabel (outputLabel, "OUTPUT");
    outputAttachment = std::make_unique<SliderAttachment> (processor.state, "output", output);

    const std::array<juce::String, 4> macroIds { "macroDrive", "macroSpace", "macroMotion", "macroTone" };
    const std::array<juce::String, 4> macroNames { "DRIVE", "SPACE", "MOTION", "TONE" };
    const std::array<juce::Colour, 4> macroColours {
        juce::Colour (0xffff5d94), juce::Colour (0xff6d8cff),
        juce::Colour (0xffc47cff), juce::Colour (0xff5ef0b1)
    };

    for (int i = 0; i < 4; ++i)
    {
        configureKnob (macroKnobs[(size_t) i], macroColours[(size_t) i]);
        configureLabel (macroLabels[(size_t) i], macroNames[(size_t) i]);
        macroAttachments[(size_t) i] = std::make_unique<SliderAttachment> (
            processor.state, macroIds[(size_t) i], macroKnobs[(size_t) i]);
    }

    // Per-module tempo controls. They stay hidden until a time-based module is selected.
    moduleSyncToggle.setColour (juce::ToggleButton::textColourId, white);
    moduleSyncToggle.setColour (juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible (moduleSyncToggle);
    configureCombo (moduleDivision);
    moduleDivision.addItemList (NeonRackProcessor::tempoDivisionNames(), 1);
    configureLabel (moduleSyncLabel, "TEMPO", juce::Justification::centredLeft);
    moduleSyncToggle.setVisible (false);
    moduleDivision.setVisible (false);
    moduleSyncLabel.setVisible (false);

    // Global modulation panel.
    lfoOn.setColour (juce::ToggleButton::textColourId, white);
    lfoOn.setColour (juce::ToggleButton::tickColourId, cyan);
    lfoSync.setColour (juce::ToggleButton::textColourId, white);
    lfoSync.setColour (juce::ToggleButton::tickColourId, cyan);
    addAndMakeVisible (lfoOn);
    addAndMakeVisible (lfoSync);

    configureKnob (lfoRate, juce::Colour (0xffa977ff));
    configureKnob (lfoDepth, cyan);
    configureLabel (lfoRateLabel, "RATE");
    configureLabel (lfoDepthLabel, "DEPTH");
    configureLabel (lfoTargetLabel, "TARGET", juce::Justification::centredLeft);
    configureLabel (lfoShapeLabel, "SHAPE", juce::Justification::centredLeft);

    configureCombo (lfoDivision);
    lfoDivision.addItemList (NeonRackProcessor::tempoDivisionNames(), 1);
    configureCombo (lfoShape);
    lfoShape.addItemList ({ "SINE", "TRIANGLE", "SQUARE", "SAW" }, 1);
    configureCombo (lfoTarget);
    lfoTarget.addItemList (NeonRackProcessor::lfoTargetNames(), 1);

    lfoOnAttachment = std::make_unique<ButtonAttachment> (processor.state, "lfoOn", lfoOn);
    lfoSyncAttachment = std::make_unique<ButtonAttachment> (processor.state, "lfoSync", lfoSync);
    lfoRateAttachment = std::make_unique<SliderAttachment> (processor.state, "lfoRate", lfoRate);
    lfoDepthAttachment = std::make_unique<SliderAttachment> (processor.state, "lfoDepth", lfoDepth);
    lfoDivisionAttachment = std::make_unique<ComboBoxAttachment> (processor.state, "lfoDivision", lfoDivision);
    lfoShapeAttachment = std::make_unique<ComboBoxAttachment> (processor.state, "lfoShape", lfoShape);
    lfoTargetAttachment = std::make_unique<ComboBoxAttachment> (processor.state, "lfoTarget", lfoTarget);

    refreshRack();
    selectSlot (0);
    startTimerHz (24);
}

NeonRackEditor::~NeonRackEditor()
{
    stopTimer();
}

void NeonRackEditor::configureKnob (juce::Slider& slider, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff173b55));
    slider.setColour (juce::Slider::thumbColourId, white);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel3);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
}

void NeonRackEditor::configureLabel (juce::Label& label,
                                     const juce::String& text,
                                     juce::Justification justification)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (justification);
    label.setColour (juce::Label::textColourId, muted);
    label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    addAndMakeVisible (label);
}

void NeonRackEditor::configureSmallButton (juce::TextButton& button)
{
    button.setColour (juce::TextButton::buttonColourId, panel3);
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff12344b));
    button.setColour (juce::TextButton::textColourOffId, white);
    button.setColour (juce::TextButton::textColourOnId, cyan);
    addAndMakeVisible (button);
}

void NeonRackEditor::configureCombo (juce::ComboBox& combo)
{
    combo.setColour (juce::ComboBox::backgroundColourId, panel3);
    combo.setColour (juce::ComboBox::textColourId, white);
    combo.setColour (juce::ComboBox::outlineColourId, line.withAlpha (0.75f));
    combo.setColour (juce::ComboBox::arrowColourId, cyan);
    addAndMakeVisible (combo);
}

void NeonRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    juce::ColourGradient background (juce::Colour (0xff0a2943), 0.0f, 0.0f,
                                     bg, (float) getWidth(), (float) getHeight(), false);
    background.addColour (0.32, juce::Colour (0xff061523));
    background.addColour (0.70, juce::Colour (0xff040a11));
    g.setGradientFill (background);
    g.fillRect (getLocalBounds());

    auto content = getLocalBounds().reduced (16);
    auto header = content.removeFromTop (138).toFloat();
    g.setColour (juce::Colour (0xff04101b));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (cyan.withAlpha (0.22f));
    g.drawRoundedRectangle (header, 18.0f, 1.0f);
    g.setColour (cyan.withAlpha (0.65f));
    g.fillRoundedRectangle (header.removeFromBottom (3.0f), 1.5f);

    for (const auto& label : macroLabels)
    {
        const auto index = (size_t) (&label - &macroLabels[0]);
        auto card = label.getBounds().getUnion (macroKnobs[index].getBounds()).expanded (6, 4).toFloat();
        g.setColour (panel2.withAlpha (0.80f));
        g.fillRoundedRectangle (card, 10.0f);
        g.setColour (line.withAlpha (0.72f));
        g.drawRoundedRectangle (card, 10.0f, 1.0f);
    }

    auto mixCard = globalMixLabel.getBounds().getUnion (globalMix.getBounds()).expanded (6, 4).toFloat();
    auto outCard = outputLabel.getBounds().getUnion (output.getBounds()).expanded (6, 4).toFloat();
    g.setColour (panel2.withAlpha (0.80f));
    g.fillRoundedRectangle (mixCard, 10.0f);
    g.fillRoundedRectangle (outCard, 10.0f);
    g.setColour (line.withAlpha (0.72f));
    g.drawRoundedRectangle (mixCard, 10.0f, 1.0f);
    g.drawRoundedRectangle (outCard, 10.0f, 1.0f);

    content.removeFromTop (12);
    auto rackPanel = content.removeFromLeft (390).toFloat();
    g.setColour (juce::Colour (0xff04101a));
    g.fillRoundedRectangle (rackPanel, 18.0f);
    g.setColour (line);
    g.drawRoundedRectangle (rackPanel, 18.0f, 1.0f);

    content.removeFromLeft (12);
    auto controlsPanel = content.toFloat();
    g.setColour (juce::Colour (0xff04101a));
    g.fillRoundedRectangle (controlsPanel, 18.0f);

    const auto selectedAccent = selectedModule == NeonRackProcessor::Module::empty
                                    ? cyan
                                    : NeonRackProcessor::moduleColour (selectedModule);
    g.setColour (selectedAccent.withAlpha (0.48f));
    g.drawRoundedRectangle (controlsPanel, 18.0f, 1.4f);
    g.setColour (selectedAccent.withAlpha (0.80f));
    g.fillRoundedRectangle (controlsPanel.withHeight (3.0f).reduced (20.0f, 0.0f), 1.5f);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("FX CHAIN  /  DRAG TO REORDER", rackPanel.toNearestInt().withTrimmedLeft (18).removeFromTop (34),
                juce::Justification::centredLeft, false);

    // Rack cards are painted from stored bounds so drag targeting and visuals agree exactly.
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        const auto card = slotCardBounds[(size_t) i].toFloat();
        if (card.isEmpty())
            continue;

        const auto module = processor.getSlot (i);
        const auto occupied = module != NeonRackProcessor::Module::empty;
        const auto accent = occupied ? NeonRackProcessor::moduleColour (module) : line;
        const bool selected = i == selectedSlot;
        const bool dropTarget = i == dragTargetSlot;

        g.setColour ((selected ? panel3 : panel2).withAlpha (occupied ? 0.97f : 0.62f));
        g.fillRoundedRectangle (card, 11.0f);

        g.setColour ((dropTarget ? cyan : (selected ? accent : line)).withAlpha (dropTarget ? 1.0f : 0.76f));
        g.drawRoundedRectangle (card, 11.0f, dropTarget ? 2.5f : (selected ? 1.8f : 1.0f));

        auto rail = card.withWidth (4.0f).reduced (0.0f, 8.0f);
        g.setColour (accent.withAlpha (occupied ? 0.95f : 0.32f));
        g.fillRoundedRectangle (rail, 2.0f);

        auto badge = juce::Rectangle<float> (card.getX() + 8.0f, card.getY() + 9.0f, 23.0f, 23.0f);
        g.setColour (accent.withAlpha (occupied ? 0.18f : 0.08f));
        g.fillEllipse (badge);
        g.setColour (occupied ? accent : muted.withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (juce::String (i + 1), badge.toNearestInt(), juce::Justification::centred, false);

        if (collapsed[(size_t) i] && occupied)
        {
            g.setColour (muted.withAlpha (0.75f));
            g.setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
            g.drawText ("COLLAPSED", card.toNearestInt().removeFromRight (80).reduced (4),
                        juce::Justification::centredRight, false);
        }
    }

    for (size_t i = 0; i < controlKnobs.size(); ++i)
    {
        auto card = controlLabels[i]->getBounds().getUnion (controlKnobs[i]->getBounds()).expanded (7, 5).toFloat();
        g.setColour (panel2.withAlpha (0.72f));
        g.fillRoundedRectangle (card, 13.0f);
        g.setColour (selectedAccent.withAlpha (0.18f));
        g.drawRoundedRectangle (card, 13.0f, 1.0f);
    }

    // LFO / modulation panel with a live waveform marker.
    auto lfoPanel = lfoPanelBounds.toFloat();
    g.setColour (panel2.withAlpha (0.90f));
    g.fillRoundedRectangle (lfoPanel, 14.0f);
    g.setColour (juce::Colour (0xffa977ff).withAlpha (0.50f));
    g.drawRoundedRectangle (lfoPanel, 14.0f, 1.2f);
    g.setColour (juce::Colour (0xffa977ff));
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("MOD LFO", lfoPanelBounds.reduced (12).removeFromTop (20), juce::Justification::centredLeft, false);

    if (! waveformBounds.isEmpty())
    {
        auto wave = waveformBounds.toFloat();
        g.setColour (juce::Colour (0xff06111d));
        g.fillRoundedRectangle (wave, 9.0f);
        g.setColour (line);
        g.drawRoundedRectangle (wave, 9.0f, 1.0f);

        juce::Path path;
        const int shape = juce::jmax (0, lfoShape.getSelectedItemIndex());
        for (int x = 0; x < waveformBounds.getWidth(); ++x)
        {
            const float phase = (float) x / (float) juce::jmax (1, waveformBounds.getWidth() - 1);
            const float y = wave.getCentreY() - previewWave (shape, phase) * wave.getHeight() * 0.34f;
            if (x == 0)
                path.startNewSubPath (wave.getX(), y);
            else
                path.lineTo (wave.getX() + (float) x, y);
        }
        g.setColour (juce::Colour (0xffa977ff));
        g.strokePath (path, juce::PathStrokeType (1.8f));

        const float live = processor.getLfoVisual();
        const float markerY = wave.getCentreY() - live * wave.getHeight() * 0.34f;
        g.setColour (cyan);
        g.fillEllipse (wave.getRight() - 9.0f, markerY - 4.0f, 8.0f, 8.0f);
    }

    // Output meter and host tempo display.
    if (! meterBounds.isEmpty())
    {
        auto meter = meterBounds.toFloat();
        g.setColour (juce::Colour (0xff07121f));
        g.fillRoundedRectangle (meter, 4.0f);
        const float level = processor.getOutputMeter();
        auto fill = meter.withTop (meter.getBottom() - meter.getHeight() * level);
        g.setColour (level > 0.92f ? juce::Colour (0xffff5d78) : juce::Colour (0xff5ef0b1));
        g.fillRoundedRectangle (fill, 4.0f);
    }

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText (juce::String (processor.getHostBpm(), 1) + " BPM",
                lfoPanelBounds.reduced (12).removeFromTop (20).removeFromRight (100),
                juce::Justification::centredRight, false);
}

void NeonRackEditor::resized()
{
    auto content = getLocalBounds().reduced (26);
    auto header = content.removeFromTop (112);

    auto brandArea = header.removeFromLeft (320);
    brand.setBounds (brandArea.removeFromTop (17));
    title.setBounds (brandArea.removeFromTop (40));
    subtitle.setBounds (brandArea.removeFromTop (18));
    presetLabel.setBounds (brandArea.removeFromTop (14));
    presetMenu.setBounds (brandArea.removeFromTop (28).withTrimmedRight (22));

    auto macroArea = header.removeFromLeft (500);
    const int macroW = juce::jmax (1, macroArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
    {
        auto cell = macroArea.removeFromLeft (macroW).reduced (4, 0);
        macroLabels[(size_t) i].setBounds (cell.removeFromTop (18));
        macroKnobs[(size_t) i].setBounds (cell.reduced (5, 0));
    }

    auto globals = header.reduced (2, 0);
    auto mixArea = globals.removeFromLeft (globals.getWidth() / 2);
    globalMixLabel.setBounds (mixArea.removeFromTop (18));
    globalMix.setBounds (mixArea.reduced (5, 0));
    outputLabel.setBounds (globals.removeFromTop (18));
    output.setBounds (globals.reduced (5, 0));

    content.removeFromTop (42);
    auto rackArea = content.removeFromLeft (362).reduced (10, 4);
    rackArea.removeFromTop (22);
    content.removeFromLeft (40);
    auto controlArea = content.reduced (18, 4);

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        const int slotHeight = collapsed[(size_t) i] ? 47 : 78;
        auto card = rackArea.removeFromTop (slotHeight).reduced (4, 3);
        slotCardBounds[(size_t) i] = card;

        auto inner = card.reduced (12, 7);
        inner.removeFromLeft (22);
        auto topRow = inner.removeFromTop (28);
        dragButtons[(size_t) i].setBounds (topRow.removeFromLeft (46));
        topRow.removeFromLeft (4);
        collapseButtons[(size_t) i].setBounds (topRow.removeFromRight (28));
        topRow.removeFromRight (4);
        slotMenus[(size_t) i].setBounds (topRow);

        const bool expanded = ! collapsed[(size_t) i];
        editButtons[(size_t) i].setVisible (expanded);
        bypassButtons[(size_t) i].setVisible (expanded);
        upButtons[(size_t) i].setVisible (expanded);
        downButtons[(size_t) i].setVisible (expanded);
        removeButtons[(size_t) i].setVisible (expanded);

        if (expanded)
        {
            inner.removeFromTop (4);
            auto buttons = inner.removeFromTop (25);
            editButtons[(size_t) i].setBounds (buttons.removeFromLeft (48));
            buttons.removeFromLeft (3);
            bypassButtons[(size_t) i].setBounds (buttons.removeFromLeft (62));
            buttons.removeFromLeft (3);
            upButtons[(size_t) i].setBounds (buttons.removeFromLeft (38));
            buttons.removeFromLeft (2);
            downButtons[(size_t) i].setBounds (buttons.removeFromLeft (38));
            buttons.removeFromLeft (2);
            removeButtons[(size_t) i].setBounds (buttons.removeFromLeft (34));
        }
    }

    auto top = controlArea.removeFromTop (70);
    selectedTitle.setBounds (top.removeFromTop (38));
    hint.setBounds (top.removeFromTop (25));

    auto syncRow = controlArea.removeFromTop (38);
    if (moduleSyncToggle.isVisible())
    {
        moduleSyncLabel.setBounds (syncRow.removeFromLeft (55));
        moduleSyncToggle.setBounds (syncRow.removeFromLeft (105));
        syncRow.removeFromLeft (8);
        moduleDivision.setBounds (syncRow.removeFromLeft (100));
    }

    controlArea.removeFromTop (6);
    lfoPanelBounds = controlArea.removeFromBottom (154);
    controlArea.removeFromBottom (10);

    const int columns = 3;
    const int rows = 2;
    const int cellW = juce::jmax (1, controlArea.getWidth() / columns);
    const int cellH = juce::jmax (120, controlArea.getHeight() / rows);

    for (size_t i = 0; i < controlKnobs.size(); ++i)
    {
        const auto index = (int) i;
        const auto col = index % columns;
        const auto row = index / columns;
        if (row >= rows)
            break;

        auto cell = juce::Rectangle<int> (controlArea.getX() + col * cellW,
                                          controlArea.getY() + row * cellH,
                                          cellW, cellH).reduced (12, 8);
        controlLabels[i]->setBounds (cell.removeFromTop (22));
        controlKnobs[i]->setBounds (cell.reduced (6, 1));
    }

    int toggleIndex = 0;
    for (auto& toggle : controlToggles)
    {
        toggle->setBounds (controlArea.getX() + 14,
                           controlArea.getBottom() - 34 - toggleIndex * 32,
                           220, 28);
        ++toggleIndex;
    }

    // LFO panel layout.
    auto lfo = lfoPanelBounds.reduced (12, 10);
    lfo.removeFromTop (22);
    auto left = lfo.removeFromLeft (330);
    auto toggles = left.removeFromLeft (82);
    lfoOn.setBounds (toggles.removeFromTop (28));
    lfoSync.setBounds (toggles.removeFromTop (28));
    toggles.removeFromTop (4);
    lfoDivision.setBounds (toggles.removeFromTop (27));

    auto rateCell = left.removeFromLeft (112).reduced (4, 0);
    lfoRateLabel.setBounds (rateCell.removeFromTop (18));
    lfoRate.setBounds (rateCell);
    auto depthCell = left.removeFromLeft (112).reduced (4, 0);
    lfoDepthLabel.setBounds (depthCell.removeFromTop (18));
    lfoDepth.setBounds (depthCell);

    lfo.removeFromLeft (8);
    auto menus = lfo.removeFromLeft (130);
    lfoShapeLabel.setBounds (menus.removeFromTop (16));
    lfoShape.setBounds (menus.removeFromTop (27));
    menus.removeFromTop (6);
    lfoTargetLabel.setBounds (menus.removeFromTop (16));
    lfoTarget.setBounds (menus.removeFromTop (27));

    lfo.removeFromLeft (8);
    meterBounds = lfo.removeFromRight (12).reduced (0, 3);
    lfo.removeFromRight (8);
    waveformBounds = lfo.reduced (2, 8);
}

void NeonRackEditor::refreshRack()
{
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        const auto module = processor.getSlot (i);
        slotMenus[(size_t) i].setSelectedId ((int) module + 1, juce::dontSendNotification);

        bool on = false;
        if (module != NeonRackProcessor::Module::empty)
        {
            const auto enabledId = NeonRackProcessor::enabledParameter (module);
            if (! enabledId.isEmpty())
                if (auto* raw = processor.state.getRawParameterValue (enabledId))
                    on = raw->load() >= 0.5f;
        }

        const bool occupied = module != NeonRackProcessor::Module::empty;
        bypassButtons[(size_t) i].setToggleState (occupied && ! on, juce::dontSendNotification);
        bypassButtons[(size_t) i].setEnabled (occupied);
        editButtons[(size_t) i].setEnabled (occupied);
        removeButtons[(size_t) i].setEnabled (occupied);
        dragButtons[(size_t) i].setEnabled (occupied);
        collapseButtons[(size_t) i].setEnabled (occupied);
        upButtons[(size_t) i].setEnabled (occupied && i > 0);
        downButtons[(size_t) i].setEnabled (occupied && i + 1 < NeonRackProcessor::numRackSlots);
        collapseButtons[(size_t) i].setButtonText (collapsed[(size_t) i] ? "+" : "-");
    }

    resized();
    repaint();
}

void NeonRackEditor::selectSlot (int slot)
{
    selectedSlot = juce::jlimit (0, NeonRackProcessor::numRackSlots - 1, slot);
    selectedModule = processor.getSlot (selectedSlot);
    selectedTitle.setText (selectedModule == NeonRackProcessor::Module::empty
                               ? "EMPTY SLOT - ADD AN EFFECT"
                               : NeonRackProcessor::moduleName (selectedModule),
                           juce::dontSendNotification);
    rebuildControls();
    repaint();
}

void NeonRackEditor::rebuildControls()
{
    controlAttachments.clear();
    controlToggleAttachments.clear();
    controlKnobs.clear();
    controlLabels.clear();
    controlToggles.clear();
    moduleSyncAttachment.reset();
    moduleDivisionAttachment.reset();

    moduleSyncToggle.setVisible (false);
    moduleDivision.setVisible (false);
    moduleSyncLabel.setVisible (false);

    if (selectedModule == NeonRackProcessor::Module::empty)
    {
        resized();
        return;
    }

    juce::String syncId;
    juce::String divisionId;
    if (selectedModule == NeonRackProcessor::Module::chorus)
    {
        syncId = "chorusSync";
        divisionId = "chorusDivision";
    }
    else if (selectedModule == NeonRackProcessor::Module::prismDelay)
    {
        syncId = "delaySync";
        divisionId = "delayDivision";
    }
    else if (selectedModule == NeonRackProcessor::Module::photonPhaser)
    {
        syncId = "phaserSync";
        divisionId = "phaserDivision";
    }

    if (syncId.isNotEmpty())
    {
        moduleSyncToggle.setVisible (true);
        moduleDivision.setVisible (true);
        moduleSyncLabel.setVisible (true);
        moduleSyncAttachment = std::make_unique<ButtonAttachment> (processor.state, syncId, moduleSyncToggle);
        moduleDivisionAttachment = std::make_unique<ComboBoxAttachment> (processor.state, divisionId, moduleDivision);
    }

    const auto accent = NeonRackProcessor::moduleColour (selectedModule);
    for (const auto& id : NeonRackProcessor::moduleParameterIds (selectedModule))
    {
        if (id.endsWith ("Sync") || id.endsWith ("Division"))
            continue;

        if (id == "reverbFreeze")
        {
            auto toggle = std::make_unique<juce::ToggleButton> ("FREEZE / INFINITE HOLD");
            toggle->setColour (juce::ToggleButton::textColourId, white);
            toggle->setColour (juce::ToggleButton::tickColourId, accent);
            addAndMakeVisible (*toggle);
            auto attachment = std::make_unique<ButtonAttachment> (processor.state, id, *toggle);
            controlToggles.push_back (std::move (toggle));
            controlToggleAttachments.push_back (std::move (attachment));
            continue;
        }

        if (processor.state.getParameter (id) == nullptr)
            continue;

        auto knob = std::make_unique<juce::Slider>();
        auto label = std::make_unique<juce::Label>();
        configureKnob (*knob, accent);
        configureLabel (*label, NeonRackProcessor::parameterLabel (id));
        auto attachment = std::make_unique<SliderAttachment> (processor.state, id, *knob);
        controlKnobs.push_back (std::move (knob));
        controlLabels.push_back (std::move (label));
        controlAttachments.push_back (std::move (attachment));
    }

    resized();
}

void NeonRackEditor::moveCollapsedState (int from, int to)
{
    if (! juce::isPositiveAndBelow (from, NeonRackProcessor::numRackSlots)
        || ! juce::isPositiveAndBelow (to, NeonRackProcessor::numRackSlots)
        || from == to)
        return;

    const bool moving = collapsed[(size_t) from];
    if (from < to)
        for (int i = from; i < to; ++i)
            collapsed[(size_t) i] = collapsed[(size_t) i + 1];
    else
        for (int i = from; i > to; --i)
            collapsed[(size_t) i] = collapsed[(size_t) i - 1];
    collapsed[(size_t) to] = moving;
}

int NeonRackEditor::slotAtY (int y) const
{
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
        if (slotCardBounds[(size_t) i].contains (slotCardBounds[(size_t) i].getCentreX(), y))
            return i;

    int best = 0;
    int bestDistance = std::numeric_limits<int>::max();
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        const int distance = std::abs (slotCardBounds[(size_t) i].getCentreY() - y);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

bool NeonRackEditor::isInterestedInDragSource (const SourceDetails& details)
{
    const int from = details.description.toString().getIntValue();
    return juce::isPositiveAndBelow (from, NeonRackProcessor::numRackSlots)
        && processor.getSlot (from) != NeonRackProcessor::Module::empty;
}

void NeonRackEditor::itemDragMove (const SourceDetails& details)
{
    dragTargetSlot = slotAtY (details.localPosition.y);
    repaint();
}

void NeonRackEditor::itemDragExit (const SourceDetails& details)
{
    juce::ignoreUnused (details);
    dragTargetSlot = -1;
    repaint();
}

void NeonRackEditor::itemDropped (const SourceDetails& details)
{
    const int from = details.description.toString().getIntValue();
    const int to = slotAtY (details.localPosition.y);
    dragTargetSlot = -1;

    if (! juce::isPositiveAndBelow (from, NeonRackProcessor::numRackSlots)
        || ! juce::isPositiveAndBelow (to, NeonRackProcessor::numRackSlots))
        return;

    processor.moveSlot (from, to);
    moveCollapsedState (from, to);
    refreshRack();
    selectSlot (to);
}

void NeonRackEditor::timerCallback()
{
    const bool globalSync = lfoSync.getToggleState();
    lfoRate.setEnabled (! globalSync);
    lfoDivision.setEnabled (globalSync);

    if (moduleSyncToggle.isVisible())
        moduleDivision.setEnabled (moduleSyncToggle.getToggleState());

    repaint();
}
