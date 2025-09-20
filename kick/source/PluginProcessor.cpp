#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>
#include <algorithm>

namespace
{
    constexpr const char* parameterIds[] = {
        "clickLevel",
        "bodyStartHz",
        "bodyEndHz",
        "sweepSemis",
        "bodyTimeMs",
        "bodyCurve",
        "toneHz",
        "driveDb",
        "tailLevel",
        "tailDecayMs",
        "outputDb",
        "noteTune",
        "tuneOffsetSemis",
        "velToLevel",
        "glideMs"
    };

    static_assert (std::size (parameterIds) == TriBaseKickAudioProcessor::parameterCount, "Parameter count mismatch");
}

TriBaseKickAudioProcessor::TriBaseKickAudioProcessor()
    : juce::AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "params", createParameterLayout())
{
    for (size_t i = 0; i < rawParams.size(); ++i)
        rawParams[i] = state.getRawParameterValue (parameterIds[i]);
}

juce::AudioProcessorValueTreeState::ParameterLayout TriBaseKickAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[clickLevel], "Click Level", juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[bodyStartHz], "Body Start Hz", juce::NormalisableRange<float> (50.0f, 3000.0f, 0.0f, 0.4f), 120.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[bodyEndHz], "Body End Hz", juce::NormalisableRange<float> (20.0f, 2000.0f, 0.0f, 0.4f), 50.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[sweepSemis], "Sweep ±Semis", juce::NormalisableRange<float> (-48.0f, 48.0f, 1.0f), 24.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[bodyTimeMs], "Body Time", juce::NormalisableRange<float> (5.0f, 200.0f), 60.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[bodyCurve], "Body Curve", -1.0f, 1.0f, 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[toneHz], "Tone Hz", juce::NormalisableRange<float> (200.0f, 5000.0f, 0.0f, 0.4f), 1500.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[driveDb], "Drive", juce::NormalisableRange<float> (0.0f, 24.0f), 6.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[tailLevel], "Tail Level", juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[tailDecayMs], "Tail Decay", juce::NormalisableRange<float> (20.0f, 800.0f), 180.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[outputDb], "Output", juce::NormalisableRange<float> (-12.0f, 6.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        parameterIds[noteTune], "Note Tune", true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[tuneOffsetSemis], "Tune Offset", juce::NormalisableRange<float> (-24.0f, 24.0f, 1.0f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[velToLevel], "Vel to Level", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        parameterIds[glideMs], "Glide", juce::NormalisableRange<float> (0.0f, 60.0f), 0.0f));

    return { params.begin(), params.end() };
}

void TriBaseKickAudioProcessor::prepareToPlay (double sampleRate, int)
{
    juce::ScopedNoDenormals noDenormals;
    voice.prepare (sampleRate);
    voice.setTargetParameters (makeTargetParams());
    outputPeak.store (0.0f);
    lastNoteNumber.store (-1);
    uiNote.store (-1);
    uiNoteHz.store (0.0);

    scopeBuffer.setSize (1, SCOPE_CAP);
    scopeBuffer.clear();
    scopeFifo = std::make_unique<juce::AbstractFifo> (SCOPE_CAP);
}

void TriBaseKickAudioProcessor::releaseResources()
{
    scopeFifo.reset();
}

bool TriBaseKickAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    const auto& output = layouts.getMainOutputChannelSet();
    return output == juce::AudioChannelSet::stereo() || output == juce::AudioChannelSet::mono();
}

void TriBaseKickAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal (buffer, midiMessages);
}

void TriBaseKickAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal (buffer, midiMessages);
}

juce::AudioProcessorEditor* TriBaseKickAudioProcessor::createEditor()
{
    return new TriBaseKickAudioProcessorEditor (*this);
}

void TriBaseKickAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TriBaseKickAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        state.replaceState (juce::ValueTree::fromXml (*xml));
}

