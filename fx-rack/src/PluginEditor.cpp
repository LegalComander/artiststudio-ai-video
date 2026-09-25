#include "PluginEditor.h"

namespace
{
const auto bg = juce::Colour (0xff02060c);
const auto panel = juce::Colour (0xff07121f);
const auto line = juce::Colour (0xff14334a);
const auto cyan = juce::Colour (0xff43e9ff);
const auto muted = juce::Colour (0xff84a5b8);
}

NeonRackLookAndFeel::NeonRackLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff07111d));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff1b3c53));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff07111d));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffdff8ff));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff08131f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, juce::Colours::white);
}

void NeonRackLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float pos, float start, float end, juce::Slider& slider)
{
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (10.0f);
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto centre = area.getCentre();
    const auto angle = start + pos * (end - start);
    auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    if (accent.isTransparent()) accent = cyan;

    g.setColour (juce::Colour (0xff0a1725));
    g.fillEllipse (area);
    g.setColour (juce::Colour (0xff1a3448));
    g.drawEllipse (area, 2.0f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff16364b));
    g.strokePath (track, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path active;
    active.addCentredArc (centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, angle, true);
    g.setColour (accent.withAlpha (0.96f));
    g.strokePath (active, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto p = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * (radius - 14.0f);
    g.setColour (juce::Colours::white);
    g.drawLine (centre.x, centre.y, p.x, p.y, 2.3f);
}

void NeonRackLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                const juce::Colour&, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff07111d));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour ((hover || down ? cyan : line).withAlpha (hover ? 0.78f : 0.58f));
    g.drawRoundedRectangle (r, 8.0f, hover ? 1.6f : 1.0f);
}

void NeonRackLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (juce::Colours::white.withAlpha (b.isEnabled() ? 0.92f : 0.28f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (5, 1), juce::Justification::centred, 1);
}

void NeonRackLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                        int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 9.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 9.0f, 1.0f);
    juce::Path p;
    p.addTriangle ((float) width - 18.0f, (float) height * 0.42f,
                   (float) width - 10.0f, (float) height * 0.42f,
                   (float) width - 14.0f, (float) height * 0.60f);
    g.setColour (cyan);
    g.fillPath (p);
}

void NeonRackLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (1, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
}

