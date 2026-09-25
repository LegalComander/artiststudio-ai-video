#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::NormalisableRange<float> skewed (float min, float max, float centre)
{
    juce::NormalisableRange<float> r (min, max);
    r.setSkewForCentre (centre);
    return r;
}

float readInterpolated (const std::vector<float>& ring, float read)
{
    const int size = (int) ring.size();
    if (size <= 1)
        return 0.0f;

    while (read < 0.0f)
        read += (float) size;
    while (read >= (float) size)
        read -= (float) size;

    const int i0 = (int) read;
    const int i1 = (i0 + 1) % size;
    const float frac = read - (float) i0;
    return ring[(size_t) i0] + frac * (ring[(size_t) i1] - ring[(size_t) i0]);
}

float lfoShapeValue (int shape, double phase)
{
    phase -= std::floor (phase);
    switch (shape)
    {
        case 1: return 1.0f - 4.0f * std::abs ((float) phase - 0.5f); // triangle
        case 2: return phase < 0.5 ? 1.0f : -1.0f;                   // square
        case 3: return (float) (phase * 2.0 - 1.0);                  // saw
        default: return std::sin ((float) phase * juce::MathConstants<float>::twoPi);
    }
}
}

NeonRackProcessor::NeonRackProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "NEONRACK_STATE", makeLayout())
{
    for (int i = 0; i < numRackSlots; ++i)
        rack[(size_t) i].store ((int) Module::empty);

    rack[0].store ((int) Module::filter);
    rack[1].store ((int) Module::rift);
    rack[2].store ((int) Module::aura);
    rack[3].store ((int) Module::prismDelay);
    rack[4].store ((int) Module::orbitEQ);
    storeRackToState();
}

bool NeonRackProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void NeonRackProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    filter.reset();
    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    chorus.reset();
    chorus.prepare (spec);

    phaser.reset();
    phaser.prepare (spec);

    reverb.reset();
    reverb.prepare (spec);

    compressor.reset();
    compressor.prepare (spec);

    eqLow.reset();
    eqLow.prepare (spec);
    eqFocus.reset();
    eqFocus.prepare (spec);
    eqAir.reset();
    eqAir.prepare (spec);

    auraLP = { 0.0f, 0.0f };
    delayL.assign ((size_t) std::ceil (sampleRate * 5.0), 0.0f);
    delayR.assign ((size_t) std::ceil (sampleRate * 5.0), 0.0f);
    delayWrite = 0;
    delayPhase = 0.0f;
    globalLfoPhase = 0.0;
    globalLfoSample = 0.0f;
    lfoVisual.store (0.0f);
    outputMeter.store (0.0f);
    dryBuffer.setSize (2, samplesPerBlock, false, true, true);
    moduleDryBuffer.setSize (2, samplesPerBlock, false, true, true);
}

float NeonRackProcessor::value (const char* id) const
{
    if (auto* p = state.getRawParameterValue (id))
        return p->load();
    return 0.0f;
}

bool NeonRackProcessor::moduleEnabled (Module module) const
{
    const auto id = enabledParameter (module);
    if (id.isEmpty())
        return false;

    if (auto* p = state.getRawParameterValue (id))
        return p->load() >= 0.5f;
    return false;
}

NeonRackProcessor::Module NeonRackProcessor::getSlot (int slot) const noexcept
{
    if (! juce::isPositiveAndBelow (slot, numRackSlots))
        return Module::empty;
    return (Module) rack[(size_t) slot].load();
}

void NeonRackProcessor::setSlot (int slot, Module module)
{
    if (! juce::isPositiveAndBelow (slot, numRackSlots))
        return;

    if (module != Module::empty)
    {
        for (int i = 0; i < numRackSlots; ++i)
            if (i != slot && getSlot (i) == module)
                rack[(size_t) i].store ((int) Module::empty);
    }

    rack[(size_t) slot].store ((int) module);
    storeRackToState();
}

void NeonRackProcessor::removeSlot (int slot)
{
    setSlot (slot, Module::empty);
}

void NeonRackProcessor::moveSlot (int from, int to)
{
    if (! juce::isPositiveAndBelow (from, numRackSlots)
        || ! juce::isPositiveAndBelow (to, numRackSlots)
        || from == to)
        return;

    const auto moving = rack[(size_t) from].load();
    if (from < to)
        for (int i = from; i < to; ++i)
            rack[(size_t) i].store (rack[(size_t) i + 1].load());
    else
        for (int i = from; i > to; --i)
            rack[(size_t) i].store (rack[(size_t) i - 1].load());

    rack[(size_t) to].store (moving);
    storeRackToState();
}

