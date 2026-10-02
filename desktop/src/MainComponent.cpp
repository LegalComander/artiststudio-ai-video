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
const auto green = juce::Colour (0xff8ff5c9);
const auto warning = juce::Colour (0xffffbb72);
const auto danger = juce::Colour (0xffff8ba5);

void styleValueLabel (juce::Label& label)
{
    label.setColour (juce::Label::textColourId, text);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (25.0f, juce::Font::bold)));
}

void styleStemLabel (juce::Label& label)
{
    label.setColour (juce::Label::textColourId, juce::Colour (0xffaeefff));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::Font (juce::FontOptions (14.5f, juce::Font::bold)));
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

    g.setGradientFill (juce::ColourGradient (left, area.getX(), area.getCentreY(),
                                             right, area.getRight(), area.getCentreY(), false));
    g.fillRoundedRectangle (area, 10.0f);
    g.setColour (cyan.withAlpha (button.isEnabled() ? 0.55f : 0.15f));
    g.drawRoundedRectangle (area, 10.0f, 1.0f);
}

void NeonLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setColour (button.isEnabled() ? juce::Colours::white : juce::Colour (0xff7890a0));
    g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (6, 2),
                      juce::Justification::centred, 1);
}

MainComponent::MainComponent (PreviewEngine* sharedPreviewEngine)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1120, 860);

    previewEngine = sharedPreviewEngine != nullptr ? sharedPreviewEngine : &ownedPreviewEngine;
    if (sharedPreviewEngine == nullptr)
    {
        const auto audioError = previewDeviceManager.initialiseWithDefaultDevices (0, 2);
        if (audioError.isEmpty())
        {
            previewSourcePlayer.setSource (previewEngine);
            previewDeviceManager.addAudioCallback (&previewSourcePlayer);
            ownsPreviewAudioDevice = true;
        }
    }

    thumbnailFormats.registerBasicFormats();

    title.setText ("ARTISTSTUDIO  STEM LAB", juce::dontSendNotification);
    title.setColour (juce::Label::textColourId, juce::Colours::white);
    title.setFont (juce::Font (juce::FontOptions (38.0f, juce::Font::bold)));
    addAndMakeVisible (title);

    versionLabel.setText ("v0.3  PREVIEW MIXER", juce::dontSendNotification);
    versionLabel.setColour (juce::Label::textColourId, cyan);
    versionLabel.setJustificationType (juce::Justification::centredRight);
    versionLabel.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    addAndMakeVisible (versionLabel);

    subtitle.setText ("Local track intelligence + neural stem separation", juce::dontSendNotification);
    subtitle.setColour (juce::Label::textColourId, muted);
    subtitle.setFont (juce::Font (juce::FontOptions (15.0f)));
    addAndMakeVisible (subtitle);

    fileName.setText ("Drop a WAV / MP3 / FLAC here, or choose a track", juce::dontSendNotification);
    fileName.setColour (juce::Label::textColourId, juce::Colour (0xffbfefff));
    fileName.setJustificationType (juce::Justification::centred);
    fileName.setFont (juce::Font (juce::FontOptions (16.0f, juce::Font::bold)));
    addAndMakeVisible (fileName);

    status.setText ("Ready.", juce::dontSendNotification);
    status.setColour (juce::Label::textColourId, muted);
    status.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (status);

    for (auto* label : { &bpmValue, &keyValue, &durationValue, &sampleRateValue })
    {
        styleValueLabel (*label);
        label->setText ("--", juce::dontSendNotification);
        addAndMakeVisible (*label);
    }

    vocalsStatus.setText ("VOCALS  - waiting", juce::dontSendNotification);
    drumsStatus.setText  ("DRUMS   - waiting", juce::dontSendNotification);
    bassStatus.setText   ("BASS    - waiting", juce::dontSendNotification);
    otherStatus.setText  ("OTHER   - waiting", juce::dontSendNotification);

    for (auto* label : { &vocalsStatus, &drumsStatus, &bassStatus, &otherStatus })
    {
        styleStemLabel (*label);
        addAndMakeVisible (*label);
    }

    for (auto* button : { &chooseButton, &analyzeButton, &engineButton, &separateButton,
                          &cancelButton, &outputButton, &playOriginalButton,
                          &playStemsButton, &stopPreviewButton })
    {
        button->addListener (this);
        addAndMakeVisible (*button);
    }

    maximumQuality.setColour (juce::ToggleButton::textColourId, muted);
    addAndMakeVisible (maximumQuality);

    separationProgressBar.setColour (juce::ProgressBar::backgroundColourId, panel2);
    separationProgressBar.setColour (juce::ProgressBar::foregroundColourId, cyan);
    separationProgressBar.setPercentageDisplay (true);
    addAndMakeVisible (separationProgressBar);

    for (int i = 0; i < stemCount; ++i)
    {
        auto& solo = stemSoloButtons[(size_t) i];
        auto& mute = stemMuteButtons[(size_t) i];
        auto& volume = stemVolumeSliders[(size_t) i];

        solo.setButtonText ("S");
        mute.setButtonText ("M");
        solo.setColour (juce::ToggleButton::textColourId, juce::Colour (0xffffda73));
        solo.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffffda73));
        mute.setColour (juce::ToggleButton::textColourId, danger);
        mute.setColour (juce::ToggleButton::tickColourId, danger);
        addAndMakeVisible (solo);
        addAndMakeVisible (mute);

        volume.setSliderStyle (juce::Slider::LinearHorizontal);
        volume.setRange (0.0, 1.25, 0.01);
        volume.setValue (1.0, juce::dontSendNotification);
        volume.setTextBoxStyle (juce::Slider::TextBoxRight, false, 52, 20);
        volume.setNumDecimalPlacesToDisplay (2);
        volume.setColour (juce::Slider::trackColourId, cyan);
        volume.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff153044));
        volume.setColour (juce::Slider::thumbColourId, text);
        volume.setColour (juce::Slider::textBoxTextColourId, text);
        volume.setColour (juce::Slider::textBoxBackgroundColourId, panel);
        volume.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (volume);

        solo.onClick = [this, i]
        {
            previewEngine->setStemSolo (previewTrackForStem (i), stemSoloButtons[(size_t) i].getToggleState());
        };
        mute.onClick = [this, i]
        {
            previewEngine->setStemMute (previewTrackForStem (i), stemMuteButtons[(size_t) i].getToggleState());
        };
        volume.onValueChange = [this, i]
        {
            previewEngine->setStemGain (previewTrackForStem (i), (float) stemVolumeSliders[(size_t) i].getValue());
        };
    }

    analyzeButton.setEnabled (false);
    separateButton.setEnabled (false);
    cancelButton.setEnabled (false);
    outputButton.setEnabled (false);
    playOriginalButton.setEnabled (false);
    playStemsButton.setEnabled (false);
    stopPreviewButton.setEnabled (false);
    separationProgressBar.setVisible (false);
    resetStemMixer();
    refreshEngineButton();
}