NeonRackEditor::NeonRackEditor (NeonRackProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (900, 620, 1340, 920);
    setSize (1120, 760);

    brand.setText ("ARTISTSTUDIO / FX LAB", juce::dontSendNotification);
    brand.setColour (juce::Label::textColourId, muted);
    brand.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (brand);

    title.setText ("NEONRACK FX", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    title.setFont (juce::Font (juce::FontOptions (34.0f, juce::Font::bold)));
    addAndMakeVisible (title);

    subtitle.setText ("MODULAR MULTI-FX / DRAG TO REORDER", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, cyan);
    subtitle.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    addAndMakeVisible (subtitle);

    styleSmallLabel (presetLabel, "FACTORY PRESET");
    addAndMakeVisible (presetMenu);
    presetMenu.addItemList (NeonRackProcessor::factoryPresetNames(), 1);
    presetMenu.setSelectedId (1, juce::dontSendNotification);
    presetMenu.onChange = [this]
    {
        const int index = presetMenu.getSelectedItemIndex();
        if (index >= 0)
        {
            processor.applyFactoryPreset (index);
            selectedSlot = 0;
            refreshRack();
            selectSlot (0);
        }
    };

    selectedTitle.setColour (juce::Label::textColourId, juce::Colours::white);
    selectedTitle.setFont (juce::Font (juce::FontOptions (23.0f, juce::Font::bold)));
    addAndMakeVisible (selectedTitle);

    hint.setText ("Select a module to edit. Drag the ≡ handle to move it anywhere in the chain.", juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, muted);
    hint.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (hint);

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto& menu = slotMenus[(size_t) i];
        menu.addItem ("+ ADD FX", 1);
        for (int m = (int) NeonRackProcessor::Module::filter; m <= (int) NeonRackProcessor::Module::pulseComp; ++m)
            menu.addItem (NeonRackProcessor::moduleName ((NeonRackProcessor::Module) m), m + 1);
        addAndMakeVisible (menu);

        menu.onChange = [this, i]
        {
            processor.setSlot (i, (NeonRackProcessor::Module) (slotMenus[(size_t) i].getSelectedId() - 1));
            selectSlot (i);
            refreshRack();
        };

        dragHandles[(size_t) i] = std::make_unique<RackDragHandle>();
        auto* handle = dragHandles[(size_t) i].get();
        addAndMakeVisible (*handle);
        handle->onDown = [this, i] (const juce::MouseEvent&) { beginRackDrag (i); };
        handle->onDrag = [this] (const juce::MouseEvent& e)
        {
            updateRackDrag (e.getEventRelativeTo (this).getPosition());
        };
        handle->onUp = [this] (const juce::MouseEvent&) { endRackDrag(); };

        editButtons[(size_t) i].setButtonText ("EDIT");
        upButtons[(size_t) i].setButtonText ("▲");
        downButtons[(size_t) i].setButtonText ("▼");
        removeButtons[(size_t) i].setButtonText ("×");
        bypassButtons[(size_t) i].setButtonText ("BYPASS");

        for (auto* b : { &editButtons[(size_t) i], &upButtons[(size_t) i], &downButtons[(size_t) i], &removeButtons[(size_t) i] })
            addAndMakeVisible (*b);
        addAndMakeVisible (bypassButtons[(size_t) i]);

        editButtons[(size_t) i].onClick = [this, i] { selectSlot (i); };
        upButtons[(size_t) i].onClick = [this, i]
        {
            if (i > 0) { processor.moveSlot (i, i - 1); selectSlot (i - 1); refreshRack(); }
        };
        downButtons[(size_t) i].onClick = [this, i]
        {
            if (i + 1 < NeonRackProcessor::numRackSlots) { processor.moveSlot (i, i + 1); selectSlot (i + 1); refreshRack(); }
        };
        removeButtons[(size_t) i].onClick = [this, i]
        {
            processor.removeSlot (i); selectSlot (i); refreshRack();
        };
        bypassButtons[(size_t) i].onClick = [this, i]
        {
            const auto module = processor.getSlot (i);
            const auto id = NeonRackProcessor::enabledParameter (module);
            if (auto* param = processor.state.getParameter (id))
                param->setValueNotifyingHost (bypassButtons[(size_t) i].getToggleState() ? 0.0f : 1.0f);
        };
    }

    styleKnob (globalMix, cyan);
    styleSmallLabel (globalMixLabel, "GLOBAL MIX");
    styleKnob (output, juce::Colour (0xff7ef0bd));
    styleSmallLabel (outputLabel, "OUTPUT");
    globalMixAttachment = std::make_unique<SliderAttachment> (processor.state, "globalMix", globalMix);
    outputAttachment = std::make_unique<SliderAttachment> (processor.state, "output", output);

    const std::array<juce::String, 4> macroIds { "macroDrive", "macroSpace", "macroMotion", "macroTone" };
    const std::array<juce::String, 4> macroNames { "DRIVE", "SPACE", "MOTION", "TONE" };
    const std::array<juce::Colour, 4> macroColours {
        juce::Colour (0xffff5d94), juce::Colour (0xff6d8cff), juce::Colour (0xffc47cff), juce::Colour (0xff5ef0b1)
    };

    for (int i = 0; i < 4; ++i)
    {
        styleKnob (macroKnobs[(size_t) i], macroColours[(size_t) i]);
        styleSmallLabel (macroLabels[(size_t) i], macroNames[(size_t) i]);
        macroAttachments[(size_t) i] = std::make_unique<SliderAttachment> (processor.state, macroIds[(size_t) i], macroKnobs[(size_t) i]);
    }

    refreshRack();
    selectSlot (0);
    startTimerHz (10);
}

NeonRackEditor::~NeonRackEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void NeonRackEditor::styleKnob (juce::Slider& slider, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible (slider);
}

void NeonRackEditor::styleSmallLabel (juce::Label& label, const juce::String& textValue)
{
    label.setText (textValue, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, muted);
    label.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    addAndMakeVisible (label);
}

void NeonRackEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    auto full = getLocalBounds().toFloat();
    juce::ColourGradient glow (juce::Colour (0xff0b365a), 0.0f, 0.0f, bg, full.getRight(), full.getBottom(), false);
    glow.addColour (0.45, juce::Colour (0xff06111c));
    g.setGradientFill (glow);
    g.fillRect (full);

    auto content = getLocalBounds().reduced (18);
    auto header = content.removeFromTop (132).toFloat();
    g.setColour (juce::Colour (0xff06111d));
    g.fillRoundedRectangle (header, 20.0f);
    g.setColour (cyan.withAlpha (0.25f));
    g.drawRoundedRectangle (header, 20.0f, 1.0f);

    content.removeFromTop (14);
    auto rackArea = content.removeFromLeft (350).toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (rackArea, 18.0f);
    g.setColour (line.withAlpha (0.85f));
    g.drawRoundedRectangle (rackArea, 18.0f, 1.0f);

    content.removeFromLeft (14);
    auto controlArea = content.toFloat();
    g.setColour (juce::Colour (0xff06101b));
    g.fillRoundedRectangle (controlArea, 18.0f);
    auto accent = NeonRackProcessor::moduleColour (selectedModule);
    g.setColour (accent.withAlpha (0.25f));
    g.drawRoundedRectangle (controlArea, 18.0f, 1.0f);

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto r = slotBounds[(size_t) i].toFloat();
        auto m = processor.getSlot (i);
        auto c = NeonRackProcessor::moduleColour (m);
        const bool selected = i == selectedSlot;
        const bool target = i == dragTarget && dragSource >= 0;

        g.setColour ((target ? cyan : selected ? c : line).withAlpha (target ? 0.25f : selected ? 0.15f : 0.08f));
        g.fillRoundedRectangle (r, 11.0f);
        g.setColour ((target ? cyan : selected ? c : line).withAlpha (target ? 0.95f : 0.62f));
        g.drawRoundedRectangle (r, 11.0f, target ? 2.2f : selected ? 1.4f : 1.0f);
    }
}

void NeonRackEditor::resized()
{
    auto content = getLocalBounds().reduced (26);
    auto header = content.removeFromTop (114);

    auto leftHeader = header.removeFromLeft (350);
    brand.setBounds (leftHeader.removeFromTop (17));
    title.setBounds (leftHeader.removeFromTop (41));
    subtitle.setBounds (leftHeader.removeFromTop (20));
    presetLabel.setBounds (leftHeader.removeFromTop (16));
    presetMenu.setBounds (leftHeader.removeFromTop (28).withTrimmedRight (40));

    auto macroArea = header.removeFromLeft (520);
    const int macroW = macroArea.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto cell = macroArea.removeFromLeft (macroW);
        macroLabels[(size_t) i].setBounds (cell.removeFromTop (18));
        macroKnobs[(size_t) i].setBounds (cell.reduced (8, 0));
    }

    auto globals = header;
    auto gm = globals.removeFromLeft (globals.getWidth() / 2);
    globalMixLabel.setBounds (gm.removeFromTop (18));
    globalMix.setBounds (gm.reduced (3, 0));
    outputLabel.setBounds (globals.removeFromTop (18));
    output.setBounds (globals.reduced (3, 0));

    content.removeFromTop (28);
    auto rackArea = content.removeFromLeft (330).reduced (10);
    content.removeFromLeft (34);
    auto controlArea = content.reduced (20);

    const int slotH = juce::jmax (68, rackArea.getHeight() / NeonRackProcessor::numRackSlots);
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto row = rackArea.removeFromTop (slotH).reduced (3, 5);
        slotBounds[(size_t) i] = row.expanded (4, 3);
        dragHandles[(size_t) i]->setBounds (row.removeFromLeft (28));
        row.removeFromLeft (4);
        slotMenus[(size_t) i].setBounds (row.removeFromTop (30));
        auto buttons = row.removeFromTop (27);
        editButtons[(size_t) i].setBounds (buttons.removeFromLeft (48));
        buttons.removeFromLeft (3);
        bypassButtons[(size_t) i].setBounds (buttons.removeFromLeft (68));
        buttons.removeFromLeft (3);
        upButtons[(size_t) i].setBounds (buttons.removeFromLeft (31));
        buttons.removeFromLeft (2);
        downButtons[(size_t) i].setBounds (buttons.removeFromLeft (31));
        buttons.removeFromLeft (2);
        removeButtons[(size_t) i].setBounds (buttons.removeFromLeft (31));
    }

    auto top = controlArea.removeFromTop (58);
    selectedTitle.setBounds (top.removeFromTop (32));
    hint.setBounds (top.removeFromTop (24));
    controlArea.removeFromTop (16);

    const int columns = 3;
    const int rows = 2;
    const int cellW = controlArea.getWidth() / columns;
    const int cellH = juce::jmax (130, controlArea.getHeight() / rows);

    for (size_t i = 0; i < controlKnobs.size(); ++i)
    {
        const int index = (int) i;
        const int col = index % columns;
        const int row = index / columns;
        if (row >= rows) break;
        auto cell = juce::Rectangle<int> (controlArea.getX() + col * cellW,
                                          controlArea.getY() + row * cellH,
                                          cellW, cellH).reduced (10);
        controlLabels[i]->setBounds (cell.removeFromTop (22));
        controlKnobs[i]->setBounds (cell.reduced (4, 0));
    }

    int toggleIndex = 0;
    for (auto& toggle : controlToggles)
    {
        auto r = juce::Rectangle<int> (controlArea.getX() + 10,
                                       controlArea.getBottom() - 38 - toggleIndex * 34,
                                       160, 30);
        toggle->setBounds (r);
        ++toggleIndex;
    }
}