TriBaseKickAudioProcessor::KickParams TriBaseKickAudioProcessor::makeTargetParams() const
{
    KickParams params;
    params.clickLevel   = rawParams[clickLevel]->load();
    params.bodyStartHz  = rawParams[bodyStartHz]->load();
    params.bodyEndHz    = rawParams[bodyEndHz]->load();
    params.bodyTimeSec  = juce::jmax (0.001, static_cast<double> (rawParams[bodyTimeMs]->load()) / 1000.0);
    params.bodyCurve    = rawParams[bodyCurve]->load();
    params.toneHz       = rawParams[toneHz]->load();
    params.driveGain    = juce::Decibels::decibelsToGain (rawParams[driveDb]->load());
    params.tailLevel    = rawParams[tailLevel]->load();
    params.tailDecaySec = juce::jmax (0.02, static_cast<double> (rawParams[tailDecayMs]->load()) / 1000.0);
    params.outputGain   = juce::Decibels::decibelsToGain (rawParams[outputDb]->load());
    return params;
}

float TriBaseKickAudioProcessor::getAndResetPeak()
{
    return outputPeak.exchange (0.0f);
}

void TriBaseKickAudioProcessor::KickVoice::prepare (double newSampleRate)
{
    sampleRate = juce::jmax (1.0, newSampleRate);
    const double smoothingTime = 0.02; // 20 ms
    smoothingAlpha = 1.0 - std::exp (-1.0 / (smoothingTime * sampleRate));
    active = false;
    toneState = 0.0;
    clickLP = 0.0;
}

void TriBaseKickAudioProcessor::KickVoice::setTargetParameters (const KickParams& newTarget)
{
    target = newTarget;
}

void TriBaseKickAudioProcessor::KickVoice::trigger (const KickParams& params, double vel)
{
    target = params;
    current = params;

    velocity = juce::jlimit (0.0, 2.0, vel);
    time = 0.0;
    bodyPhase = 0.0;
    tailPhase = 0.0;
    tailEnv = params.tailLevel;
    clickTime = 0.0;
    toneState = 0.0;
    clickLP = 0.0;
    active = true;
}

double TriBaseKickAudioProcessor::KickVoice::applyCurve (double t, double curve)
{
    t = juce::jlimit (0.0, 1.0, t);
    if (curve < 0.0)
    {
        const double k = 1.0 + (-curve * 4.0);
        return 1.0 - std::pow (1.0 - t, k);
    }

    if (curve > 0.0)
    {
        const double k = 1.0 + (curve * 4.0);
        return std::pow (t, k);
    }

    return t;
}

