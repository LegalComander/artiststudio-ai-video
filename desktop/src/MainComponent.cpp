#include "MainComponent.h"

#include <thread>

namespace
{
const auto bg = juce::Colour (0xff02060d);
const auto panel = juce::Colour (0xff071522);
const auto panel2 = juce::Colour (0xff0a1d30);
const auto cyan = juce::Colour (0xff39eaff);
const auto blue = juce::Colour (0xff3f7dff);
const auto text = juce::Colour (0xffeffbff);
const auto muted = juce::Colour (0xff86a9bc);

void styleValueLabel (juce::Label& label)
{
    label.setColour (juce::Label::textColourId, text);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));
}

void styleStemLabel (juce::Label& label)
{
    label.setColour (juce::Label::textColourId, juce::Colour (0xffaeefff));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
}
}

NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::ToggleButton::textColourId, juce::Colour (0xffa6c8d9));
}

void NeonLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                            const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto area = button.getLocalBounds().toFloat().reduced (1.0f);
    auto left = isDown ? juce::Colour (0xff155fd2) : juce::Colour (0xff1879ff);
    auto right = isDown ? juce::Colour (0xff078aa9) : juce::Colour (0xff08bfdc);

    if (! button.isEnabled())
    {
        left = juce::Colour (0xff243342);
        right = juce::Colour (0xff1b2b38);
    }
    else if (isHighlighted)
    {
        left = left.brighter (0.15f);
        right = right.brighter (0.15f);
    }

    g.setGradientFill (juce::ColourGradient (left, area.getX(), area.getCentreY(), right, area.getRight(), area.getCentreY(), false));
    g.fillRoundedRectangle (area, 11.0f);
    g.setColour (cyan.withAlpha (button.isEnabled() ? 0.55f : 0.15f));
    g.drawRoundedRectangle (area, 11.0f, 1.0f);
}

void NeonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setColour (button.isEnabled() ? juce::Colours::white : juce::Colour (0xff7890a0));
    g.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (8, 2), juce::Justification::centred, 1);
}

