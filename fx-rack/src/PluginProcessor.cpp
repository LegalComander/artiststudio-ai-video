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
    while (read < 0.0f) read += (float) size;
    while (read >= (float) size) read -= (float) size;
    const int i0 = (int) read;
    const int i1 = (i0 + 1) % size;
    const float frac = read - (float) i0;
    return ring[(size_t) i0] + frac * (ring[(size_t) i1] - ring[(size_t) i0]);
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

    eqLow.reset(); eqLow.prepare (spec);
    eqFocus.reset(); eqFocus.prepare (spec);
    eqAir.reset(); eqAir.prepare (spec);

    auraLP = { 0.0f, 0.0f };
    delayL.assign ((size_t) std::ceil (sampleRate * 2.5), 0.0f);
    delayR.assign ((size_t) std::ceil (sampleRate * 2.5), 0.0f);
    delayWrite = 0;
    delayPhase = 0.0f;
    dryBuffer.setSize (2, samplesPerBlock, false, true, true);
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
    if (id.isEmpty()) return false;
    if (auto* p = state.getRawParameterValue (id))
        return p->load() >= 0.5f;
    return false;
}

NeonRackProcessor::Module NeonRackProcessor::getSlot (int slot) const noexcept
{
    if (! juce::isPositiveAndBelow (slot, numRackSlots)) return Module::empty;
    return (Module) rack[(size_t) slot].load();
}

void NeonRackProcessor::setSlot (int slot, Module module)
{
    if (! juce::isPositiveAndBelow (slot, numRackSlots)) return;

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
    if (! juce::isPositiveAndBelow (from, numRackSlots) || ! juce::isPositiveAndBelow (to, numRackSlots) || from == to)
        return;
    const auto a = rack[(size_t) from].load();
    const auto b = rack[(size_t) to].load();
    rack[(size_t) from].store (b);
    rack[(size_t) to].store (a);
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
        const auto fallback = i < 5 ? i + 1 : 0;
        rack[(size_t) i].store ((int) state.state.getProperty ("rackSlot" + juce::String (i), fallback));
    }
}

void NeonRackProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels == 0) return;

    if (dryBuffer.getNumSamples() < samples || dryBuffer.getNumChannels() < channels)
        dryBuffer.setSize (channels, samples, false, false, true);

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
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* wet = buffer.getWritePointer (ch);
        const auto* dry = dryBuffer.getReadPointer (ch);
        for (int i = 0; i < samples; ++i)
            wet[i] = (dry[i] + (wet[i] - dry[i]) * mix) * out;
    }

    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, samples);
}

void NeonRackProcessor::processModule (Module module, juce::AudioBuffer<float>& buffer)
{
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    if (module == Module::filter)
    {
        filter.setCutoffFrequency (value ("filterCutoff"));
        filter.setResonance (value ("filterRes"));
        const float drive = dbToGain (value ("filterDrive"));
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            for (int i = 0; i < samples; ++i) d[i] = std::tanh (d[i] * drive);
        }
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
        return;
    }

    if (module == Module::rift)
    {
        const float drive = dbToGain (value ("riftDrive"));
        const float fold = value ("riftFold");
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
        const float heat = dbToGain (value ("auraHeat"));
        const float warmth = value ("auraWarmth");
        const float air = juce::Decibels::decibelsToGain (value ("auraAir")) - 1.0f;
        const float alpha = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * juce::jmap (warmth, 18000.0f, 3500.0f) / (float) sr);
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
        chorus.setRate (value ("chorusRate"));
        chorus.setDepth (value ("chorusDepth"));
        chorus.setCentreDelay (value ("chorusDelay"));
        chorus.setFeedback (0.08f);
        chorus.setMix (value ("chorusMix"));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
        return;
    }

    if (module == Module::prismDelay && ! delayL.empty())
    {
        const float timeSamples = value ("delayTime") * 0.001f * (float) sr;
        const float feedback = value ("delayFeedback");
        const float mix = value ("delayMix");
        const float spread = value ("delaySpread");
        const int size = (int) delayL.size();
        const float phaseInc = 0.11f / (float) sr;

        for (int i = 0; i < samples; ++i)
        {
            const float lfo = std::sin (juce::MathConstants<float>::twoPi * delayPhase);
            delayPhase += phaseInc;
            if (delayPhase >= 1.0f) delayPhase -= 1.0f;

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
        eqLow.state = juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, value ("eqLowFreq"), 0.7071f, dbToGain (value ("eqLow")));
        eqFocus.state = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, value ("eqFocusFreq"), value ("eqFocusQ"), dbToGain (value ("eqFocus")));
        eqAir.state = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 9500.0, 0.7071f, dbToGain (value ("eqAir")));
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        eqLow.process (ctx); eqFocus.process (ctx); eqAir.process (ctx);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout NeonRackProcessor::makeLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout p;

    p.add (std::make_unique<F> (juce::ParameterID { "globalMix", 1 }, "Global Mix", 0.0f, 1.0f, 1.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", -18.0f, 6.0f, 0.0f));

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
    p.add (std::make_unique<F> (juce::ParameterID { "chorusRate", 1 }, "Rate", 0.05f, 5.0f, 0.4f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusDepth", 1 }, "Depth", 0.0f, 1.0f, 0.35f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusDelay", 1 }, "Width", 2.0f, 28.0f, 11.0f));
    p.add (std::make_unique<F> (juce::ParameterID { "chorusMix", 1 }, "Chorus Mix", 0.0f, 1.0f, 0.3f));

    p.add (std::make_unique<B> (juce::ParameterID { "delayOn", 1 }, "PRISM Delay On", true));
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
    return p;
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
        case Module::chorus: return { "chorusRate", "chorusDepth", "chorusDelay", "chorusMix" };
        case Module::prismDelay: return { "delayTime", "delayFeedback", "delaySpread", "delayMix" };
        case Module::orbitEQ: return { "eqLow", "eqLowFreq", "eqFocus", "eqFocusFreq", "eqFocusQ", "eqAir" };
        default: return {};
    }
}

juce::String NeonRackProcessor::parameterLabel (const juce::String& id)
{
    if (id == "filterCutoff") return "CUTOFF";
    if (id == "filterRes") return "RESONANCE";
    if (id == "filterDrive" || id == "riftDrive") return "DRIVE";
    if (id == "riftFold") return "FOLD";
    if (id == "riftMix" || id == "chorusMix" || id == "delayMix") return "MIX";
    if (id == "auraHeat") return "HEAT";
    if (id == "auraWarmth") return "WARMTH";
    if (id == "auraAir" || id == "eqAir") return "AIR";
    if (id == "chorusRate") return "RATE";
    if (id == "chorusDepth") return "DEPTH";
    if (id == "chorusDelay") return "WIDTH";
    if (id == "delayTime") return "TIME";
    if (id == "delayFeedback") return "FEEDBACK";
    if (id == "delaySpread") return "SPREAD";
    if (id == "eqLow") return "LOW";
    if (id == "eqLowFreq") return "LOW FREQ";
    if (id == "eqFocus") return "FOCUS";
    if (id == "eqFocusFreq") return "FOCUS FREQ";
    if (id == "eqFocusQ") return "FOCUS Q";
    return id.toUpperCase();
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
