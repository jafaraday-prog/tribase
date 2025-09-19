#include "SynthVoice.h"

#include <cmath>

bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

void SynthVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int)
{
    const auto sampleRate = getSampleRate();
    const auto frequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    currentAngle = 0.0;
    angleDelta = (juce::MathConstants<double>::twoPi * frequency) / juce::jmax(1.0, sampleRate);
    level = velocity;
    tailOff = 0.0f;
}

void SynthVoice::stopNote(float, bool allowTailOff)
{
    if (allowTailOff && angleDelta != 0.0)
    {
        tailOff = 1.0f;
    }
    else
    {
        clearCurrentNote();
        angleDelta = 0.0;
    }
}

void SynthVoice::pitchWheelMoved(int)
{
}

void SynthVoice::controllerMoved(int, int)
{
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    renderInternal(buffer, startSample, numSamples);
}

void SynthVoice::renderNextBlock(juce::AudioBuffer<double>& buffer, int startSample, int numSamples)
{
    renderInternal(buffer, startSample, numSamples);
}

template <typename FloatType>
void SynthVoice::renderInternal(juce::AudioBuffer<FloatType>& buffer, int startSample, int numSamples)
{
    if (angleDelta == 0.0)
        return;

    const auto numChannels = buffer.getNumChannels();

    while (numSamples > 0)
    {
        auto sample = static_cast<FloatType>(std::sin(currentAngle) * level);

        if (tailOff > 0.0f)
        {
            sample = static_cast<FloatType>(sample * tailOff);
            tailOff *= 0.995f;

            if (tailOff <= 0.001f)
            {
                clearCurrentNote();
                angleDelta = 0.0;
                break;
            }
        }

        for (int channel = 0; channel < numChannels; ++channel)
            buffer.addSample(channel, startSample, sample);

        currentAngle += angleDelta;
        ++startSample;
        --numSamples;
    }
}

template void SynthVoice::renderInternal<float>(juce::AudioBuffer<float>&, int, int);
template void SynthVoice::renderInternal<double>(juce::AudioBuffer<double>&, int, int);