MainComponent::MainComponent()
{
    setLookAndFeel (&lookAndFeel);
    setSize (1120, 780);

    title.setText ("ARTISTSTUDIO  STEM LAB", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    title.setFont (juce::Font (juce::FontOptions (42.0f, juce::Font::bold)));
    addAndMakeVisible (title);

    subtitle.setText ("Local track intelligence + neural stem separation", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, muted);
    subtitle.setFont (juce::Font (juce::FontOptions (16.0f)));
    addAndMakeVisible (subtitle);

    fileName.setText ("Drop a WAV / MP3 / FLAC here, or choose a track", juce::dontSendNotification);
    fileName.setColour (juce::Label::textColourId, juce::Colour (0xffbfefff));
    fileName.setJustificationType (juce::Justification::centred);
    fileName.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    addAndMakeVisible (fileName);

    status.setText ("Ready.", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, muted);
    status.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (status);

    for (auto* label : { &bpmValue, &keyValue, &durationValue, &sampleRateValue })
    {
        styleValueLabel (*label);
        label->setText ("—", juce::dontSendNotification);
        addAndMakeVisible (*label);
    }

    vocalsStatus.setText ("🎤  Vocals   • waiting", juce::dontSendNotification);
    drumsStatus.setText  ("🥁  Drums    • waiting", juce::dontSendNotification);
    bassStatus.setText   ("🔊  Bass     • waiting", juce::dontSendNotification);
    otherStatus.setText  ("🎹  Other    • waiting", juce::dontSendNotification);

    for (auto* label : { &vocalsStatus, &drumsStatus, &bassStatus, &otherStatus })
    {
        styleStemLabel (*label);
        addAndMakeVisible (*label);
    }

    for (auto* button : { &chooseButton, &analyzeButton, &separateButton, &outputButton })
    {
        button->addListener (this);
        addAndMakeVisible (*button);
    }

    maximumQuality.setColour (juce::ToggleButton::textColourId, muted);
    addAndMakeVisible (maximumQuality);

    analyzeButton.setEnabled (false);
    separateButton.setEnabled (false);
    outputButton.setEnabled (false);
}

MainComponent::~MainComponent()
{
    stemEngine.cancel();
    setLookAndFeel (nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    juce::ColourGradient backgroundGlow (juce::Colour (0xff08294d), 0.0f, 0.0f,
                                         bg, static_cast<float> (getWidth()), static_cast<float> (getHeight()), false);
    backgroundGlow.addColour (0.38, juce::Colour (0xff061522));
    g.setGradientFill (backgroundGlow);
    g.fillRect (getLocalBounds());

    auto bounds = getLocalBounds().reduced (24);
    auto hero = bounds.removeFromTop (112).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff06111f), hero.getX(), hero.getY(),
                                             juce::Colour (0xff0a3155), hero.getRight(), hero.getBottom(), false));
    g.fillRoundedRectangle (hero, 24.0f);
    g.setColour (cyan.withAlpha (0.24f));
    g.drawRoundedRectangle (hero, 24.0f, 1.0f);

    bounds.removeFromTop (16);
    auto drop = bounds.removeFromTop (116).toFloat();
    g.setColour (juce::Colour (0xff061420));
    g.fillRoundedRectangle (drop, 18.0f);
    g.setColour (cyan.withAlpha (0.34f));
    g.drawRoundedRectangle (drop, 18.0f, 1.3f);

    bounds.removeFromTop (72);
    auto metrics = bounds.removeFromTop (112);
    const int gap = 12;
    const int w = (metrics.getWidth() - gap * 3) / 4;
    const juce::StringArray captions { "BPM", "MUSICAL KEY", "LENGTH", "SAMPLE RATE" };

    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    for (int i = 0; i < 4; ++i)
    {
        auto card = juce::Rectangle<int> (metrics.getX() + i * (w + gap), metrics.getY(), w, metrics.getHeight()).toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (card, 16.0f);
        g.setColour (cyan.withAlpha (0.18f));
        g.drawRoundedRectangle (card, 16.0f, 1.0f);
        g.setColour (juce::Colour (0xff73dff3));
        g.drawText (captions[i], card.toNearestInt().removeFromTop (28).reduced (12, 4), juce::Justification::centredLeft);
    }

    bounds.removeFromTop (14);
    auto stems = bounds.removeFromTop (174);
    const int stemW = (stems.getWidth() - gap) / 2;
    for (int i = 0; i < 4; ++i)
    {
        const int row = i / 2;
        const int col = i % 2;
        auto card = juce::Rectangle<int> (stems.getX() + col * (stemW + gap), stems.getY() + row * 81, stemW, 69).toFloat();
        g.setGradientFill (juce::ColourGradient (panel2, card.getX(), card.getY(), panel, card.getRight(), card.getBottom(), false));
        g.fillRoundedRectangle (card, 15.0f);
        g.setColour (blue.withAlpha (0.27f));
        g.drawRoundedRectangle (card, 15.0f, 1.0f);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (24);
    auto hero = bounds.removeFromTop (112).reduced (24, 16);
    title.setBounds (hero.removeFromTop (52));
    subtitle.setBounds (hero.removeFromTop (30));

    bounds.removeFromTop (16);
    auto drop = bounds.removeFromTop (116).reduced (18);
    fileName.setBounds (drop.removeFromTop (44));
    chooseButton.setBounds (drop.withSizeKeepingCentre (180, 40));

    auto buttons = bounds.removeFromTop (72).reduced (0, 14);
    const int buttonW = 185;
    analyzeButton.setBounds (buttons.removeFromLeft (buttonW));
    buttons.removeFromLeft (10);
    separateButton.setBounds (buttons.removeFromLeft (buttonW));
    buttons.removeFromLeft (14);
    maximumQuality.setBounds (buttons.removeFromLeft (225));
    buttons.removeFromLeft (10);
    outputButton.setBounds (buttons.removeFromLeft (185));

    auto metrics = bounds.removeFromTop (112);
    const int gap = 12;
    const int w = (metrics.getWidth() - gap * 3) / 4;
    auto placeMetric = [&] (juce::Label& label, int i)
    {
        auto r = juce::Rectangle<int> (metrics.getX() + i * (w + gap), metrics.getY(), w, metrics.getHeight());
        label.setBounds (r.reduced (10).withTrimmedTop (25));
    };
    placeMetric (bpmValue, 0);
    placeMetric (keyValue, 1);
    placeMetric (durationValue, 2);
    placeMetric (sampleRateValue, 3);

    bounds.removeFromTop (14);
    auto stems = bounds.removeFromTop (174);
    const int stemW = (stems.getWidth() - gap) / 2;
    vocalsStatus.setBounds (stems.getX() + 14, stems.getY() + 7, stemW - 28, 55);
    drumsStatus.setBounds (stems.getX() + stemW + gap + 14, stems.getY() + 7, stemW - 28, 55);
    bassStatus.setBounds (stems.getX() + 14, stems.getY() + 88, stemW - 28, 55);
    otherStatus.setBounds (stems.getX() + stemW + gap + 14, stems.getY() + 88, stemW - 28, 55);

    status.setBounds (bounds.removeFromTop (48).reduced (6, 4));
}