MainComponent::~MainComponent()
{
    stemEngine.cancel();
    if (previewEngine != nullptr)
        previewEngine->stop();

    if (ownsPreviewAudioDevice)
    {
        previewDeviceManager.removeAudioCallback (&previewSourcePlayer);
        previewSourcePlayer.setSource (nullptr);
    }

    thumbnail.setSource (nullptr);
    setLookAndFeel (nullptr);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    juce::ColourGradient backgroundGlow (juce::Colour (0xff08294d), 0.0f, 0.0f,
                                         bg, (float) getWidth(), (float) getHeight(), false);
    backgroundGlow.addColour (0.38, juce::Colour (0xff061522));
    g.setGradientFill (backgroundGlow);
    g.fillRect (getLocalBounds());

    auto bounds = getLocalBounds().reduced (22);
    auto hero = bounds.removeFromTop (92).toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff06111f), hero.getX(), hero.getY(),
                                             juce::Colour (0xff0a3155), hero.getRight(), hero.getBottom(), false));
    g.fillRoundedRectangle (hero, 22.0f);
    g.setColour (cyan.withAlpha (0.24f));
    g.drawRoundedRectangle (hero, 22.0f, 1.0f);

    bounds.removeFromTop (12);
    auto drop = bounds.removeFromTop (78).toFloat();
    g.setColour (juce::Colour (0xff061420));
    g.fillRoundedRectangle (drop, 16.0f);
    g.setColour (cyan.withAlpha (0.34f));
    g.drawRoundedRectangle (drop, 16.0f, 1.3f);

    bounds.removeFromTop (10);
    auto waveformCard = bounds.removeFromTop (104).toFloat();
    g.setColour (juce::Colour (0xff040c14));
    g.fillRoundedRectangle (waveformCard, 15.0f);
    g.setColour (blue.withAlpha (0.32f));
    g.drawRoundedRectangle (waveformCard, 15.0f, 1.0f);

    if (! waveformBounds.isEmpty())
    {
        if (thumbnail.getTotalLength() > 0.0)
        {
            g.setColour (cyan.withAlpha (0.88f));
            thumbnail.drawChannels (g, waveformBounds, 0.0, thumbnail.getTotalLength(), 1.0f);
        }
        else
        {
            g.setColour (muted);
            g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
            g.drawText ("TRACK WAVEFORM", waveformBounds, juce::Justification::centred, false);
        }
    }

    bounds.removeFromTop (58);
    auto metrics = bounds.removeFromTop (94);
    const int gap = 10;
    const int w = (metrics.getWidth() - gap * 3) / 4;
    const juce::StringArray captions { "BPM", "MUSICAL KEY", "LENGTH", "SAMPLE RATE" };

    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    for (int i = 0; i < 4; ++i)
    {
        auto card = juce::Rectangle<int> (metrics.getX() + i * (w + gap), metrics.getY(), w, metrics.getHeight()).toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (card, 14.0f);
        g.setColour (cyan.withAlpha (0.18f));
        g.drawRoundedRectangle (card, 14.0f, 1.0f);
        g.setColour (juce::Colour (0xff73dff3));
        g.drawText (captions[i], card.toNearestInt().removeFromTop (25).reduced (10, 3), juce::Justification::centredLeft);
    }

    bounds.removeFromTop (42);
    auto stems = bounds.removeFromTop (190);
    const int stemW = (stems.getWidth() - gap) / 2;
    for (int i = 0; i < 4; ++i)
    {
        const int row = i / 2;
        const int col = i % 2;
        auto card = juce::Rectangle<int> (stems.getX() + col * (stemW + gap),
                                          stems.getY() + row * 94,
                                          stemW, 84).toFloat();
        g.setGradientFill (juce::ColourGradient (panel2, card.getX(), card.getY(),
                                                 panel, card.getRight(), card.getBottom(), false));
        g.fillRoundedRectangle (card, 14.0f);
        g.setColour (blue.withAlpha (0.27f));
        g.drawRoundedRectangle (card, 14.0f, 1.0f);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced (22);
    auto hero = bounds.removeFromTop (92).reduced (22, 11);
    auto titleRow = hero.removeFromTop (43);
    title.setBounds (titleRow.removeFromLeft (650));
    versionLabel.setBounds (titleRow);
    subtitle.setBounds (hero.removeFromTop (25));

    bounds.removeFromTop (12);
    auto drop = bounds.removeFromTop (78).reduced (16, 10);
    fileName.setBounds (drop.removeFromTop (30));
    chooseButton.setBounds (drop.withSizeKeepingCentre (170, 32));

    bounds.removeFromTop (10);
    auto waveformCard = bounds.removeFromTop (104).reduced (12, 10);
    waveformBounds = waveformCard;

    auto buttons = bounds.removeFromTop (58).reduced (0, 10);
    analyzeButton.setBounds (buttons.removeFromLeft (150));
    buttons.removeFromLeft (7);
    engineButton.setBounds (buttons.removeFromLeft (145));
    buttons.removeFromLeft (7);
    separateButton.setBounds (buttons.removeFromLeft (150));
    buttons.removeFromLeft (7);
    cancelButton.setBounds (buttons.removeFromLeft (82));
    buttons.removeFromLeft (10);
    maximumQuality.setBounds (buttons.removeFromLeft (188));
    buttons.removeFromLeft (7);
    outputButton.setBounds (buttons.removeFromLeft (170));

    auto metrics = bounds.removeFromTop (94);
    const int gap = 10;
    const int w = (metrics.getWidth() - gap * 3) / 4;
    auto placeMetric = [&] (juce::Label& label, int i)
    {
        auto r = juce::Rectangle<int> (metrics.getX() + i * (w + gap), metrics.getY(), w, metrics.getHeight());
        label.setBounds (r.reduced (8).withTrimmedTop (22));
    };
    placeMetric (bpmValue, 0);
    placeMetric (keyValue, 1);
    placeMetric (durationValue, 2);
    placeMetric (sampleRateValue, 3);

    auto progressRow = bounds.removeFromTop (42).reduced (3, 7);
    separationProgressBar.setBounds (progressRow.removeFromLeft (340));
    progressRow.removeFromLeft (12);
    playOriginalButton.setBounds (progressRow.removeFromLeft (125));
    progressRow.removeFromLeft (6);
    playStemsButton.setBounds (progressRow.removeFromLeft (125));
    progressRow.removeFromLeft (6);
    stopPreviewButton.setBounds (progressRow.removeFromLeft (82));

    auto stems = bounds.removeFromTop (190);
    const int stemW = (stems.getWidth() - gap) / 2;
    std::array<juce::Label*, stemCount> labels { &vocalsStatus, &drumsStatus, &bassStatus, &otherStatus };

    for (int i = 0; i < stemCount; ++i)
    {
        const int row = i / 2;
        const int col = i % 2;
        auto card = juce::Rectangle<int> (stems.getX() + col * (stemW + gap),
                                          stems.getY() + row * 94,
                                          stemW, 84).reduced (12, 8);

        labels[(size_t) i]->setBounds (card.removeFromTop (28));
        auto mixer = card.removeFromTop (38);
        stemSoloButtons[(size_t) i].setBounds (mixer.removeFromLeft (46));
        stemMuteButtons[(size_t) i].setBounds (mixer.removeFromLeft (46));
        mixer.removeFromLeft (6);
        stemVolumeSliders[(size_t) i].setBounds (mixer);
    }

    status.setBounds (bounds.removeFromTop (42).reduced (5, 4));
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
    else if (button == &engineButton)
        installLocalEngine();
    else if (button == &separateButton)
        separateCurrentTrack();
    else if (button == &cancelButton)
        cancelSeparation();
    else if (button == &outputButton && lastStemFolder.isDirectory())
        lastStemFolder.startAsProcess();
    else if (button == &playOriginalButton)
    {
        previewEngine->playOriginal();
        stopPreviewButton.setEnabled (true);
        setStatus ("Previewing the original track.", cyan);
    }
    else if (button == &playStemsButton)
    {
        previewEngine->playStemMix();
        stopPreviewButton.setEnabled (true);
        setStatus ("Previewing the synchronized stem mix. Use S, M and volume controls below.", cyan);
    }
    else if (button == &stopPreviewButton)
    {
        previewEngine->stop();
        stopPreviewButton.setEnabled (false);
        setStatus ("Preview stopped.", muted);
    }
}