double TriBaseKickAudioProcessor::KickVoice::renderSample()
{
    if (! active)
        return 0.0;

    const double dt = 1.0 / sampleRate;

    const auto smooth = [this] (double& value, double targetValue) noexcept
    {
        value += smoothingAlpha * (targetValue - value);
    };

    smooth (current.clickLevel, target.clickLevel);
    smooth (current.bodyStartHz, target.bodyStartHz);
    smooth (current.bodyEndHz, target.bodyEndHz);
    smooth (current.bodyTimeSec, target.bodyTimeSec);
    smooth (current.bodyCurve, target.bodyCurve);
    smooth (current.toneHz, target.toneHz);
    smooth (current.driveGain, target.driveGain);
    smooth (current.tailLevel, target.tailLevel);
    smooth (current.tailDecaySec, target.tailDecaySec);
    smooth (current.outputGain, target.outputGain);

    double sample = 0.0;

    const double clickDuration = 0.003 + current.clickLevel * 0.005;

    if (clickTime < clickDuration)
    {
        const double noise = (random.nextDouble() * 2.0) - 1.0;
        clickLP += 0.15 * (noise - clickLP);
        const double hp = noise - clickLP;
        sample += hp * current.clickLevel * 0.6;
        clickTime += dt;
    }

    if (current.bodyTimeSec > 0.0)
    {
        const double prog = juce::jlimit (0.0, 1.0, time / current.bodyTimeSec);
        const double shaped = applyCurve (prog, current.bodyCurve);
        const double freq = juce::jlimit (20.0, 4000.0, juce::jmap (shaped, 0.0, 1.0, current.bodyStartHz, current.bodyEndHz));
        bodyPhase += juce::MathConstants<double>::twoPi * freq * dt;
        if (bodyPhase >= juce::MathConstants<double>::twoPi)
            bodyPhase -= juce::MathConstants<double>::twoPi;

        const double env = std::exp (-prog * 5.0);
        sample += std::sin (bodyPhase) * env;
    }

    if (current.tailLevel > 0.0 && current.tailDecaySec > 0.0)
    {
        tailPhase += juce::MathConstants<double>::twoPi * current.bodyEndHz * dt;
        if (tailPhase >= juce::MathConstants<double>::twoPi)
            tailPhase -= juce::MathConstants<double>::twoPi;

        const double tailCoeff = std::exp (-dt / current.tailDecaySec);
        sample += std::sin (tailPhase) * tailEnv * current.tailLevel;
        tailEnv *= tailCoeff;
    }

    const double cutoff = juce::jlimit (100.0, 10000.0, current.toneHz);
    const double toneCoeff = std::exp (-juce::MathConstants<double>::twoPi * cutoff * dt);
    toneState = toneCoeff * toneState + (1.0 - toneCoeff) * sample;

    const double driveGain = juce::jmax (1.0, current.driveGain);
    const double driven = std::tanh (toneState * driveGain);
    const double norm = std::tanh (driveGain);
    const double processed = norm > 0.0 ? driven / norm : driven;

    double out = processed * current.outputGain * velocity;
    out = juce::jlimit (-1.2, 1.2, out);

    time += dt;

    if (time > current.bodyTimeSec + (current.tailDecaySec * 2.5) && std::abs (out) < 1.0e-4)
        active = false;

    return out;
}



