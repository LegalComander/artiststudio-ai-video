#pragma once

#include <JuceHeader.h>

struct TrackAnalysisResult
{
    bool ok = false;
    double bpm = 0.0;
    juce::String key;
    double durationSeconds = 0.0;
    double sampleRate = 0.0;
    juce::String error;
};

class TrackAnalyzer
{
public:
    static TrackAnalysisResult analyze (const juce::File& file);

private:
    static double estimateBpm (const juce::AudioBuffer<float>& buffer, double sampleRate);
    static juce::String estimateKey (const juce::AudioBuffer<float>& buffer, double sampleRate);
};