void MainComponent::chooseFile()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Choose a track", juce::File(), "*.wav;*.mp3;*.flac;*.aif;*.aiff");
    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (chooserFlags, [safeThis = juce::Component::SafePointer<MainComponent> (this)] (const juce::FileChooser& chooser)
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
    if (stemEngine.isBusy())
    {
        setStatus ("Finish or cancel the current AI job before loading another track.", warning);
        return;
    }

    previewEngine->stop();
    previewEngine->clearStems();
    previewEngine->loadTrack (PreviewEngine::Track::original, file);

    currentFile = file;
    lastStemFolder = {};
    fileName.setText (file.getFileName(), juce::dontSendNotification);
    bpmValue.setText ("--", juce::dontSendNotification);
    keyValue.setText ("--", juce::dontSendNotification);
    durationValue.setText ("--", juce::dontSendNotification);
    sampleRateValue.setText ("--", juce::dontSendNotification);
    vocalsStatus.setText ("VOCALS  - waiting", juce::dontSendNotification);
    drumsStatus.setText  ("DRUMS   - waiting", juce::dontSendNotification);
    bassStatus.setText   ("BASS    - waiting", juce::dontSendNotification);
    otherStatus.setText  ("OTHER   - waiting", juce::dontSendNotification);

    thumbnail.setSource (new juce::FileInputSource (file));
    resetStemMixer();
    separationProgress = 0.0;
    separationProgressBar.setVisible (false);

    analyzeButton.setEnabled (true);
    separateButton.setEnabled (true);
    cancelButton.setEnabled (false);
    outputButton.setEnabled (false);
    playOriginalButton.setEnabled (true);
    playStemsButton.setEnabled (false);
    stopPreviewButton.setEnabled (false);
    setStatus ("Track loaded. Analyze it or start local stem separation.", muted);
    repaint();
}

