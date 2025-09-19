#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

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

    const auto* lookahead = apvts.getRawParameterValue ("lookaheadMs");
    const auto lookaheadMs = lookahead != nullptr ? lookahead->load() : 0.0f;

    latencySamples = static_cast<int> (std::round (sampleRateHz * (static_cast<double> (lookaheadMs) / 1000.0)));
    setLatencySamples (latencySamples);
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
    process (buffer, midiMessages);
}

void TriBaseAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    process (buffer, midiMessages);
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

template <typename SampleType>
void TriBaseAudioProcessor::process (juce::AudioBuffer<SampleType>& buffer, juce::MidiBuffer&)
{
    auto mainIn = getBusBuffer (buffer, true, 0);
    auto mainOut = getBusBuffer (buffer, false, 0);

    mainOut.makeCopyOf (mainIn);

    for (int ch = mainOut.getNumChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    if (hasSidechainEnabled() && getBusCount (true) > 1)
    {
        auto sidechainBuffer = getBusBuffer (buffer, true, 1);
        if (sidechainBuffer.getNumChannels() > 0)
        {
            double sumSquares = 0.0;
            for (int ch = 0; ch < sidechainBuffer.getNumChannels(); ++ch)
            {
                const auto* data = sidechainBuffer.getReadPointer (ch);
                for (int i = 0; i < sidechainBuffer.getNumSamples(); ++i)
                {
                    const auto sample = static_cast<double> (data[i]);
                    sumSquares += sample * sample;
                }
            }

            const auto totalSamples = static_cast<double> (sidechainBuffer.getNumChannels() * sidechainBuffer.getNumSamples());
            const auto rms = totalSamples > 0.0 ? std::sqrt (sumSquares / totalSamples) : 0.0;

            scLevel.store (juce::jlimit (0.0f, 1.0f, static_cast<float> (rms)));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TriBaseAudioProcessor();
}