void NeonRackProcessor::storeRackToState()
{
    for (int i = 0; i < numRackSlots; ++i)
        state.state.setProperty ("rackSlot" + juce::String (i), rack[(size_t) i].load(), nullptr);
}

void NeonRackProcessor::restoreRackFromState()
{
    for (int i = 0; i < numRackSlots; ++i)
    {
        const int fallback = i < 5 ? i + 1 : 0;
        const int raw = (int) state.state.getProperty ("rackSlot" + juce::String (i), fallback);
        rack[(size_t) i].store (juce::jlimit (0, (int) Module::pulseComp, raw));
    }
}

void NeonRackProcessor::updateHostTempo()
{
    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (auto bpm = position->getBpm())
            {
                if (*bpm > 10.0 && *bpm < 999.0)
                    hostBpm.store (*bpm);
            }
        }
    }
}

float NeonRackProcessor::syncedMilliseconds (int divisionIndex) const
{
    static constexpr std::array<double, 8> beats { 4.0, 2.0, 1.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 1.0 / 6.0 };
    const int index = juce::jlimit (0, (int) beats.size() - 1, divisionIndex);
    const double bpm = juce::jlimit (20.0, 400.0, hostBpm.load());
    return (float) ((60000.0 / bpm) * beats[(size_t) index]);
}

float NeonRackProcessor::syncedRateHz (int divisionIndex) const
{
    return 1000.0f / juce::jmax (1.0f, syncedMilliseconds (divisionIndex));
}

void NeonRackProcessor::updateGlobalLfo (int samples)
{
    if (value ("lfoOn") < 0.5f || sr <= 0.0)
    {
        globalLfoSample = 0.0f;
        lfoVisual.store (0.0f);
        return;
    }

    const bool sync = value ("lfoSync") >= 0.5f;
    const auto division = (int) std::lround (value ("lfoDivision"));
    const float rate = sync ? syncedRateHz (division) : value ("lfoRate");
    const auto shape = (int) std::lround (value ("lfoShape"));

    globalLfoSample = lfoShapeValue (shape, globalLfoPhase);
    lfoVisual.store (globalLfoSample);

    globalLfoPhase += ((double) rate * (double) samples) / sr;
    globalLfoPhase -= std::floor (globalLfoPhase);
}

float NeonRackProcessor::activeLfoForTarget (int targetIndex) const noexcept
{
    if (value ("lfoOn") < 0.5f)
        return 0.0f;

    const auto selectedTarget = (int) std::lround (value ("lfoTarget"));
    if (selectedTarget != targetIndex)
        return 0.0f;

    return globalLfoSample * value ("lfoDepth");
}

void NeonRackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0)
        return;

    updateHostTempo();
    updateGlobalLfo (samples);

    if (dryBuffer.getNumSamples() < samples || dryBuffer.getNumChannels() < channels)
        dryBuffer.setSize (channels, samples, false, false, true);
    if (moduleDryBuffer.getNumSamples() < samples || moduleDryBuffer.getNumChannels() < channels)
        moduleDryBuffer.setSize (channels, samples, false, false, true);

    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, samples);

    for (int slot = 0; slot < numRackSlots; ++slot)
    {
        const auto module = getSlot (slot);
        if (module != Module::empty && moduleEnabled (module))
            processModule (module, buffer);
    }

    const float mix = value ("globalMix");
    const float out = dbToGain (value ("output"));
    float peak = 0.0f;

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        const auto* dry = dryBuffer.getReadPointer (ch);
        for (int i = 0; i < samples; ++i)
        {
            wet[i] = (dry[i] + (wet[i] - dry[i]) * mix) * out;
            peak = juce::jmax (peak, std::abs (wet[i]));
        }
    }

    outputMeter.store (juce::jlimit (0.0f, 1.0f, peak));

    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, samples);
}