void MainComponent::analyzeCurrentTrack()
{
    if (! currentFile.existsAsFile())
        return;

    analyzeButton.setEnabled (false);
    setStatus ("Analyzing BPM, musical key and track metadata...", cyan);

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
                safeThis->setStatus ("Analysis failed: " + result.error, danger);
                return;
            }

            safeThis->bpmValue.setText (juce::String (result.bpm, 0), juce::dontSendNotification);
            safeThis->keyValue.setText (result.key, juce::dontSendNotification);
            safeThis->durationValue.setText (formatDuration (result.durationSeconds), juce::dontSendNotification);
            safeThis->sampleRateValue.setText (juce::String (result.sampleRate / 1000.0, 1) + " kHz", juce::dontSendNotification);
            safeThis->setStatus ("Analysis complete. Everything ran locally on this computer.", green);
        });
    }).detach();
}

void MainComponent::refreshEngineButton()
{
    const auto ready = stemEngine.isEngineReady();
    engineButton.setButtonText (ready ? "AI Engine Ready" : "Install Local AI");
    engineButton.setEnabled (! ready && ! stemEngine.isBusy());
}

void MainComponent::installLocalEngine()
{
    if (stemEngine.isBusy())
        return;

    if (stemEngine.isEngineReady())
    {
        refreshEngineButton();
        setStatus ("Local AI engine is already installed and ready.", green);
        return;
    }

    engineButton.setButtonText ("Installing AI...");
    engineButton.setEnabled (false);
    separateButton.setEnabled (false);
    analyzeButton.setEnabled (false);
    setStatus ("Setting up ArtistStudio's private local AI engine. First setup downloads AI dependencies...", cyan);

    stemEngine.startEngineSetup ([safeThis = juce::Component::SafePointer<MainComponent> (this)] (EngineSetupResult result)
    {
        if (safeThis == nullptr)
            return;

        safeThis->refreshEngineButton();
        safeThis->analyzeButton.setEnabled (safeThis->currentFile.existsAsFile());
        safeThis->separateButton.setEnabled (safeThis->currentFile.existsAsFile());

        if (result.ok)
            safeThis->setStatus ("Local AI engine installed and verified. Stem separation is ready.", green);
        else
            safeThis->setStatus ("AI engine setup failed: " + result.error, danger);
    });
}