bool MainComponent::isInterestedInFileDrag (const juce::StringArray& files)
{
    if (files.isEmpty())
        return false;

    const auto ext = juce::File (files[0]).getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".aiff" || ext == ".aif";
}

void MainComponent::filesDropped (const juce::StringArray& files, int, int)
{
    if (isInterestedInFileDrag (files))
        loadFile (juce::File (files[0]));
}

void MainComponent::buttonClicked (juce::Button* button)
{
    if (button == &chooseButton)
        chooseFile();
    else if (button == &analyzeButton)
        analyzeCurrentTrack();
    else if (button == &separateButton)
        separateCurrentTrack();
    else if (button == &outputButton && lastStemFolder.isDirectory())
        lastStemFolder.startAsProcess();
}

void MainComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a track", juce::File(), "*.wav;*.mp3;*.flac;*.aif;*.aiff");
    const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (flags, [safeThis = juce::Component::SafePointer<MainComponent> (this)] (const juce::FileChooser& chooser)
    {
        if (safeThis != nullptr)
        {
            const auto file = chooser.getResult();
            if (file.existsAsFile())
                safeThis->loadFile (file);
        }
    });
}

void MainComponent::loadFile (const juce::File& file)
{
    currentFile = file;
    lastStemFolder = {};
    fileName.setText (file.getFileName(), juce::dontSendNotification);
    bpmValue.setText ("—", juce::dontSendNotification);
    keyValue.setText ("—", juce::dontSendNotification);
    durationValue.setText ("—", juce::dontSendNotification);
    sampleRateValue.setText ("—", juce::dontSendNotification);
    vocalsStatus.setText ("🎤  Vocals   • waiting", juce::dontSendNotification);
    drumsStatus.setText  ("🥁  Drums    • waiting", juce::dontSendNotification);
    bassStatus.setText   ("🔊  Bass     • waiting", juce::dontSendNotification);
    otherStatus.setText  ("🎹  Other    • waiting", juce::dontSendNotification);
    analyzeButton.setEnabled (true);
    separateButton.setEnabled (true);
    outputButton.setEnabled (false);
    setStatus ("Track loaded. Analyze it or start local stem separation.", muted);
}