void NeonRackEditor::timerCallback()
{
    refreshRack();
}

void NeonRackEditor::refreshRack()
{
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        const auto module = processor.getSlot (i);
        slotMenus[(size_t) i].setSelectedId ((int) module + 1, juce::dontSendNotification);
        const auto enabledId = NeonRackProcessor::enabledParameter (module);
        bool on = true;
        if (auto* raw = processor.state.getRawParameterValue (enabledId)) on = raw->load() >= 0.5f;
        bypassButtons[(size_t) i].setToggleState (module != NeonRackProcessor::Module::empty && ! on, juce::dontSendNotification);
        bypassButtons[(size_t) i].setEnabled (module != NeonRackProcessor::Module::empty);
        editButtons[(size_t) i].setEnabled (module != NeonRackProcessor::Module::empty);
        removeButtons[(size_t) i].setEnabled (module != NeonRackProcessor::Module::empty);
        upButtons[(size_t) i].setEnabled (i > 0 && module != NeonRackProcessor::Module::empty);
        downButtons[(size_t) i].setEnabled (i + 1 < NeonRackProcessor::numRackSlots && module != NeonRackProcessor::Module::empty);
        dragHandles[(size_t) i]->setEnabled (module != NeonRackProcessor::Module::empty);
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
            addAndMakeVisible (*toggle);
            auto attachment = std::make_unique<ButtonAttachment> (processor.state, id, *toggle);
            controlToggles.push_back (std::move (toggle));
            controlToggleAttachments.push_back (std::move (attachment));
            continue;
        }

        auto knob = std::make_unique<juce::Slider>();
        auto label = std::make_unique<juce::Label>();
        styleKnob (*knob, accent);
        styleSmallLabel (*label, NeonRackProcessor::parameterLabel (id));
        auto attachment = std::make_unique<SliderAttachment> (processor.state, id, *knob);
        controlKnobs.push_back (std::move (knob));
        controlLabels.push_back (std::move (label));
        controlAttachments.push_back (std::move (attachment));
    }

    resized();
}

void NeonRackEditor::beginRackDrag (int slot)
{
    if (processor.getSlot (slot) == NeonRackProcessor::Module::empty) return;
    dragSource = slot;
    dragTarget = slot;
    selectedSlot = slot;
    repaint();
}

void NeonRackEditor::updateRackDrag (juce::Point<int> editorPoint)
{
    if (dragSource < 0) return;
    const int target = slotAtPoint (editorPoint);
    if (target >= 0 && target != dragTarget)
    {
        dragTarget = target;
        repaint();
    }
}

void NeonRackEditor::endRackDrag()
{
    if (dragSource >= 0 && dragTarget >= 0 && dragSource != dragTarget)
    {
        processor.moveSlot (dragSource, dragTarget);
        selectedSlot = dragTarget;
    }
    dragSource = -1;
    dragTarget = -1;
    refreshRack();
    selectSlot (selectedSlot);
}

int NeonRackEditor::slotAtPoint (juce::Point<int> point) const
{
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
        if (slotBounds[(size_t) i].contains (point))
            return i;
    return -1;
}