template <typename FloatType>
void TriBaseKickAudioProcessor::processBlockInternal (juce::AudioBuffer<FloatType>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    const double sweepSemisVal = rawParams[sweepSemis]->load();
    const bool noteTuneEnabled = rawParams[noteTune]->load() >= 0.5f;
    const double tuneOffset = rawParams[tuneOffsetSemis]->load();
    const double velToLevelVal = juce::jlimit (0.0, 1.0, static_cast<double> (rawParams[velToLevel]->load()));

    auto baseParams = makeTargetParams();
    auto currentParams = baseParams;

    const auto applyPitchFromNote = [&] (int noteNumber, KickParams& params) -> double
    {
        double endHz = params.bodyEndHz;
        double startHz = params.bodyStartHz;

        if (noteTuneEnabled && noteNumber >= 0)
        {
            const double noteHz = 440.0 * std::pow (2.0, ((static_cast<double> (noteNumber) - 69.0) + tuneOffset) / 12.0);
            endHz = juce::jlimit (20.0, 2000.0, noteHz);
            startHz = endHz * std::pow (2.0, sweepSemisVal / 12.0);
        }

        params.bodyStartHz = juce::jlimit (20.0, 4000.0, startHz);
        params.bodyEndHz = juce::jlimit (20.0, 4000.0, endHz);
        return params.bodyEndHz;
    };

    const int previousNote = lastNoteNumber.load (std::memory_order_relaxed);
    double displayedEndHz = applyPitchFromNote (previousNote, currentParams);
    voice.setTargetParameters (currentParams);

    for (const auto metadata : midiMessages)
    {
        const auto& message = metadata.getMessage();
        if (message.isNoteOn())
        {
            const int noteNumber = message.getNoteNumber();
            lastNoteNumber.store (noteNumber, std::memory_order_relaxed);

            auto triggeredParams = makeTargetParams();
            displayedEndHz = applyPitchFromNote (noteNumber, triggeredParams);
            voice.setTargetParameters (triggeredParams);

            const double velNorm = juce::jlimit (0.0, 1.0, static_cast<double> (message.getFloatVelocity()));
            double velocityScale = 1.0 + (velNorm - 1.0) * velToLevelVal;
            velocityScale = juce::jlimit (0.0, 1.0, velocityScale);

            voice.trigger (triggeredParams, velocityScale);
            currentParams = triggeredParams;

            uiNote.store (noteNumber, std::memory_order_relaxed);
            uiNoteHz.store (displayedEndHz, std::memory_order_relaxed);
        }
    }

    midiMessages.clear();
    voice.setTargetParameters (currentParams);

    float blockPeak = 0.0f;

    auto* firstChannel = buffer.getWritePointer (0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const double value = voice.renderSample();
        const FloatType outSample = static_cast<FloatType> (value);

        firstChannel[sample] = outSample;

        const float absValue = static_cast<float> (std::abs (value));
        blockPeak = juce::jmax (blockPeak, absValue);
    }

    if (auto* fifo = scopeFifo.get())
    {
        const int freeSpace = fifo->getFreeSpace();
        const int samplesToWrite = juce::jmin (numSamples, freeSpace);

        if (samplesToWrite > 0)
        {
            int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
            fifo->prepareToWrite (samplesToWrite, start1, size1, start2, size2);

            const int sourceOffset = numSamples - samplesToWrite;
            const int totalChannels = buffer.getNumChannels();

            const auto writeRegion = [&] (int start, int size, int offset)
            {
                if (size <= 0)
                    return;

                auto* dest = scopeBuffer.getWritePointer (0, start);
                const auto* src0 = buffer.getReadPointer (0) + offset;
                const auto* src1 = totalChannels > 1 ? buffer.getReadPointer (1) + offset : nullptr;

                for (int i = 0; i < size; ++i)
                {
                    FloatType sample = src0[i];
                    if (src1 != nullptr)
                        sample = static_cast<FloatType> (0.5) * (sample + src1[i]);

                    dest[i] = static_cast<float> (sample);
                }
            };

            writeRegion (start1, size1, sourceOffset);
            writeRegion (start2, size2, sourceOffset + size1);

            fifo->finishedWrite (size1 + size2);
        }
    }

    for (int channel = 1; channel < totalNumOutputChannels; ++channel)
        buffer.copyFrom (channel, 0, buffer, 0, 0, numSamples);

    auto previous = outputPeak.load (std::memory_order_relaxed);
    const float updated = (blockPeak > previous) ? blockPeak : previous * 0.9f;
    outputPeak.store (updated, std::memory_order_relaxed);
}

bool TriBaseKickAudioProcessor::readScopeBlock (float* dst, int num)
{
    if (scopeFifo == nullptr || dst == nullptr || num <= 0)
        return false;

    auto* fifo = scopeFifo.get();
    int available = fifo->getNumReady();

    if (available <= 0)
        return false;

    const int toDiscard = juce::jmax (0, available - num);
    if (toDiscard > 0)
    {
        int discardStart1 = 0, discardSize1 = 0, discardStart2 = 0, discardSize2 = 0;
        fifo->prepareToRead (toDiscard, discardStart1, discardSize1, discardStart2, discardSize2);
        fifo->finishedRead (discardSize1 + discardSize2);
        available -= (discardSize1 + discardSize2);
    }

    const int toRead = juce::jmin (available, num);
    if (toRead <= 0)
        return false;

    int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
    fifo->prepareToRead (toRead, start1, size1, start2, size2);

    const auto copyRegion = [&] (int start, int size, int offset)
    {
        if (size <= 0)
            return;

        const float* src = scopeBuffer.getReadPointer (0, start);
        std::copy (src, src + size, dst + offset);
    };

    copyRegion (start1, size1, 0);
    copyRegion (start2, size2, size1);

    fifo->finishedRead (size1 + size2);

    if (toRead < num)
        std::fill (dst + toRead, dst + num, 0.0f);

    return true;
}


// Explicit instantiations
template void TriBaseKickAudioProcessor::processBlockInternal (juce::AudioBuffer<float>&, juce::MidiBuffer&);
template void TriBaseKickAudioProcessor::processBlockInternal (juce::AudioBuffer<double>&, juce::MidiBuffer&);

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TriBaseKickAudioProcessor();
}
