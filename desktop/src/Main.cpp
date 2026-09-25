#include <JuceHeader.h>
#include "MainComponent.h"

class ArtistStudioStemLabApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "ArtistStudio Stem Lab"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise (const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow> (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String&) override {}

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (std::move (name), juce::Colour (0xff02060d), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setResizable (true, true);
            setResizeLimits (820, 650, 1600, 1200);
            setContentOwned (new MainComponent(), true);
            centreWithSize (getWidth(), getHeight());
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (ArtistStudioStemLabApplication)