void MainComponent::analyzeCurrentTrack()
{
    if (! currentFile.existsAsFile())
        return;

    analyzeButton.setEnabled (false);
    setStatus ("Analyzing BPM, musical key and track metadata…", cyan);

    auto file = currentFile;
    auto safeThis = juce::Component::SafePointer<MainComponent> (this);
    std::thread ([safeThis, file] () mutable
    {
        auto result = TrackAnalyzer::analyze (file);
        juce::MessageManager::callAsync ([safeThis, result = std::move (result)] () mutable
        {
            if (safeThis == nullptr)
                return;

            safeThis->analyzeButton.setEnabled (true);
            if (! result.ok)
            {
                safeThis->setStatus ("Analysis failed: " + result.error, juce::Colour (0xffff8ba5));
                return;
            }

            safeThis->bpmValue.setText (juce::String (result.bpm, 0), juce::dontSendNotification);
            safeThis->keyValue.setText (result.key, juce::dontSendNotification);
            safeThis->durationValue.setText (formatDuration (result.durationSeconds), juce::dontSendNotification);
            safeThis->sampleRateValue.setText (juce::String (result.sampleRate / 1000.0, 1) + " kHz", juce::dontSendNotification);
            safeThis->setStatus ("Analysis complete. Everything ran locally on this computer.", juce::Colour (0xff8ff5c9));
        });
    }).detach();
}

void MainComponent::separateCurrentTrack()
{
    if (! currentFile.existsAsFile() || stemEngine.isBusy())
        return;

    const auto demucs = stemEngine.detectDemucsLauncher();
    if (demucs.isEmpty())
    {
        setStatus ("Local AI engine not installed yet. Install Python 3, then run: pip install -U demucs", juce::Colour (0xffffbb72));
        return;
    }

    auto root = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("ArtistStudio Stem Lab")
                    .getChildFile ("Stems");

    separateButton.setEnabled (false);
    analyzeButton.setEnabled (false);
    vocalsStatus.setText ("🎤  Vocals   • processing", juce::dontSendNotification);
    drumsStatus.setText  ("🥁  Drums    • processing", juce::dontSendNotification);
    bassStatus.setText   ("🔊  Bass     • processing", juce::dontSendNotification);
    otherStatus.setText  ("🎹  Other    • processing", juce::dontSendNotification);
    setStatus ("Demucs is separating four stems locally. This can take a few minutes…", cyan);

    stemEngine.startSeparation (currentFile, root, maximumQuality.getToggleState(),
                                [safeThis = juce::Component::SafePointer<MainComponent> (this)] (StemSeparationResult result)
    {
        if (safeThis != nullptr)
            safeThis->updateStemStatus (result);
    });
}

void MainComponent::updateStemStatus (const StemSeparationResult& result)
{
    separateButton.setEnabled (true);
    analyzeButton.setEnabled (currentFile.existsAsFile());

    if (! result.ok)
    {
        vocalsStatus.setText ("🎤  Vocals   • failed", juce::dontSendNotification);
        drumsStatus.setText  ("🥁  Drums    • failed", juce::dontSendNotification);
        bassStatus.setText   ("🔊  Bass     • failed", juce::dontSendNotification);
        otherStatus.setText  ("🎹  Other    • failed", juce::dontSendNotification);
        setStatus ("Stem separation failed: " + result.error, juce::Colour (0xffff8ba5));
        return;
    }

    lastStemFolder = result.outputDirectory;
    outputButton.setEnabled (true);
    vocalsStatus.setText ("🎤  Vocals   • ready WAV", juce::dontSendNotification);
    drumsStatus.setText  ("🥁  Drums    • ready WAV", juce::dontSendNotification);
    bassStatus.setText   ("🔊  Bass     • ready WAV", juce::dontSendNotification);
    otherStatus.setText  ("🎹  Other    • ready WAV", juce::dontSendNotification);
    setStatus ("Four stems are ready. Open the output folder to use them in your DAW.", juce::Colour (0xff8ff5c9));
}

void MainComponent::setStatus (juce::String value, juce::Colour colour)
{
    status.setColour (juce::Label::textColourId, colour);
    status.setText (std::move (value), juce::dontSendNotification);
}

juce::String MainComponent::formatDuration (double seconds)
{
    const auto total = juce::jmax (0, static_cast<int> (std::round (seconds)));
    return juce::String (total / 60) + ":" + juce::String (total % 60).paddedLeft ('0', 2);
}