void NeonRackProcessor::processModule (Module module, juce::AudioBuffer<float>& buffer)
{
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float macroDrive = value ("macroDrive");
    const float macroSpace = value ("macroSpace");
    const float macroMotion = value ("macroMotion");
    const float macroTone = value ("macroTone");

    if (module == Module::filter)
    {
        const float lfo = activeLfoForTarget (0);
        const float cutoff = juce::jlimit (40.0f, 20000.0f,
                                          value ("filterCutoff") * std::pow (2.0f, macroTone * 1.5f + lfo * 2.0f));
        filter.setCutoffFrequency (cutoff);
        filter.setResonance (juce::jlimit (0.1f, 1.35f, value ("filterRes") + macroMotion * 0.22f));
        const float drive = dbToGain (value ("filterDrive") + macroDrive * 12.0f);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < samples; ++i)
                d[i] = std::tanh (d[i] * drive);
        }
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
        return;
    }

    if (module == Module::rift)
    {
        const float drive = dbToGain (value ("riftDrive") + macroDrive * 18.0f);
        const float fold = juce::jlimit (0.0f, 1.0f, value ("riftFold") + macroDrive * 0.25f);
        const float mix = value ("riftMix");
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < samples; ++i)
            {
                const float dry = d[i];
                const float x = dry * drive;
                const float soft = std::tanh (x);
                const float folded = std::sin (x * (1.0f + 4.0f * fold));
                const float wet = soft + (folded - soft) * fold;
                d[i] = dry + (wet - dry) * mix;
            }
        }
        return;
    }

    if (module == Module::aura)
    {
        const float heat = dbToGain (value ("auraHeat") + macroDrive * 10.0f);
        const float warmth = juce::jlimit (0.0f, 1.0f, value ("auraWarmth") - macroTone * 0.2f);
        const float airDb = juce::jlimit (-6.0f, 9.0f, value ("auraAir") + macroTone * 4.0f);
        const float air = juce::Decibels::decibelsToGain (airDb) - 1.0f;
        const float cutoff = juce::jmap (warmth, 18000.0f, 3500.0f);
        const float alpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / (float) sr);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < samples; ++i)
            {
                const float x = std::tanh (d[i] * heat + 0.08f * warmth);
                auraLP[(size_t) ch] += alpha * (x - auraLP[(size_t) ch]);
                const float hi = x - auraLP[(size_t) ch];
                d[i] = auraLP[(size_t) ch] + hi * (1.0f + air);
            }
        }
        return;
    }

    if (module == Module::chorus)
    {
        const bool sync = value ("chorusSync") >= 0.5f;
        const int division = (int) std::lround (value ("chorusDivision"));
        const float baseRate = sync ? syncedRateHz (division) : value ("chorusRate");
        chorus.setRate (juce::jlimit (0.03f, 8.0f, baseRate * (1.0f + macroMotion * 1.8f)));
        chorus.setDepth (juce::jlimit (0.0f, 1.0f, value ("chorusDepth") + macroMotion * 0.45f));
        chorus.setCentreDelay (value ("chorusDelay"));
        chorus.setFeedback (0.08f + macroSpace * 0.08f);
        chorus.setMix (juce::jlimit (0.0f, 1.0f, value ("chorusMix") + macroSpace * 0.25f));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
        return;
    }

    if (module == Module::prismDelay && ! delayL.empty())
    {
        const bool sync = value ("delaySync") >= 0.5f;
        const int division = (int) std::lround (value ("delayDivision"));
        float timeMs = sync ? syncedMilliseconds (division) : value ("delayTime");
        timeMs *= juce::jlimit (0.35f, 1.65f, 1.0f + activeLfoForTarget (1) * 0.45f);
        timeMs = juce::jlimit (20.0f, 4500.0f, timeMs);

        const float timeSamples = timeMs * 0.001f * (float) sr;
        const float feedback = juce::jlimit (0.0f, 0.96f, value ("delayFeedback") + macroSpace * 0.20f);
        const float mix = juce::jlimit (0.0f, 1.0f, value ("delayMix") + macroSpace * 0.34f);
        const float spread = juce::jlimit (0.0f, 1.0f, value ("delaySpread") + macroMotion * 0.30f);
        const int size = (int) delayL.size();
        const float phaseInc = (0.11f + macroMotion * 0.24f) / (float) sr;

        for (int i = 0; i < samples; ++i)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * delayPhase);
            delayPhase += phaseInc;
            if (delayPhase >= 1.0f)
                delayPhase -= 1.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                auto& ring = ch == 0 ? delayL : delayR;
                const float stereo = (ch == 0 ? -1.0f : 1.0f) * spread * timeSamples * 0.08f;
                const float mod = lfo * spread * 0.003f * (float) sr * (ch == 0 ? 1.0f : -1.0f);
                const float wet = readInterpolated (ring, (float) delayWrite - timeSamples - stereo - mod);
                const float dry = d[i];
                ring[(size_t) delayWrite] = dry + std::tanh (wet * 1.15f) * feedback;
                d[i] = dry + (wet - dry) * mix;
            }
            delayWrite = (delayWrite + 1) % size;
        }
        return;
    }

    if (module == Module::orbitEQ)
    {
        const float toneLfo = activeLfoForTarget (3);
        const float air = juce::jlimit (-12.0f, 12.0f, value ("eqAir") + macroTone * 7.0f + toneLfo * 5.0f);
        const float focus = juce::jlimit (-12.0f, 12.0f, value ("eqFocus") + macroTone * 2.0f + toneLfo * 2.0f);
        eqLow.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
            sr, value ("eqLowFreq"), 0.7071f, dbToGain (value ("eqLow")));
        eqFocus.state = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, value ("eqFocusFreq"), value ("eqFocusQ"), dbToGain (focus));
        eqAir.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            sr, 9500.0, 0.7071f, dbToGain (air));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        eqLow.process (ctx);
        eqFocus.process (ctx);
        eqAir.process (ctx);
        return;
    }

    if (module == Module::photonPhaser)
    {
        const bool sync = value ("phaserSync") >= 0.5f;
        const int division = (int) std::lround (value ("phaserDivision"));
        const float baseRate = sync ? syncedRateHz (division) : value ("phaserRate");
        const float centre = juce::jlimit (80.0f, 5000.0f,
                                          value ("phaserCentre") * std::pow (2.0f, activeLfoForTarget (2) * 1.5f));
        phaser.setRate (juce::jlimit (0.03f, 8.0f, baseRate * (1.0f + macroMotion * 2.0f)));
        phaser.setDepth (juce::jlimit (0.0f, 1.0f, value ("phaserDepth") + macroMotion * 0.35f));
        phaser.setCentreFrequency (centre);
        phaser.setFeedback (value ("phaserFeedback"));
        phaser.setMix (juce::jlimit (0.0f, 1.0f, value ("phaserMix") + macroSpace * 0.15f));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        phaser.process (ctx);
        return;
    }

    if (module == Module::spaceReverb)
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.0f, 1.0f, value ("reverbSize") + macroSpace * 0.25f);
        rp.damping = value ("reverbDamping");
        rp.width = value ("reverbWidth");
        const float wet = juce::jlimit (0.0f, 1.0f, value ("reverbMix") + macroSpace * 0.45f);
        rp.wetLevel = wet;
        rp.dryLevel = 1.0f - wet * 0.65f;
        rp.freezeMode = value ("reverbFreeze") >= 0.5f ? 1.0f : 0.0f;
        reverb.setParameters (rp);
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        reverb.process (ctx);
        return;
    }

    if (module == Module::pulseComp)
    {
        for (int ch = 0; ch < channels; ++ch)
            moduleDryBuffer.copyFrom (ch, 0, buffer, ch, 0, samples);

        compressor.setThreshold (value ("compThreshold"));
        compressor.setRatio (value ("compRatio"));
        compressor.setAttack (value ("compAttack"));
        compressor.setRelease (value ("compRelease"));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        compressor.process (ctx);

        const float makeup = dbToGain (value ("compMakeup") + macroDrive * 2.0f);
        const float mix = value ("compMix");
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            const auto* dry = moduleDryBuffer.getReadPointer (ch);
            for (int i = 0; i < samples; ++i)
                wet[i] = dry[i] + (wet[i] * makeup - dry[i]) * mix;
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout NeonRackProcessor::makeLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout p;

    p.add (std::make_unique<F> (juce::ParameterID { "globalMix", 1 }, "Global Mix", 0.0f, 1.0f, 1.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", -18.0f, 6.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "macroDrive", 1 }, "Macro Drive", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "macroSpace", 1 }, "Macro Space", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "macroMotion", 1 }, "Macro Motion", 0.0f, 1.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "macroTone", 1 }, "Macro Tone", -1.0f, 1.0f, 0.0f));

    p.add (std::make_unique<B> (juce::ParameterID { "lfoOn", 1 }, "LFO On", true));
    p.add (std::make_unique<B> (juce::ParameterID { "lfoSync", 1 }, "LFO Sync", true));
    p.add (std::make_unique<F> (juce::ParameterID { "lfoRate", 1 }, "LFO Rate", skewed (0.03f, 12.0f, 1.0f), 1.0f));
    p.add (std::make_unique<C> (juce::ParameterID { "lfoDivision", 1 }, "LFO Division", tempoDivisionNames(), 3));
    p.add (std::make_unique<F> (juce::ParameterID { "lfoDepth", 1 }, "LFO Depth", 0.0f, 1.0f, 0.35f));
    p.add (std::make_unique<C> (juce::ParameterID { "lfoShape", 1 }, "LFO Shape",
                                juce::StringArray { "SINE", "TRIANGLE", "SQUARE", "SAW" }, 0));
    p.add (std::make_unique<C> (juce::ParameterID { "lfoTarget", 1 }, "LFO Target", lfoTargetNames(), 0));

    p.add (std::make_unique<B> (juce::ParameterID { "filterOn", 1 }, "Filter On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "filterCutoff", 1 }, "Cutoff", skewed (40.0f, 20000.0f, 1800.0f), 12000.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "filterRes", 1 }, "Resonance", 0.1f, 1.35f, 0.45f));
    p.add (std::make_unique<F> (juce::ParameterID { "filterDrive", 1 }, "Drive", 0.0f, 18.0f, 0.0f));

    p.add (std::make_unique<B> (juce::ParameterID { "riftOn", 1 }, "RIFT On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "riftDrive", 1 }, "RIFT Drive", 0.0f, 36.0f, 10.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "riftFold", 1 }, "RIFT Fold", 0.0f, 1.0f, 0.24f));
    p.add (std::make_unique<F> (juce::ParameterID { "riftMix", 1 }, "RIFT Mix", 0.0f, 1.0f, 0.7f));

    p.add (std::make_unique<B> (juce::ParameterID { "auraOn", 1 }, "AURA On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "auraHeat", 1 }, "Heat", 0.0f, 24.0f, 5.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "auraWarmth", 1 }, "Warmth", 0.0f, 1.0f, 0.35f));
    p.add (std::make_unique<F> (juce::ParameterID { "auraAir", 1 }, "Air", -6.0f, 6.0f, 0.0f));

    p.add (std::make_unique<B> (juce::ParameterID { "chorusOn", 1 }, "Chorus On", true));
    p.add (std::make_unique<B> (juce::ParameterID { "chorusSync", 1 }, "Chorus Sync", false));
    p.add (std::make_unique<C> (juce::ParameterID { "chorusDivision", 1 }, "Chorus Division", tempoDivisionNames(), 2));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusRate", 1 }, "Rate", 0.05f, 5.0f, 0.4f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusDepth", 1 }, "Depth", 0.0f, 1.0f, 0.35f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusDelay", 1 }, "Width", 2.0f, 28.0f, 11.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusMix", 1 }, "Chorus Mix", 0.0f, 1.0f, 0.3f));

    p.add (std::make_unique<B> (juce::ParameterID { "delayOn", 1 }, "PRISM Delay On", true));
    p.add (std::make_unique<B> (juce::ParameterID { "delaySync", 1 }, "Delay Sync", false));
    p.add (std::make_unique<C> (juce::ParameterID { "delayDivision", 1 }, "Delay Division", tempoDivisionNames(), 3));
    p.add (std::make_unique<F> (juce::ParameterID { "delayTime", 1 }, "Time", skewed (20.0f, 1400.0f, 350.0f), 360.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "delayFeedback", 1 }, "Feedback", 0.0f, 0.92f, 0.42f));
    p.add (std::make_unique<F> (juce::ParameterID { "delayMix", 1 }, "Delay Mix", 0.0f, 1.0f, 0.28f));
    p.add (std::make_unique<F> (juce::ParameterID { "delaySpread", 1 }, "Spread", 0.0f, 1.0f, 0.52f));

    p.add (std::make_unique<B> (juce::ParameterID { "eqOn", 1 }, "ORBIT EQ On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "eqLow", 1 }, "Low", -12.0f, 12.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "eqLowFreq", 1 }, "Low Frequency", skewed (40.0f, 500.0f, 120.0f), 110.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "eqFocus", 1 }, "Focus", -12.0f, 12.0f, 0.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "eqFocusFreq", 1 }, "Focus Frequency", skewed (180.0f, 8000.0f, 1600.0f), 1600.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "eqFocusQ", 1 }, "Focus Q", 0.3f, 4.0f, 0.9f));
    p.add (std::make_unique<F> (juce::ParameterID { "eqAir", 1 }, "Air", -12.0f, 12.0f, 0.0f));

    p.add (std::make_unique<B> (juce::ParameterID { "phaserOn", 1 }, "PHOTON Phaser On", true));
    p.add (std::make_unique<B> (juce::ParameterID { "phaserSync", 1 }, "Phaser Sync", false));
    p.add (std::make_unique<C> (juce::ParameterID { "phaserDivision", 1 }, "Phaser Division", tempoDivisionNames(), 2));
    p.add (std::make_unique<F> (juce::ParameterID { "phaserRate", 1 }, "Phaser Rate", 0.05f, 5.0f, 0.28f));
    p.add (std::make_unique<F> (juce::ParameterID { "phaserDepth", 1 }, "Phaser Depth", 0.0f, 1.0f, 0.62f));
    p.add (std::make_unique<F> (juce::ParameterID { "phaserCentre", 1 }, "Phaser Centre", skewed (80.0f, 5000.0f, 900.0f), 850.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "phaserFeedback", 1 }, "Phaser Feedback", -0.8f, 0.8f, 0.18f));
    p.add (std::make_unique<F> (juce::ParameterID { "phaserMix", 1 }, "Phaser Mix", 0.0f, 1.0f, 0.35f));

    p.add (std::make_unique<B> (juce::ParameterID { "reverbOn", 1 }, "SPACE Reverb On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "reverbSize", 1 }, "Size", 0.0f, 1.0f, 0.58f));
    p.add (std::make_unique<F> (juce::ParameterID { "reverbDamping", 1 }, "Damping", 0.0f, 1.0f, 0.38f));
    p.add (std::make_unique<F> (juce::ParameterID { "reverbWidth", 1 }, "Width", 0.0f, 1.0f, 0.92f));
    p.add (std::make_unique<F> (juce::ParameterID { "reverbMix", 1 }, "Reverb Mix", 0.0f, 1.0f, 0.25f));
    p.add (std::make_unique<B> (juce::ParameterID { "reverbFreeze", 1 }, "Freeze", false));

    p.add (std::make_unique<B> (juce::ParameterID { "compOn", 1 }, "PULSE Compressor On", true));
    p.add (std::make_unique<F> (juce::ParameterID { "compThreshold", 1 }, "Threshold", -48.0f, 0.0f, -18.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "compRatio", 1 }, "Ratio", 1.0f, 20.0f, 4.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "compAttack", 1 }, "Attack", 0.2f, 100.0f, 10.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "compRelease", 1 }, "Release", 10.0f, 600.0f, 120.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "compMakeup", 1 }, "Makeup", 0.0f, 18.0f, 2.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "compMix", 1 }, "Comp Mix", 0.0f, 1.0f, 1.0f));

    return p;
}

juce::StringArray NeonRackProcessor::tempoDivisionNames()
{
    return { "1/1", "1/2", "1/4", "1/8", "1/8D", "1/8T", "1/16", "1/16T" };
}

juce::StringArray NeonRackProcessor::lfoTargetNames()
{
    return { "FILTER", "DELAY", "PHASER", "TONE" };
}

juce::String NeonRackProcessor::moduleName (Module m)
{
    switch (m)
    {
        case Module::filter: return "NEON FILTER";
        case Module::rift: return "RIFT DISTORTION";
        case Module::aura: return "AURA SATURATOR";
        case Module::chorus: return "CHORUS MATRIX";
        case Module::prismDelay: return "PRISM DELAY";
        case Module::orbitEQ: return "ORBIT EQ";
        case Module::photonPhaser: return "PHOTON PHASER";
        case Module::spaceReverb: return "SPACE REVERB";
        case Module::pulseComp: return "PULSE COMPRESSOR";
        default: return "+ ADD FX";
    }
}

juce::Colour NeonRackProcessor::moduleColour (Module m)
{
    switch (m)
    {
        case Module::filter: return juce::Colour (0xff48e9ff);
        case Module::rift: return juce::Colour (0xffff4b9b);
        case Module::aura: return juce::Colour (0xffffb14c);
        case Module::chorus: return juce::Colour (0xff9f7cff);
        case Module::prismDelay: return juce::Colour (0xff4c8dff);
        case Module::orbitEQ: return juce::Colour (0xff5ef0b1);
        case Module::photonPhaser: return juce::Colour (0xffd076ff);
        case Module::spaceReverb: return juce::Colour (0xff6d8cff);
        case Module::pulseComp: return juce::Colour (0xffff6e6e);
        default: return juce::Colour (0xff52738a);
    }
}

juce::String NeonRackProcessor::enabledParameter (Module m)
{
    switch (m)
    {
        case Module::filter: return "filterOn";
        case Module::rift: return "riftOn";
        case Module::aura: return "auraOn";
        case Module::chorus: return "chorusOn";
        case Module::prismDelay: return "delayOn";
        case Module::orbitEQ: return "eqOn";
        case Module::photonPhaser: return "phaserOn";
        case Module::spaceReverb: return "reverbOn";
        case Module::pulseComp: return "compOn";
        default: return {};
    }
}

juce::StringArray NeonRackProcessor::moduleParameterIds (Module m)
{
    switch (m)
    {
        case Module::filter: return { "filterCutoff", "filterRes", "filterDrive" };
        case Module::rift: return { "riftDrive", "riftFold", "riftMix" };
        case Module::aura: return { "auraHeat", "auraWarmth", "auraAir" };
        case Module::chorus: return { "chorusSync", "chorusDivision", "chorusRate", "chorusDepth", "chorusDelay", "chorusMix" };
        case Module::prismDelay: return { "delaySync", "delayDivision", "delayTime", "delayFeedback", "delaySpread", "delayMix" };
        case Module::orbitEQ: return { "eqLow", "eqLowFreq", "eqFocus", "eqFocusFreq", "eqFocusQ", "eqAir" };
        case Module::photonPhaser: return { "phaserSync", "phaserDivision", "phaserRate", "phaserDepth", "phaserCentre", "phaserFeedback", "phaserMix" };
        case Module::spaceReverb: return { "reverbSize", "reverbDamping", "reverbWidth", "reverbMix", "reverbFreeze" };
        case Module::pulseComp: return { "compThreshold", "compRatio", "compAttack", "compRelease", "compMakeup", "compMix" };
        default: return {};
    }
}

juce::String NeonRackProcessor::parameterLabel (const juce::String& id)
{
    if (id == "filterCutoff") return "CUTOFF";
    if (id == "filterRes") return "RESONANCE";
    if (id == "filterDrive" || id == "riftDrive") return "DRIVE";
    if (id == "riftFold") return "FOLD";
    if (id.endsWithIgnoreCase ("Mix")) return "MIX";
    if (id == "auraHeat") return "HEAT";
    if (id == "auraWarmth") return "WARMTH";
    if (id == "auraAir" || id == "eqAir") return "AIR";
    if (id == "chorusRate" || id == "phaserRate") return "RATE";
    if (id == "chorusDepth" || id == "phaserDepth") return "DEPTH";
    if (id == "chorusDelay" || id == "reverbWidth") return "WIDTH";
    if (id == "delayTime") return "TIME";
    if (id == "delayFeedback" || id == "phaserFeedback") return "FEEDBACK";
    if (id == "delaySpread") return "SPREAD";
    if (id == "eqLow") return "LOW";
    if (id == "eqLowFreq") return "LOW FREQ";
    if (id == "eqFocus") return "FOCUS";
    if (id == "eqFocusFreq") return "FOCUS FREQ";
    if (id == "eqFocusQ") return "FOCUS Q";
    if (id == "phaserCentre") return "CENTRE";
    if (id == "reverbSize") return "SIZE";
    if (id == "reverbDamping") return "DAMPING";
    if (id == "reverbFreeze") return "FREEZE";
    if (id == "compThreshold") return "THRESHOLD";
    if (id == "compRatio") return "RATIO";
    if (id == "compAttack") return "ATTACK";
    if (id == "compRelease") return "RELEASE";
    if (id == "compMakeup") return "MAKEUP";
    if (id.endsWith ("Sync")) return "SYNC";
    if (id.endsWith ("Division")) return "DIVISION";
    return id.toUpperCase();
}

void NeonRackProcessor::setPlainParameter (const juce::String& id, float plainValue)
{
    if (auto* p = state.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void NeonRackProcessor::enableAllModules()
{
    for (int m = (int) Module::filter; m <= (int) Module::pulseComp; ++m)
        if (auto* p = state.getParameter (enabledParameter ((Module) m)))
            p->setValueNotifyingHost (1.0f);
}

juce::StringArray NeonRackProcessor::factoryPresetNames()
{
    return { "INIT / CLEAN", "DNB IMPACT", "VOCAL NEON", "BASS MELTDOWN", "DREAM SPACE", "MASTER GLOW" };
}

void NeonRackProcessor::applyFactoryPreset (int index)
{
    auto setRack = [this] (std::initializer_list<Module> modules)
    {
        int i = 0;
        for (auto m : modules)
        {
            if (i >= numRackSlots)
                break;
            rack[(size_t) i++].store ((int) m);
        }
        while (i < numRackSlots)
            rack[(size_t) i++].store ((int) Module::empty);
    };

    enableAllModules();
    setPlainParameter ("globalMix", 1.0f);
    setPlainParameter ("output", 0.0f);
    setPlainParameter ("macroDrive", 0.0f);
    setPlainParameter ("macroSpace", 0.0f);
    setPlainParameter ("macroMotion", 0.0f);
    setPlainParameter ("macroTone", 0.0f);
    setPlainParameter ("lfoDepth", 0.35f);

    switch (juce::jlimit (0, 5, index))
    {
        case 0:
            setRack ({ Module::filter, Module::aura, Module::chorus, Module::prismDelay, Module::orbitEQ });
            setPlainParameter ("filterCutoff", 16000.0f);
            setPlainParameter ("auraHeat", 3.0f);
            setPlainParameter ("chorusMix", 0.16f);
            setPlainParameter ("delayMix", 0.12f);
            break;
        case 1:
            setRack ({ Module::filter, Module::pulseComp, Module::rift, Module::aura, Module::orbitEQ });
            setPlainParameter ("compThreshold", -20.0f);
            setPlainParameter ("compRatio", 5.0f);
            setPlainParameter ("compAttack", 7.0f);
            setPlainParameter ("compRelease", 90.0f);
            setPlainParameter ("riftDrive", 13.0f);
            setPlainParameter ("riftMix", 0.42f);
            setPlainParameter ("macroDrive", 0.22f);
            break;
        case 2:
            setRack ({ Module::filter, Module::aura, Module::photonPhaser, Module::spaceReverb, Module::prismDelay, Module::orbitEQ });
            setPlainParameter ("filterCutoff", 14500.0f);
            setPlainParameter ("reverbMix", 0.22f);
            setPlainParameter ("delaySync", 1.0f);
            setPlainParameter ("delayDivision", 3.0f);
            setPlainParameter ("delayMix", 0.18f);
            setPlainParameter ("eqAir", 3.0f);
            setPlainParameter ("macroSpace", 0.18f);
            break;
        case 3:
            setRack ({ Module::rift, Module::filter, Module::aura, Module::pulseComp, Module::orbitEQ, Module::prismDelay });
            setPlainParameter ("riftDrive", 24.0f);
            setPlainParameter ("riftFold", 0.55f);
            setPlainParameter ("filterCutoff", 5200.0f);
            setPlainParameter ("compThreshold", -16.0f);
            setPlainParameter ("compRatio", 7.0f);
            setPlainParameter ("macroDrive", 0.34f);
            break;
        case 4:
            setRack ({ Module::chorus, Module::photonPhaser, Module::prismDelay, Module::spaceReverb, Module::orbitEQ, Module::aura });
            setPlainParameter ("chorusSync", 1.0f);
            setPlainParameter ("chorusDivision", 2.0f);
            setPlainParameter ("chorusMix", 0.38f);
            setPlainParameter ("delaySync", 1.0f);
            setPlainParameter ("delayDivision", 3.0f);
            setPlainParameter ("delayFeedback", 0.58f);
            setPlainParameter ("delayMix", 0.32f);
            setPlainParameter ("reverbSize", 0.82f);
            setPlainParameter ("reverbMix", 0.34f);
            setPlainParameter ("macroSpace", 0.30f);
            setPlainParameter ("macroMotion", 0.25f);
            break;
        case 5:
            setRack ({ Module::pulseComp, Module::aura, Module::orbitEQ });
            setPlainParameter ("compThreshold", -12.0f);
            setPlainParameter ("compRatio", 2.2f);
            setPlainParameter ("compAttack", 25.0f);
            setPlainParameter ("compRelease", 180.0f);
            setPlainParameter ("compMix", 0.72f);
            setPlainParameter ("auraHeat", 2.4f);
            setPlainParameter ("eqAir", 1.8f);
            setPlainParameter ("macroTone", 0.10f);
            break;
    }

    storeRackToState();
}

void NeonRackProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    storeRackToState();
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void NeonRackProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        state.replaceState (juce::ValueTree::fromXml (*xml));
        restoreRackFromState();
    }
}

juce::AudioProcessorEditor* NeonRackProcessor::createEditor()
{
    return new NeonRackEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeonRackProcessor();
}
