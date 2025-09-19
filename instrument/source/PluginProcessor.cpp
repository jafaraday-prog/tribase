#include "PluginProcessor.h"
#include "PluginEditor.h"

TriBaseInstrumentAudioProcessor::TriBaseInstrumentAudioProcessor()
    : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    synthesiser.clearVoices();

    constexpr auto numVoices = 8;
    for (int i = 0; i < numVoices; ++i)
        synthesiser.addVoice(new SynthVoice());

    synthesiser.clearSounds();
    synthesiser.addSound(new SynthSound());
    synthesiser.setNoteStealingEnabled(true);
}

void TriBaseInstrumentAudioProcessor::prepareToPlay(double sampleRate, int)
{
    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void TriBaseInstrumentAudioProcessor::releaseResources()
{
}

bool TriBaseInstrumentAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled())
        return false;

    const auto& outputLayout = layouts.getMainOutputChannelSet();
    return outputLayout == juce::AudioChannelSet::mono() || outputLayout == juce::AudioChannelSet::stereo();
}

void TriBaseInstrumentAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal(buffer, midiMessages);
}

void TriBaseInstrumentAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockInternal(buffer, midiMessages);
}

juce::AudioProcessorEditor* TriBaseInstrumentAudioProcessor::createEditor()
{
    return new TriBaseInstrumentAudioProcessorEditor(*this);
}

bool TriBaseInstrumentAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String TriBaseInstrumentAudioProcessor::getName() const
{
    return "TriBase Instrument";
}

bool TriBaseInstrumentAudioProcessor::acceptsMidi() const
{
    return true;
}

bool TriBaseInstrumentAudioProcessor::producesMidi() const
{
    return false;
}

bool TriBaseInstrumentAudioProcessor::isMidiEffect() const
{
    return false;
}

double TriBaseInstrumentAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TriBaseInstrumentAudioProcessor::getNumPrograms()
{
    return 1;
}

int TriBaseInstrumentAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TriBaseInstrumentAudioProcessor::setCurrentProgram(int)
{
}

const juce::String TriBaseInstrumentAudioProcessor::getProgramName(int)
{
    return "Default";
}

void TriBaseInstrumentAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void TriBaseInstrumentAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ignoreUnused(destData);
}

void TriBaseInstrumentAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::ignoreUnused(data, sizeInBytes);
}

template <typename FloatType>
void TriBaseInstrumentAudioProcessor::processBlockInternal(juce::AudioBuffer<FloatType>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

template void TriBaseInstrumentAudioProcessor::processBlockInternal<float>(juce::AudioBuffer<float>&, juce::MidiBuffer&);
template void TriBaseInstrumentAudioProcessor::processBlockInternal<double>(juce::AudioBuffer<double>&, juce::MidiBuffer&);

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TriBaseInstrumentAudioProcessor();
}
