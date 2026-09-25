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
}

NeonRackEditor::NeonRackEditor (NeonRackProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Keep the editor fixed while we preserve the host-safe behaviour that
    // passed pluginval and Ableton testing. The visual rack can still feel
    // modular without relying on host resize callbacks.
    setSize (1120, 760);
    setOpaque (true);

    configureLabel (brand, "ARTISTSTUDIO / FX LAB", juce::Justification::centredLeft);
    brand.setColour (juce::Label::textColourId, muted);
    brand.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));

    configureLabel (title, "NEONRACK FX", juce::Justification::centredLeft);
    title.setColour (juce::Label::textColourId, white);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));

    configureLabel (subtitle, "MODULAR MULTI-FX / BUILD YOUR OWN CHAIN", juce::Justification::centredLeft);
    subtitle.setColour (juce::Label::textColourId, cyan);
    subtitle.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));

    configureLabel (presetLabel, "FACTORY PRESET", juce::Justification::centredLeft);
    presetMenu.setColour (juce::ComboBox::backgroundColourId, panel2);
    presetMenu.setColour (juce::ComboBox::textColourId, white);
    presetMenu.setColour (juce::ComboBox::outlineColourId, line);
    presetMenu.setColour (juce::ComboBox::arrowColourId, cyan);
    presetMenu.addItemList (NeonRackProcessor::factoryPresetNames(), 1);
    presetMenu.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (presetMenu);
    presetMenu.onChange = [this]
    {
        const auto index = presetMenu.getSelectedItemIndex();
        if (index < 0)
            return;

        processor.applyFactoryPreset (index);
        refreshRack();
        selectSlot (0);
    };

    configureLabel (selectedTitle, "", juce::Justification::centredLeft);
    selectedTitle.setColour (juce::Label::textColourId, white);
    selectedTitle.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));

    configureLabel (hint,
                    "MODULE CONTROL  |  Select an FX card, edit it here, and use UP / DN to change the signal flow.",
                    juce::Justification::centredLeft);
    hint.setColour (juce::Label::textColourId, muted);
    hint.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto& menu = slotMenus[(size_t) i];
        menu.setColour (juce::ComboBox::backgroundColourId, panel3);
        menu.setColour (juce::ComboBox::textColourId, white);
        menu.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
        menu.setColour (juce::ComboBox::arrowColourId, cyan);
        menu.addItem ("+ ADD FX", 1);
        for (int m = (int) NeonRackProcessor::Module::filter;
             m <= (int) NeonRackProcessor::Module::pulseComp; ++m)
            menu.addItem (NeonRackProcessor::moduleName ((NeonRackProcessor::Module) m), m + 1);
        addAndMakeVisible (menu);

        menu.onChange = [this, i]
        {
            const auto selected = slotMenus[(size_t) i].getSelectedId();
            if (selected <= 0)
                return;

            processor.setSlot (i, (NeonRackProcessor::Module) (selected - 1));
            refreshRack();
            selectSlot (i);
        };

        editButtons[(size_t) i].setButtonText ("EDIT");
        upButtons[(size_t) i].setButtonText ("UP");
        downButtons[(size_t) i].setButtonText ("DN");
        removeButtons[(size_t) i].setButtonText ("X");
        bypassButtons[(size_t) i].setButtonText ("BYP");

        upButtons[(size_t) i].setTooltip ("Move effect up");
        downButtons[(size_t) i].setTooltip ("Move effect down");
        removeButtons[(size_t) i].setTooltip ("Remove effect from rack");
        bypassButtons[(size_t) i].setTooltip ("Bypass this effect without removing it");

        configureSmallButton (editButtons[(size_t) i]);
        configureSmallButton (upButtons[(size_t) i]);
        configureSmallButton (downButtons[(size_t) i]);
        configureSmallButton (removeButtons[(size_t) i]);

        bypassButtons[(size_t) i].setColour (juce::ToggleButton::textColourId, white);
        bypassButtons[(size_t) i].setColour (juce::ToggleButton::tickColourId, cyan);
        bypassButtons[(size_t) i].setColour (juce::ToggleButton::tickDisabledColourId, muted);
        addAndMakeVisible (bypassButtons[(size_t) i]);

        editButtons[(size_t) i].onClick = [this, i] { selectSlot (i); };
        upButtons[(size_t) i].onClick = [this, i]
        {
            if (i <= 0)
                return;
            processor.moveSlot (i, i - 1);
            refreshRack();
            selectSlot (i - 1);
        };
        downButtons[(size_t) i].onClick = [this, i]
        {
            if (i + 1 >= NeonRackProcessor::numRackSlots)
                return;
            processor.moveSlot (i, i + 1);
            refreshRack();
            selectSlot (i + 1);
        };
        removeButtons[(size_t) i].onClick = [this, i]
        {
            processor.removeSlot (i);
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

    refreshRack();
    selectSlot (0);
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

void NeonRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    juce::ColourGradient background (juce::Colour (0xff0a2943), 0.0f, 0.0f,
                                     bg, (float) getWidth(), (float) getHeight(), false);
    background.addColour (0.32, juce::Colour (0xff061523));
    background.addColour (0.70, juce::Colour (0xff040a11));
    g.setGradientFill (background);
    g.fillRect (getLocalBounds());

    // Header / macro strip.
    auto content = getLocalBounds().reduced (16);
    auto header = content.removeFromTop (138).toFloat();
    g.setColour (juce::Colour (0xff04101b));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (cyan.withAlpha (0.22f));
    g.drawRoundedRectangle (header, 18.0f, 1.0f);

    g.setColour (cyan.withAlpha (0.65f));
    g.fillRoundedRectangle (header.removeFromBottom (3.0f), 1.5f);

    // Small cards behind macros and global controls.
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

    // Main rack and editor panels.
    content.removeFromTop (12);
    auto rackPanel = content.removeFromLeft (370).toFloat();
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

    // Rack heading.
    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("FX CHAIN", rackPanel.toNearestInt().withTrimmedLeft (18).removeFromTop (34),
                juce::Justification::centredLeft, false);

    // Individual FX cards. The visible card order is the true DSP order.
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto card = slotMenus[(size_t) i].getBounds()
                        .getUnion (removeButtons[(size_t) i].getBounds())
                        .expanded (10, 7)
                        .toFloat();

        const auto module = processor.getSlot (i);
        const auto occupied = module != NeonRackProcessor::Module::empty;
        const auto accent = occupied ? NeonRackProcessor::moduleColour (module) : line;

        g.setColour ((i == selectedSlot ? panel3 : panel2).withAlpha (occupied ? 0.96f : 0.62f));
        g.fillRoundedRectangle (card, 11.0f);

        g.setColour ((i == selectedSlot ? accent : line).withAlpha (i == selectedSlot ? 0.88f : 0.66f));
        g.drawRoundedRectangle (card, 11.0f, i == selectedSlot ? 1.8f : 1.0f);

        auto rail = card.withWidth (4.0f).reduced (0.0f, 8.0f);
        g.setColour (accent.withAlpha (occupied ? 0.95f : 0.32f));
        g.fillRoundedRectangle (rail, 2.0f);

        auto badge = juce::Rectangle<float> (card.getX() + 8.0f, card.getY() + 9.0f, 23.0f, 23.0f);
        g.setColour (accent.withAlpha (occupied ? 0.18f : 0.08f));
        g.fillEllipse (badge);
        g.setColour (occupied ? accent : muted.withAlpha (0.55f));
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (juce::String (i + 1), badge.toNearestInt(), juce::Justification::centred, false);
    }

    // Module parameter cards.
    for (size_t i = 0; i < controlKnobs.size(); ++i)
    {
        auto card = controlLabels[i]->getBounds().getUnion (controlKnobs[i]->getBounds()).expanded (7, 5).toFloat();
        g.setColour (panel2.withAlpha (0.72f));
        g.fillRoundedRectangle (card, 13.0f);
        g.setColour (selectedAccent.withAlpha (0.18f));
        g.drawRoundedRectangle (card, 13.0f, 1.0f);
    }
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
    auto rackArea = content.removeFromLeft (342).reduced (10, 4);
    rackArea.removeFromTop (22);
    content.removeFromLeft (40);
    auto controlArea = content.reduced (18, 4);

    const int slotH = juce::jmax (76, rackArea.getHeight() / NeonRackProcessor::numRackSlots);
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto row = rackArea.removeFromTop (slotH).reduced (10, 7);
        row.removeFromLeft (28);
        slotMenus[(size_t) i].setBounds (row.removeFromTop (28));
        row.removeFromTop (4);
        auto buttons = row.removeFromTop (25);
        editButtons[(size_t) i].setBounds (buttons.removeFromLeft (48));
        buttons.removeFromLeft (3);
        bypassButtons[(size_t) i].setBounds (buttons.removeFromLeft (64));
        buttons.removeFromLeft (3);
        upButtons[(size_t) i].setBounds (buttons.removeFromLeft (40));
        buttons.removeFromLeft (2);
        downButtons[(size_t) i].setBounds (buttons.removeFromLeft (40));
        buttons.removeFromLeft (2);
        removeButtons[(size_t) i].setBounds (buttons.removeFromLeft (36));
    }

    auto top = controlArea.removeFromTop (72);
    selectedTitle.setBounds (top.removeFromTop (38));
    hint.setBounds (top.removeFromTop (25));
    controlArea.removeFromTop (12);

    const int columns = 3;
    const int rows = 2;
    const int cellW = juce::jmax (1, controlArea.getWidth() / columns);
    const int cellH = juce::jmax (145, controlArea.getHeight() / rows);

    for (size_t i = 0; i < controlKnobs.size(); ++i)
    {
        const auto index = (int) i;
        const auto col = index % columns;
        const auto row = index / columns;
        if (row >= rows)
            break;

        auto cell = juce::Rectangle<int> (controlArea.getX() + col * cellW,
                                          controlArea.getY() + row * cellH,
                                          cellW, cellH).reduced (14, 10);
        controlLabels[i]->setBounds (cell.removeFromTop (24));
        controlKnobs[i]->setBounds (cell.reduced (8, 2));
    }

    int toggleIndex = 0;
    for (auto& toggle : controlToggles)
    {
        toggle->setBounds (controlArea.getX() + 14,
                           controlArea.getBottom() - 36 - toggleIndex * 34,
                           220, 30);
        ++toggleIndex;
    }
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
        upButtons[(size_t) i].setEnabled (occupied && i > 0);
        downButtons[(size_t) i].setEnabled (occupied && i + 1 < NeonRackProcessor::numRackSlots);
    }

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
    // Attachments must be destroyed before their UI controls.
    controlAttachments.clear();
    controlToggleAttachments.clear();
    controlKnobs.clear();
    controlLabels.clear();
    controlToggles.clear();

    if (selectedModule == NeonRackProcessor::Module::empty)
    {
        resized();
        return;
    }

    const auto accent = NeonRackProcessor::moduleColour (selectedModule);
    for (const auto& id : NeonRackProcessor::moduleParameterIds (selectedModule))
    {
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
