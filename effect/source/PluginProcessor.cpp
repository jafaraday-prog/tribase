#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <array>
#include <vector>

bool TriBaseAudioProcessor::supportsDoublePrecisionProcessing() const
{
    return true;
}

bool TriBaseAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getChannelSet (true, 0);
    const auto mainOut = layouts.getChannelSet (false, 0);

    const auto isMonoOrStereo = [] (const juce::AudioChannelSet& set)
    {
        return set == juce::AudioChannelSet::mono() || set == juce::AudioChannelSet::stereo();
    };

    if (! isMonoOrStereo (mainIn) || ! isMonoOrStereo (mainOut))
        return false;

    if (mainIn.size() != mainOut.size())
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechain = layouts.getChannelSet (true, 1);
        if (sidechain != juce::AudioChannelSet::stereo() && sidechain != juce::AudioChannelSet::disabled())
            return false;
    }

    return true;
}

void TriBaseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateHz = sampleRate;
    maxBlock = samplesPerBlock;

    detector.prepare (sampleRate, samplesPerBlock);

    const int totalOutputs = getTotalNumOutputChannels();
    delayLanes.resize (totalOutputs);

    const int maxDelay = juce::jmax (1, juce::roundToInt (sampleRate * 0.25));
    for (auto& delay : delayLanes)
    {
        delay.setMaximumDelayInSamples (maxDelay);
        delay.reset();
    }

    const auto look = apvts.getRawParameterValue ("lookaheadMs")->load();
    const auto ftype = static_cast<int> (apvts.getRawParameterValue ("scFilterType")->load());
    const auto flo = apvts.getRawParameterValue ("scFilterLoHz")->load();
    const auto fhi = apvts.getRawParameterValue ("scFilterHiHz")->load();
    const auto rms = static_cast<int> (apvts.getRawParameterValue ("detectorMode")->load()) == 0;

    detector.setModeRMS (rms);
    detector.setFilter (ftype, flo, fhi);
    detector.setLookaheadMs (look);

    prevLookaheadMs = look;

    const double sr = sampleRate;
    latencySamples = sr > 0.0 ? static_cast<int> (std::lround (sr * (look / 1000.0f))) : 0;
    setLatencySamples (latencySamples);

    for (auto& delay : delayLanes)
        delay.setDelay (static_cast<float> (latencySamples));

    refreshParams();
}

void TriBaseAudioProcessor::releaseResources()
{
}

int TriBaseAudioProcessor::getLatencySamples() const
{
    return latencySamples;
}

void TriBaseAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    applyParamUpdatesIfChanged();
    juce::ignoreUnused (midiMessages);

    auto inMain = getBusBuffer (buffer, true, 0);
    auto outMain = getBusBuffer (buffer, false, 0);

    outMain.makeCopyOf (inMain, true);

    for (int ch = getMainBusNumOutputChannels(); ch < outMain.getNumChannels(); ++ch)
        outMain.clear (ch, 0, outMain.getNumSamples());

    if (delayLanes.size() != static_cast<size_t> (outMain.getNumChannels()))
    {
        const int maxDelay = juce::jmax (1, juce::roundToInt (getSampleRate() * 0.25));
        delayLanes.resize (outMain.getNumChannels());
        for (auto& delay : delayLanes)
        {
            delay.setMaximumDelayInSamples (maxDelay);
            delay.reset();
            delay.setDelay (static_cast<float> (latencySamples));
        }
    }

    float blockPeakDb = -60.0f;

    if (hasSidechainEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        const int numSamples = sc.getNumSamples();
        const int nCh = juce::jmin (sc.getNumChannels(), 8);

        std::array<const float*, 8> scPtrs { { nullptr } };

        for (int c = 0; c < nCh; ++c)
            scPtrs[c] = sc.getReadPointer (c);

        const auto* env = detector.processSidechain (scPtrs.data(), nCh, numSamples);

        float guiPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float linear = juce::jlimit (1.0e-6f, 1.0f, std::abs (env[i]));
            guiPeak = std::max (guiPeak, linear);
            blockPeakDb = std::max (blockPeakDb, juce::Decibels::gainToDecibels (linear, -60.0f));
        }

        scLevel.store (juce::jlimit (0.0f, 1.0f, guiPeak));
    }
    else
    {
        scLevel.store (0.0f);
    }

    if (! std::isfinite (blockPeakDb))
        blockPeakDb = -60.0f;

    meterScDb.store (juce::jlimit (-60.0f, 0.0f, blockPeakDb));

    const float sr = static_cast<float> (getSampleRate());
    const float attackCoeff = (sr > 0.0f) ? std::exp (-1.0f / (0.001f * atkMs * sr)) : 0.0f;
    const float releaseCoeff = (sr > 0.0f) ? std::exp (-1.0f / (0.001f * relMs * sr)) : 0.0f;

    const int numSamples = outMain.getNumSamples();
    const float wetMix = juce::jlimit (0.0f, 1.0f, mix);
    const float dryMix = 1.0f - wetMix;

    for (int n = 0; n < numSamples; ++n)
    {
        const float targetDb = blockPeakDb;

        if (targetDb > envDb)
            envDb = attackCoeff * envDb + (1.0f - attackCoeff) * targetDb;
        else
            envDb = releaseCoeff * envDb + (1.0f - releaseCoeff) * targetDb;

        grDb = computeGainDb (envDb);
        const float gain = juce::Decibels::decibelsToGain (grDb + makeupDb);

        for (int ch = 0; ch < outMain.getNumChannels(); ++ch)
        {
            auto* data = outMain.getWritePointer (ch);
            const float dry = data[n];
            const float delayed = delayLanes[(size_t) ch].popSample (0);
            delayLanes[(size_t) ch].pushSample (0, dry);
            const float wet = delayed * gain;
            data[n] = wet * wetMix + dry * dryMix;
        }
    }

    meterGrDb.store (juce::jlimit (-48.0f, 0.0f, grDb));
}

void TriBaseAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    applyParamUpdatesIfChanged();
    juce::ignoreUnused (midiMessages);

    auto inMain = getBusBuffer (buffer, true, 0);
    auto outMain = getBusBuffer (buffer, false, 0);

    outMain.makeCopyOf (inMain, true);

    for (int ch = getMainBusNumOutputChannels(); ch < outMain.getNumChannels(); ++ch)
        outMain.clear (ch, 0, outMain.getNumSamples());

    if (delayLanes.size() != static_cast<size_t> (outMain.getNumChannels()))
    {
        const int maxDelay = juce::jmax (1, juce::roundToInt (getSampleRate() * 0.25));
        delayLanes.resize (outMain.getNumChannels());
        for (auto& delay : delayLanes)
        {
            delay.setMaximumDelayInSamples (maxDelay);
            delay.reset();
            delay.setDelay (static_cast<float> (latencySamples));
        }
    }

    float blockPeakDb = -60.0f;

    if (hasSidechainEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        const int numSamples = sc.getNumSamples();
        const int nCh = juce::jmin (sc.getNumChannels(), 8);

        static thread_local std::array<std::vector<float>, 8> scTemp;
        std::array<const float*, 8> scPtrs { { nullptr } };

        for (int c = 0; c < nCh; ++c)
        {
            auto& tmp = scTemp[c];
            tmp.resize (static_cast<size_t> (numSamples));

            const auto* src = sc.getReadPointer (c);
            float* dst = tmp.data();

            for (int i = 0; i < numSamples; ++i)
                dst[i] = static_cast<float> (src[i]);

            scPtrs[c] = dst;
        }

        const auto* env = detector.processSidechain (scPtrs.data(), nCh, numSamples);

        float guiPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            const float linear = juce::jlimit (1.0e-6f, 1.0f, std::abs (env[i]));
            guiPeak = std::max (guiPeak, linear);
            blockPeakDb = std::max (blockPeakDb, juce::Decibels::gainToDecibels (linear, -60.0f));
        }

        scLevel.store (juce::jlimit (0.0f, 1.0f, guiPeak));
    }
    else
    {
        scLevel.store (0.0f);
    }

    if (! std::isfinite (blockPeakDb))
        blockPeakDb = -60.0f;

    meterScDb.store (juce::jlimit (-60.0f, 0.0f, blockPeakDb));

    const float sr = static_cast<float> (getSampleRate());
    const float attackCoeff = (sr > 0.0f) ? std::exp (-1.0f / (0.001f * atkMs * sr)) : 0.0f;
    const float releaseCoeff = (sr > 0.0f) ? std::exp (-1.0f / (0.001f * relMs * sr)) : 0.0f;

    const int numSamples = outMain.getNumSamples();
    const float wetMix = juce::jlimit (0.0f, 1.0f, mix);
    const float dryMix = 1.0f - wetMix;

    for (int n = 0; n < numSamples; ++n)
    {
        const float targetDb = blockPeakDb;

        if (targetDb > envDb)
            envDb = attackCoeff * envDb + (1.0f - attackCoeff) * targetDb;
        else
            envDb = releaseCoeff * envDb + (1.0f - releaseCoeff) * targetDb;

        grDb = computeGainDb (envDb);
        const float gain = juce::Decibels::decibelsToGain (grDb + makeupDb);

        for (int ch = 0; ch < outMain.getNumChannels(); ++ch)
        {
            auto* data = outMain.getWritePointer (ch);
            const double dryDouble = data[n];
            const float dry = static_cast<float> (dryDouble);
            const float delayed = delayLanes[(size_t) ch].popSample (0);
            delayLanes[(size_t) ch].pushSample (0, dry);
            const double wet = static_cast<double> (delayed * gain);
            data[n] = wet * static_cast<double> (wetMix) + dryDouble * static_cast<double> (dryMix);
        }
    }

    meterGrDb.store (juce::jlimit (-48.0f, 0.0f, grDb));
}

void TriBaseAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void TriBaseAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*state));
}

const juce::String TriBaseAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TriBaseAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TriBaseAudioProcessor::createEditor()
{
    return new TriBaseAudioProcessorEditor (*this);
}

bool TriBaseAudioProcessor::acceptsMidi() const
{
    return false;
}

bool TriBaseAudioProcessor::producesMidi() const
{
    return false;
}

bool TriBaseAudioProcessor::isMidiEffect() const
{
    return false;
}

double TriBaseAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TriBaseAudioProcessor::getNumPrograms()
{
    return 1;
}

int TriBaseAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TriBaseAudioProcessor::setCurrentProgram (int)
{
}

const juce::String TriBaseAudioProcessor::getProgramName (int)
{
    return {};
}

void TriBaseAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void TriBaseAudioProcessor::applyParamUpdatesIfChanged()
{
    const auto look = apvts.getRawParameterValue ("lookaheadMs")->load();

    if (std::abs (look - prevLookaheadMs) > 1.0e-3f)
    {
        detector.setLookaheadMs (look);
        prevLookaheadMs = look;

        const double sr = getSampleRate();
        latencySamples = sr > 0.0 ? static_cast<int> (std::lround (sr * (look / 1000.0f))) : 0;
        setLatencySamples (latencySamples);

        for (auto& delay : delayLanes)
            delay.setDelay (static_cast<float> (latencySamples));
    }

    const auto ftype = static_cast<int> (apvts.getRawParameterValue ("scFilterType")->load());
    const auto flo = apvts.getRawParameterValue ("scFilterLoHz")->load();
    const auto fhi = apvts.getRawParameterValue ("scFilterHiHz")->load();
    const auto rms = static_cast<int> (apvts.getRawParameterValue ("detectorMode")->load()) == 0;

    detector.setModeRMS (rms);
    detector.setFilter (ftype, flo, fhi);

    refreshParams();
}

void TriBaseAudioProcessor::refreshParams()
{
    threshDb = apvts.getRawParameterValue ("threshold")->load();
    ratio    = apvts.getRawParameterValue ("ratio")->load();
    atkMs    = apvts.getRawParameterValue ("attackMs")->load();
    relMs    = apvts.getRawParameterValue ("releaseMs")->load();
    depthDb  = apvts.getRawParameterValue ("depthDb")->load();
    mix      = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    makeupDb = apvts.getRawParameterValue ("makeupDb")->load();

    mix = juce::jlimit (0.0f, 1.0f, mix);
}

float TriBaseAudioProcessor::computeGainDb (float detectorDb)
{
    const float over = detectorDb - threshDb;
    if (over <= 0.0f)
        return 0.0f;

    const float reduction = over - (over / juce::jmax (1.0f, ratio));
    return -juce::jmin (depthDb, reduction);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TriBaseAudioProcessor();
}