void MainComponent::separateCurrentTrack()
{
    if (! currentFile.existsAsFile() || stemEngine.isBusy())
        return;

    const auto demucs = stemEngine.detectDemucsLauncher();
    if (demucs.isEmpty())
    {
        refreshEngineButton();
        setStatus ("Local AI is not installed yet. Press Install Local AI once, then separation will run here on your PC.", warning);
        return;
    }

    previewEngine->stop();
    auto root = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("ArtistStudio Stem Lab")
                    .getChildFile ("Stems");

    separateButton.setEnabled (false);
    analyzeButton.setEnabled (false);
    engineButton.setEnabled (false);
    cancelButton.setEnabled (true);
    playStemsButton.setEnabled (false);
    separationProgress = 0.01;
    separationProgressBar.setVisible (true);
    vocalsStatus.setText ("VOCALS  - processing", juce::dontSendNotification);
    drumsStatus.setText  ("DRUMS   - processing", juce::dontSendNotification);
    bassStatus.setText   ("BASS    - processing", juce::dontSendNotification);
    otherStatus.setText  ("OTHER   - processing", juce::dontSendNotification);
    setStatus ("Demucs is separating four stems locally...", cyan);

    stemEngine.startSeparation (currentFile, root, maximumQuality.getToggleState(),
                                [safeThis = juce::Component::SafePointer<MainComponent> (this)] (double progress, juce::String message)
    {
        if (safeThis == nullptr)
            return;
        safeThis->separationProgress = progress;
        if (message.isNotEmpty())
            safeThis->setStatus (std::move (message), cyan);
    },
                                [safeThis = juce::Component::SafePointer<MainComponent> (this)] (StemSeparationResult result)
    {
        if (safeThis != nullptr)
            safeThis->updateStemStatus (result);
    });
}

