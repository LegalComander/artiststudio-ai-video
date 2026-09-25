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
    g.setColour (accent.withAlpha (0.95f));
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
    g.setColour ((hover || down ? cyan : line).withAlpha (hover ? 0.75f : 0.55f));
    g.drawRoundedRectangle (r, 8.0f, hover ? 1.6f : 1.0f);
}

void NeonRackLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (juce::Colours::white.withAlpha (b.isEnabled() ? 0.92f : 0.28f));
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 1), juce::Justification::centred, 1);
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
    label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
}

NeonRackEditor::NeonRackEditor (NeonRackProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (760, 560, 1180, 820);
    setSize (980, 680);

    brand.setText ("ARTISTSTUDIO / FX LAB", juce::dontSendNotification);
    brand.setColour (juce::Label::textColourId, muted);
    brand.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    addAndMakeVisible (brand);

    title.setText ("NEONRACK FX", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    title.setFont (juce::Font (juce::FontOptions (36.0f, juce::Font::bold)));
    addAndMakeVisible (title);

    subtitle.setText ("BUILD YOUR OWN SIGNAL CHAIN", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, cyan);
    subtitle.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    addAndMakeVisible (subtitle);

    selectedTitle.setColour (juce::Label::textColourId, juce::Colours::white);
    selectedTitle.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    addAndMakeVisible (selectedTitle);

    hint.setText ("Choose any module, bypass it, remove it, or move it anywhere in the rack.", juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, muted);
    addAndMakeVisible (hint);

    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto& menu = slotMenus[(size_t) i];
        menu.addItem ("+ ADD FX", 1);
        menu.addItem ("NEON FILTER", 2);
        menu.addItem ("RIFT DISTORTION", 3);
        menu.addItem ("AURA SATURATOR", 4);
        menu.addItem ("CHORUS MATRIX", 5);
        menu.addItem ("PRISM DELAY", 6);
        menu.addItem ("ORBIT EQ", 7);
        addAndMakeVisible (menu);

        menu.onChange = [this, i]
        {
            processor.setSlot (i, (NeonRackProcessor::Module) (slotMenus[(size_t) i].getSelectedId() - 1));
            selectSlot (i);
            refreshRack();
        };

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
            selectSlot (i);
        };
    }

    auto setupGlobal = [this] (juce::Slider& s, juce::Label& l, juce::String text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 21);
        s.setColour (juce::Slider::rotarySliderFillColourId, cyan);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, muted);
        l.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        addAndMakeVisible (s); addAndMakeVisible (l);
    };
    setupGlobal (globalMix, globalMixLabel, "GLOBAL MIX");
    setupGlobal (output, outputLabel, "OUTPUT");
    globalMixAttachment = std::make_unique<SliderAttachment> (processor.state, "globalMix", globalMix);
    outputAttachment = std::make_unique<SliderAttachment> (processor.state, "output", output);

    refreshRack();
    selectSlot (0);
    startTimerHz (8);
}

NeonRackEditor::~NeonRackEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
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
    auto header = content.removeFromTop (102).toFloat();
    g.setColour (juce::Colour (0xff06111d));
    g.fillRoundedRectangle (header, 20.0f);
    g.setColour (cyan.withAlpha (0.25f));
    g.drawRoundedRectangle (header, 20.0f, 1.0f);

    content.removeFromTop (14);
    auto rackArea = content.removeFromLeft (330).toFloat();
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
        auto r = slotMenus[(size_t) i].getBounds().expanded (7, 8).toFloat();
        auto m = processor.getSlot (i);
        auto c = NeonRackProcessor::moduleColour (m);
        g.setColour ((i == selectedSlot ? c : line).withAlpha (i == selectedSlot ? 0.18f : 0.09f));
        g.fillRoundedRectangle (r, 11.0f);
        g.setColour ((i == selectedSlot ? c : line).withAlpha (0.65f));
        g.drawRoundedRectangle (r, 11.0f, i == selectedSlot ? 1.4f : 1.0f);
    }
}

