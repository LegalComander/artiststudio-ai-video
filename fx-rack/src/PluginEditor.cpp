#include "PluginEditor.h"

namespace
{
const auto bg = juce::Colour (0xff02060c);
const auto panel = juce::Colour (0xff07121f);
const auto panel2 = juce::Colour (0xff091827);
const auto line = juce::Colour (0xff17384e);
const auto cyan = juce::Colour (0xff43e9ff);
const auto muted = juce::Colour (0xff86a8bb);
const auto white = juce::Colour (0xffeefbff);
}

NeonRackEditor::NeonRackEditor (NeonRackProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // A fixed editor avoids host resize/peer edge-cases while the plugin is
    // being instantiated or destroyed. We can re-introduce free resizing once
    // the rack has passed host validation in Live/pluginval.
    setSize (1080, 720);
    setOpaque (true);

    configureLabel (brand, "ARTISTSTUDIO / FX LAB", juce::Justification::centredLeft);
    brand.setColour (juce::Label::textColourId, muted);
    brand.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));

    configureLabel (title, "NEONRACK FX", juce::Justification::centredLeft);
    title.setColour (juce::Label::textColourId, white);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));

    configureLabel (subtitle, "MODULAR MULTI-FX / HOST-SAFE BETA", juce::Justification::centredLeft);
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
    selectedTitle.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));

    configureLabel (hint,
                    "Select a module to edit. Use UP / DN to reorder; BYPASS keeps it in the chain without processing.",
                    juce::Justification::centredLeft);
    hint.setColour (juce::Label::textColourId, muted);
    hint.setFont (juce::Font (juce::FontOptions (11.5f)));

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto& menu = slotMenus[(size_t) i];
        menu.setColour (juce::ComboBox::backgroundColourId, panel2);
        menu.setColour (juce::ComboBox::textColourId, white);
        menu.setColour (juce::ComboBox::outlineColourId, line);
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
        bypassButtons[(size_t) i].setButtonText ("BYPASS");

        upButtons[(size_t) i].setTooltip ("Move effect up");
        downButtons[(size_t) i].setTooltip ("Move effect down");
        removeButtons[(size_t) i].setTooltip ("Remove effect from rack");

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
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, line);
    slider.setColour (juce::Slider::thumbColourId, white);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, panel2);
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
    button.setColour (juce::TextButton::buttonColourId, panel2);
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff12344b));
    button.setColour (juce::TextButton::textColourOffId, white);
    button.setColour (juce::TextButton::textColourOnId, cyan);
    addAndMakeVisible (button);
}

void NeonRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    juce::ColourGradient gradient (juce::Colour (0xff0a2d49), 0.0f, 0.0f,
                                   bg, (float) getWidth(), (float) getHeight(), false);
    gradient.addColour (0.42, juce::Colour (0xff06101b));
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());

    auto content = getLocalBounds().reduced (18);
    auto header = content.removeFromTop (122).toFloat();
    g.setColour (juce::Colour (0xff06111d));
    g.fillRoundedRectangle (header, 18.0f);
    g.setColour (cyan.withAlpha (0.25f));
    g.drawRoundedRectangle (header, 18.0f, 1.0f);

    content.removeFromTop (14);
    auto rack = content.removeFromLeft (338).toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (rack, 18.0f);
    g.setColour (line);
    g.drawRoundedRectangle (rack, 18.0f, 1.0f);

    content.removeFromLeft (14);
    auto controls = content.toFloat();
    g.setColour (juce::Colour (0xff06101b));
    g.fillRoundedRectangle (controls, 18.0f);
    g.setColour (NeonRackProcessor::moduleColour (selectedModule).withAlpha (0.35f));
    g.drawRoundedRectangle (controls, 18.0f, 1.3f);
}

void NeonRackEditor::resized()
{
    auto content = getLocalBounds().reduced (26);
    auto header = content.removeFromTop (104);

    auto brandArea = header.removeFromLeft (330);
    brand.setBounds (brandArea.removeFromTop (17));
    title.setBounds (brandArea.removeFromTop (40));
    subtitle.setBounds (brandArea.removeFromTop (18));
    presetLabel.setBounds (brandArea.removeFromTop (15));
    presetMenu.setBounds (brandArea.removeFromTop (28).withTrimmedRight (28));

    auto macroArea = header.removeFromLeft (500);
    const int macroW = juce::jmax (1, macroArea.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
    {
        auto cell = macroArea.removeFromLeft (macroW);
        macroLabels[(size_t) i].setBounds (cell.removeFromTop (18));
        macroKnobs[(size_t) i].setBounds (cell.reduced (8, 0));
    }

    auto globals = header;
    auto mixArea = globals.removeFromLeft (globals.getWidth() / 2);
    globalMixLabel.setBounds (mixArea.removeFromTop (18));
    globalMix.setBounds (mixArea.reduced (2));
    outputLabel.setBounds (globals.removeFromTop (18));
    output.setBounds (globals.reduced (2));

    content.removeFromTop (30);
    auto rackArea = content.removeFromLeft (318).reduced (10);
    content.removeFromLeft (40);
    auto controlArea = content.reduced (20);

    const int slotH = juce::jmax (67, rackArea.getHeight() / NeonRackProcessor::numRackSlots);
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto row = rackArea.removeFromTop (slotH).reduced (4, 5);
        slotMenus[(size_t) i].setBounds (row.removeFromTop (30));
        row.removeFromTop (3);
        auto buttons = row.removeFromTop (27);
        editButtons[(size_t) i].setBounds (buttons.removeFromLeft (50));
        buttons.removeFromLeft (3);
        bypassButtons[(size_t) i].setBounds (buttons.removeFromLeft (72));
        buttons.removeFromLeft (3);
        upButtons[(size_t) i].setBounds (buttons.removeFromLeft (36));
        buttons.removeFromLeft (2);
        downButtons[(size_t) i].setBounds (buttons.removeFromLeft (36));
        buttons.removeFromLeft (2);
        removeButtons[(size_t) i].setBounds (buttons.removeFromLeft (36));
    }

    auto top = controlArea.removeFromTop (62);
    selectedTitle.setBounds (top.removeFromTop (34));
    hint.setBounds (top.removeFromTop (24));
    controlArea.removeFromTop (12);

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
                                          cellW, cellH).reduced (10);
        controlLabels[i]->setBounds (cell.removeFromTop (22));
        controlKnobs[i]->setBounds (cell.reduced (5, 0));
    }

    int toggleIndex = 0;
    for (auto& toggle : controlToggles)
    {
        toggle->setBounds (controlArea.getX() + 14,
                           controlArea.getBottom() - 35 - toggleIndex * 34,
                           210, 30);
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
                               ? "EMPTY SLOT"
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
