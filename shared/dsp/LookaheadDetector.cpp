#include "LookaheadDetector.h"

namespace
{
constexpr float kMaxLookaheadMs = 5.0f;
constexpr float kMinFreqHz = 10.0f;
}

void LookaheadDetector::prepare (double newSampleRate, int newMaxBlock)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    maxBlock   = juce::jmax (1, newMaxBlock);

    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (maxBlock);
    spec.numChannels      = 1u;

    hpf1.prepare (spec);
    hpf2.prepare (spec);
    bpf1.prepare (spec);
    bpf2.prepare (spec);

    resizeBuffers();
    reset();
    updateFilters();
    updateSmoothing();
}

void LookaheadDetector::reset()
{
    envelopeState = 0.0f;

    envBuf.clear();
    scMono.clear();

    hpf1.reset();
    hpf2.reset();
    bpf1.reset();
    bpf2.reset();
}

void LookaheadDetector::setLookaheadMs (float ms)
{
    lookaheadMs = juce::jlimit (0.0f, kMaxLookaheadMs, ms);
    updateSmoothing();
}

void LookaheadDetector::setModeRMS (bool rmsMode)
{
    rms = rmsMode;
}

void LookaheadDetector::setFilter (int type, float f1, float f2)
{
    filtType = juce::jlimit (0, 2, type);
    fLo      = juce::jmax (0.0f, f1);
    fHi      = juce::jmax (fLo, f2);

    updateFilters();
}

const float* LookaheadDetector::processSidechain (const float* const* sc, int numChannels, int numSamples)
{
    jassert (numSamples <= maxBlock);

    if (numSamples > scMono.getNumSamples())
        scMono.setSize (1, numSamples, false, false, true);

    if (numSamples > envBuf.getNumSamples())
        envBuf.setSize (1, numSamples, false, false, true);

    auto* mono = scMono.getWritePointer (0);

    if (sc == nullptr || numChannels <= 0)
    {
        juce::FloatVectorOperations::clear (mono, numSamples);
    }
    else
    {
        const float scale = 1.0f / juce::jmax (1, numChannels);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float sum = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
                sum += sc[ch][sample];

            mono[sample] = sum * scale;
        }
    }

    if (filtType == 1)
    {
        juce::dsp::AudioBlock<float> block (scMono);
        auto sub = block.getSubBlock (0, static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        hpf1.process (ctx);
        hpf2.process (ctx);
    }
    else if (filtType == 2)
    {
        juce::dsp::AudioBlock<float> block (scMono);
        auto sub = block.getSubBlock (0, static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        bpf1.process (ctx);
        bpf2.process (ctx);
    }

    auto* env = envBuf.getWritePointer (0);

    for (int i = 0; i < numSamples; ++i)
    {
        const float x = mono[i];
        const float magnitude = rms ? std::sqrt (x * x) : std::abs (x);

        if (smoothingCoeff >= 1.0f)
            envelopeState = magnitude;
        else
            envelopeState += smoothingCoeff * (magnitude - envelopeState);

        env[i] = envelopeState;
    }

    return envBuf.getReadPointer (0);
}

void LookaheadDetector::resizeBuffers()
{
    envBuf.setSize (1, maxBlock, false, false, true);
    scMono.setSize (1, maxBlock, false, false, true);
}

void LookaheadDetector::updateFilters()
{
    const float nyquist = static_cast<float> (sampleRate * 0.5);

    auto resetIfOff = [this]()
    {
        hpf1.reset();
        hpf2.reset();
        bpf1.reset();
        bpf2.reset();
    };

    if (filtType == 0)
    {
        resetIfOff();
        return;
    }

    if (filtType == 1)
    {
        const float freq = juce::jlimit (kMinFreqHz, nyquist, fLo);
        hpf1.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freq);
        hpf2.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, freq);
        hpf1.reset();
        hpf2.reset();
        return;
    }

    const float low  = juce::jlimit (kMinFreqHz, nyquist, fLo);
    const float high = juce::jlimit (low + 1.0f, nyquist, fHi);
    const float centre = std::sqrt (low * high);
    const float bandwidth = juce::jmax (1.0f, high - low);
    const float q = juce::jlimit (0.1f, 20.0f, centre / bandwidth);

    bpf1.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, centre, q);
    bpf2.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, centre, q);
    bpf1.reset();
    bpf2.reset();
}

void LookaheadDetector::updateSmoothing()
{
    const float clampedMs = juce::jlimit (0.0f, kMaxLookaheadMs, lookaheadMs);

    if (clampedMs <= 0.0f)
    {
        smoothingCoeff = 1.0f;
        return;
    }

    const double tauSeconds = static_cast<double> (clampedMs) * 0.001;
    const double alpha = 1.0 - std::exp (-1.0 / (tauSeconds * sampleRate));
    smoothingCoeff = static_cast<float> (juce::jlimit (0.0, 1.0, alpha));
}