void NeonRackEditor::resized()
{
    auto content = getLocalBounds().reduced (26);
    auto header = content.removeFromTop (86);
    brand.setBounds (header.removeFromTop (18));
    title.setBounds (header.removeFromTop (42));
    subtitle.setBounds (header.removeFromTop (22));

    content.removeFromTop (28);
    auto rackArea = content.removeFromLeft (314).reduced (14);
    content.removeFromLeft (28);
    auto controlArea = content.reduced (20);

    const int slotH = juce::jmax (60, rackArea.getHeight() / NeonRackProcessor::numRackSlots);
    for (int i = 0; i < NeonRackProcessor::numRackSlots; ++i)
    {
        auto row = rackArea.removeFromTop (slotH).reduced (4, 7);
        slotMenus[(size_t) i].setBounds (row.removeFromTop (30));
        auto buttons = row.removeFromTop (25);
        editButtons[(size_t) i].setBounds (buttons.removeFromLeft (54));
        buttons.removeFromLeft (4);
        bypassButtons[(size_t) i].setBounds (buttons.removeFromLeft (74));
        buttons.removeFromLeft (4);
        upButtons[(size_t) i].setBounds (buttons.removeFromLeft (34));
        buttons.removeFromLeft (3);
        downButtons[(size_t) i].setBounds (buttons.removeFromLeft (34));
        buttons.removeFromLeft (3);
        removeButtons[(size_t) i].setBounds (buttons.removeFromLeft (34));
    }

    auto top = controlArea.removeFromTop (54);
    selectedTitle.setBounds (top.removeFromTop (32));
    hint.setBounds (top.removeFromTop (22));
    controlArea.removeFromTop (10);

    auto globals = controlArea.removeFromBottom (126);
    auto globalCell = globals.removeFromRight (110);
    outputLabel.setBounds (globalCell.removeFromTop (20));
    output.setBounds (globalCell);
    globals.removeFromRight (12);
    globalCell = globals.removeFromRight (110);
    globalMixLabel.setBounds (globalCell.removeFromTop (20));
    globalMix.setBounds (globalCell);

    const int count = (int) controlKnobs.size();
    if (count > 0)
    {
        const int cols = count <= 4 ? count : 3;
        const int rows = (count + cols - 1) / cols;
        const int cellW = controlArea.getWidth() / juce::jmax (1, cols);
        const int cellH = controlArea.getHeight() / juce::jmax (1, rows);
        for (int i = 0; i < count; ++i)
        {
            const int row = i / cols;
            const int col = i % cols;
            auto cell = juce::Rectangle<int> (controlArea.getX() + col * cellW,
                                              controlArea.getY() + row * cellH,
                                              cellW, cellH).reduced (8);
            controlLabels[(size_t) i]->setBounds (cell.removeFromTop (22));
            controlKnobs[(size_t) i]->setBounds (cell.reduced (8, 0));
        }
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
        const int wanted = (int) module + 1;
        if (slotMenus[(size_t) i].getSelectedId() != wanted)
            slotMenus[(size_t) i].setSelectedId (wanted, juce::dontSendNotification);

        const auto enabledId = NeonRackProcessor::enabledParameter (module);
        const bool bypassed = ! enabledId.isEmpty() && processor.state.getRawParameterValue (enabledId)->load() < 0.5f;
        bypassButtons[(size_t) i].setToggleState (bypassed, juce::dontSendNotification);
        bypassButtons[(size_t) i].setButtonText (bypassed ? "BYPASSED" : "BYPASS");
        const bool hasModule = module != NeonRackProcessor::Module::empty;
        editButtons[(size_t) i].setEnabled (hasModule);
        bypassButtons[(size_t) i].setEnabled (hasModule);
        removeButtons[(size_t) i].setEnabled (hasModule);
        upButtons[(size_t) i].setEnabled (i > 0 && hasModule);
        downButtons[(size_t) i].setEnabled (i + 1 < NeonRackProcessor::numRackSlots && hasModule);
    }
}

void NeonRackEditor::selectSlot (int slot)
{
    selectedSlot = juce::jlimit (0, NeonRackProcessor::numRackSlots - 1, slot);
    selectedModule = processor.getSlot (selectedSlot);
    rebuildControls();
    repaint();
}

void NeonRackEditor::rebuildControls()
{
    controlAttachments.clear();
    controlKnobs.clear();
    controlLabels.clear();

    selectedTitle.setText (selectedModule == NeonRackProcessor::Module::empty
                               ? "EMPTY SLOT"
                               : NeonRackProcessor::moduleName (selectedModule),
                           juce::dontSendNotification);
    selectedTitle.setColour (juce::Label::textColourId,
                             selectedModule == NeonRackProcessor::Module::empty
                                 ? juce::Colours::white
                                 : NeonRackProcessor::moduleColour (selectedModule));

    const auto ids = NeonRackProcessor::moduleParameterIds (selectedModule);
    const auto accent = NeonRackProcessor::moduleColour (selectedModule);
    for (const auto& id : ids)
    {
        auto knob = std::make_unique<juce::Slider>();
        knob->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 22);
        knob->setColour (juce::Slider::rotarySliderFillColourId, accent);
        addAndMakeVisible (*knob);

        auto label = std::make_unique<juce::Label>();
        label->setText (NeonRackProcessor::parameterLabel (id), juce::dontSendNotification);
        label->setJustificationType (juce::Justification::centred);
        label->setColour (juce::Label::textColourId, muted);
        label->setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        addAndMakeVisible (*label);

        controlAttachments.push_back (std::make_unique<SliderAttachment> (processor.state, id, *knob));
        controlKnobs.push_back (std::move (knob));
        controlLabels.push_back (std::move (label));
    }
    resized();
}
