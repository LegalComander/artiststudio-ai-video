#include "TrackAnalyzer.h"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
std::vector<float> makeMono (const juce::AudioBuffer<float>& buffer)
{
    std::vector<float> mono (static_cast<size_t> (buffer.getNumSamples()), 0.0f);
    const auto channels = juce::jmax (1, buffer.getNumChannels());

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            mono[static_cast<size_t> (i)] += data[i] / static_cast<float> (channels);
    }

    return mono;
}
}

TrackAnalysisResult TrackAnalyzer::analyze (const juce::File& file)
{
    TrackAnalysisResult result;

    if (! file.existsAsFile())
    {
        result.error = "Audio file does not exist.";
        return result;
    }

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr)
    {
        result.error = "Unsupported or unreadable audio file.";
        return result;
    }

    const auto maxSamples = static_cast<juce::int64> (reader->sampleRate * 10.0 * 60.0);
    const auto samplesToRead64 = juce::jmin (reader->lengthInSamples, maxSamples);
    const auto samplesToRead = static_cast<int> (juce::jmin<juce::int64> (samplesToRead64, std::numeric_limits<int>::max()));
    const auto channels = juce::jlimit (1, 2, static_cast<int> (reader->numChannels));

    juce::AudioBuffer<float> buffer (channels, samplesToRead);
    buffer.clear();

    if (! reader->read (&buffer, 0, samplesToRead, 0, true, true))
    {
        result.error = "Could not decode the track.";
        return result;
    }

    result.sampleRate = reader->sampleRate;
    result.durationSeconds = static_cast<double> (reader->lengthInSamples) / reader->sampleRate;
    result.bpm = estimateBpm (buffer, reader->sampleRate);
    result.key = estimateKey (buffer, reader->sampleRate);
    result.ok = result.bpm > 0.0 && result.key.isNotEmpty();

    if (! result.ok)
        result.error = "Track loaded, but analysis confidence was too low.";

    return result;
}

double TrackAnalyzer::estimateBpm (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    auto mono = makeMono (buffer);
    if (mono.empty() || sampleRate <= 0.0)
        return 0.0;

    constexpr double envelopeRate = 200.0;
    const int blockSize = juce::jmax (1, static_cast<int> (std::round (sampleRate / envelopeRate)));
    const int envelopeSize = static_cast<int> (mono.size()) / blockSize;

    if (envelopeSize < 400)
        return 0.0;

    std::vector<float> envelope (static_cast<size_t> (envelopeSize), 0.0f);
    for (int block = 0; block < envelopeSize; ++block)
    {
        double sum = 0.0;
        const int start = block * blockSize;
        for (int i = 0; i < blockSize; ++i)
        {
            const auto x = mono[static_cast<size_t> (start + i)];
            sum += static_cast<double> (x) * x;
        }
        envelope[static_cast<size_t> (block)] = static_cast<float> (std::sqrt (sum / blockSize));
    }

    std::vector<float> novelty (envelope.size(), 0.0f);
    double noveltyMean = 0.0;
    for (size_t i = 1; i < envelope.size(); ++i)
    {
        novelty[i] = juce::jmax (0.0f, envelope[i] - envelope[i - 1]);
        noveltyMean += novelty[i];
    }
    noveltyMean /= static_cast<double> (novelty.size());

    for (auto& v : novelty)
        v = juce::jmax (0.0f, v - static_cast<float> (noveltyMean * 0.55));

    double bestScore = -1.0;
    int bestBpm = 0;
    std::array<double, 201> scores {};

    for (int bpm = 60; bpm <= 200; ++bpm)
    {
        const int lag = juce::jmax (1, static_cast<int> (std::round (envelopeRate * 60.0 / bpm)));
        double score = 0.0;
        for (int i = lag; i < envelopeSize; ++i)
            score += static_cast<double> (novelty[static_cast<size_t> (i)]) * novelty[static_cast<size_t> (i - lag)];

        scores[static_cast<size_t> (bpm)] = score;
        if (score > bestScore)
        {
            bestScore = score;
            bestBpm = bpm;
        }
    }

    // Prefer the double-time interpretation only when its correlation is nearly as strong.
    if (bestBpm > 0 && bestBpm < 100 && bestBpm * 2 <= 200)
    {
        const auto doubled = scores[static_cast<size_t> (bestBpm * 2)];
        if (doubled >= bestScore * 0.82)
            bestBpm *= 2;
    }

    return static_cast<double> (bestBpm);
}

juce::String TrackAnalyzer::estimateKey (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    auto mono = makeMono (buffer);
    if (mono.size() < 8192 || sampleRate <= 0.0)
        return {};

    constexpr int fftOrder = 12;
    constexpr int fftSize = 1 << fftOrder;
    constexpr int frames = 28;

    juce::dsp::FFT fft (fftOrder);
    juce::dsp::WindowingFunction<float> window (fftSize, juce::dsp::WindowingFunction<float>::hann, true);
    std::array<double, 12> chroma {};
    std::vector<float> fftData (static_cast<size_t> (fftSize * 2), 0.0f);

    const auto usable = static_cast<int> (mono.size()) - fftSize;
    for (int frameIndex = 0; frameIndex < frames; ++frameIndex)
    {
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        const int start = juce::jlimit (0, juce::jmax (0, usable), (usable * (frameIndex + 1)) / (frames + 1));

        for (int i = 0; i < fftSize; ++i)
            fftData[static_cast<size_t> (i)] = mono[static_cast<size_t> (start + i)];

        window.multiplyWithWindowingTable (fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data());

        for (int bin = 1; bin < fftSize / 2; ++bin)
        {
            const double frequency = static_cast<double> (bin) * sampleRate / fftSize;
            if (frequency < 45.0 || frequency > 5000.0)
                continue;

            const double midi = 69.0 + 12.0 * std::log2 (frequency / 440.0);
            const int pitchClass = ((static_cast<int> (std::lround (midi)) % 12) + 12) % 12;
            const double magnitude = std::log1p (static_cast<double> (fftData[static_cast<size_t> (bin)]));
            chroma[static_cast<size_t> (pitchClass)] += magnitude;
        }
    }

    constexpr std::array<double, 12> major { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    constexpr std::array<double, 12> minor { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    constexpr std::array<const char*, 12> noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    double bestScore = -std::numeric_limits<double>::infinity();
    int bestRoot = 0;
    bool bestIsMinor = false;

    for (int root = 0; root < 12; ++root)
    {
        for (int mode = 0; mode < 2; ++mode)
        {
            const auto& profile = mode == 0 ? major : minor;
            double score = 0.0;
            for (int degree = 0; degree < 12; ++degree)
                score += chroma[static_cast<size_t> ((degree + root) % 12)] * profile[static_cast<size_t> (degree)];

            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestIsMinor = mode == 1;
            }
        }
    }

    return juce::String (noteNames[static_cast<size_t> (bestRoot)]) + (bestIsMinor ? " Minor" : " Major");
}