void MainComponent::cancelSeparation()
{
    if (! stemEngine.isBusy())
        return;

    cancelButton.setEnabled (false);
    setStatus ("Cancelling stem separation...", warning);
    stemEngine.requestCancel();
}

void MainComponent::updateStemStatus (const StemSeparationResult& result)
{
    separateButton.setEnabled (currentFile.existsAsFile());
    analyzeButton.setEnabled (currentFile.existsAsFile());
    cancelButton.setEnabled (false);
    refreshEngineButton();

    if (! result.ok)
    {
        const bool cancelled = result.error.containsIgnoreCase ("cancel");
        vocalsStatus.setText (cancelled ? "VOCALS  - waiting" : "VOCALS  - failed", juce::dontSendNotification);
        drumsStatus.setText  (cancelled ? "DRUMS   - waiting" : "DRUMS   - failed", juce::dontSendNotification);
        bassStatus.setText   (cancelled ? "BASS    - waiting" : "BASS    - failed", juce::dontSendNotification);
        otherStatus.setText  (cancelled ? "OTHER   - waiting" : "OTHER   - failed", juce::dontSendNotification);
        separationProgress = 0.0;
        separationProgressBar.setVisible (false);
        setStatus (cancelled ? "Stem separation cancelled." : "Stem separation failed: " + result.error,
                   cancelled ? warning : danger);
        return;
    }

    lastStemFolder = result.outputDirectory;
    loadPreviewFiles (result);
    separationProgress = 1.0;
    outputButton.setEnabled (true);
    playStemsButton.setEnabled (true);
    vocalsStatus.setText ("VOCALS  - ready WAV", juce::dontSendNotification);
    drumsStatus.setText  ("DRUMS   - ready WAV", juce::dontSendNotification);
    bassStatus.setText   ("BASS    - ready WAV", juce::dontSendNotification);
    otherStatus.setText  ("OTHER   - ready WAV", juce::dontSendNotification);
    setStatus ("Four stems are ready. Preview the mix, solo/mute stems, or open the output folder.", green);
}

void MainComponent::loadPreviewFiles (const StemSeparationResult& result)
{
    previewEngine->loadTrack (PreviewEngine::Track::vocals, result.vocals);
    previewEngine->loadTrack (PreviewEngine::Track::drums, result.drums);
    previewEngine->loadTrack (PreviewEngine::Track::bass, result.bass);
    previewEngine->loadTrack (PreviewEngine::Track::other, result.other);
    resetStemMixer();
}

void MainComponent::resetStemMixer()
{
    for (int i = 0; i < stemCount; ++i)
    {
        stemSoloButtons[(size_t) i].setToggleState (false, juce::dontSendNotification);
        stemMuteButtons[(size_t) i].setToggleState (false, juce::dontSendNotification);
        stemVolumeSliders[(size_t) i].setValue (1.0, juce::dontSendNotification);
        previewEngine->setStemSolo (previewTrackForStem (i), false);
        previewEngine->setStemMute (previewTrackForStem (i), false);
        previewEngine->setStemGain (previewTrackForStem (i), 1.0f);
    }
}

void MainComponent::updateStemMixerState()
{
    for (int i = 0; i < stemCount; ++i)
    {
        previewEngine->setStemSolo (previewTrackForStem (i), stemSoloButtons[(size_t) i].getToggleState());
        previewEngine->setStemMute (previewTrackForStem (i), stemMuteButtons[(size_t) i].getToggleState());
        previewEngine->setStemGain (previewTrackForStem (i), (float) stemVolumeSliders[(size_t) i].getValue());
    }
}

PreviewEngine::Track MainComponent::previewTrackForStem (int index)
{
    switch (index)
    {
        case 0: return PreviewEngine::Track::vocals;
        case 1: return PreviewEngine::Track::drums;
        case 2: return PreviewEngine::Track::bass;
        default: return PreviewEngine::Track::other;
    }
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
