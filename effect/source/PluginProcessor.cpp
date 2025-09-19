#include "PluginProcessor.h"
#include "PluginEditor.h"

TriBaseBassManagerAudioProcessor::TriBaseBassManagerAudioProcessor()
    : juce::AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withInput("Sidechain", juce::AudioChannelSet::stereo(), true, true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void TriBaseBassManagerAudioProcessor::prepareToPlay(double, int)
{
}

void TriBaseBassManagerAudioProcessor::releaseResources()
{
}

bool TriBaseBassManagerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainInput = layouts.getMainInputChannelSet();
    const auto mainOutput = layouts.getMainOutputChannelSet();

    const auto isValidMainLayout =
        (mainInput == juce::AudioChannelSet::mono() || mainInput == juce::AudioChannelSet::stereo()) && mainInput == mainOutput;

    if (!isValidMainLayout)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sidechainLayout = layouts.inputBuses[1];
        if (sidechainLayout != juce::AudioChannelSet::stereo())
            return false;
    }

    return true;
}

void TriBaseBassManagerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    copyMainInputToOutput(buffer);

    if (getBusCount(true) > 1)
    {
        auto& sidechainBuffer = getBusBuffer(buffer, true, 1);
        juce::ignoreUnused(sidechainBuffer);
    }
}

void TriBaseBassManagerAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    copyMainInputToOutput(buffer);

    if (getBusCount(true) > 1)
    {
        auto& sidechainBuffer = getBusBuffer(buffer, true, 1);
        juce::ignoreUnused(sidechainBuffer);
    }
}

juce::AudioProcessorEditor* TriBaseBassManagerAudioProcessor::createEditor()
{
    return new TriBaseBassManagerAudioProcessorEditor(*this);
}

bool TriBaseBassManagerAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String TriBaseBassManagerAudioProcessor::getName() const
{
    return "TriBase Bass Manager";
}

bool TriBaseBassManagerAudioProcessor::acceptsMidi() const
{
    return false;
}

bool TriBaseBassManagerAudioProcessor::producesMidi() const
{
    return false;
}

bool TriBaseBassManagerAudioProcessor::isMidiEffect() const
{
    return false;
}

double TriBaseBassManagerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TriBaseBassManagerAudioProcessor::getNumPrograms()
{
    return 1;
}

int TriBaseBassManagerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TriBaseBassManagerAudioProcessor::setCurrentProgram(int)
{
}

const juce::String TriBaseBassManagerAudioProcessor::getProgramName(int)
{
    return "Default";
}

void TriBaseBassManagerAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void TriBaseBassManagerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void TriBaseBassManagerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

template <typename FloatType>
void TriBaseBassManagerAudioProcessor::copyMainInputToOutput(juce::AudioBuffer<FloatType>& buffer)
{
    auto& input = getBusBuffer(buffer, true, 0);
    auto& output = getBusBuffer(buffer, false, 0);

    if (&input == &output)
        return;

    const auto numSamples = output.getNumSamples();
    const auto numInputChannels = input.getNumChannels();
    const auto numOutputChannels = output.getNumChannels();

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (channel < numInputChannels)
            output.copyFrom(channel, 0, input, channel, 0, numSamples);
        else
            output.clear(channel, 0, numSamples);
    }
}

template void TriBaseBassManagerAudioProcessor::copyMainInputToOutput<float>(juce::AudioBuffer<float>&);
template void TriBaseBassManagerAudioProcessor::copyMainInputToOutput<double>(juce::AudioBuffer<double>&);
