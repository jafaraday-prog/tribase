#pragma once

#include <JuceHeader.h>
#include "SynthSound.h"

class SynthVoice : public juce::SynthesiserVoice
{
public:
    SynthVoice() = default;

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;
    void renderNextBlock(juce::AudioBuffer<double>& buffer, int startSample, int numSamples) override;

private:
    template <typename FloatType>
    void renderInternal(juce::AudioBuffer<FloatType>& buffer, int startSample, int numSamples);

    double currentAngle = 0.0;
    double angleDelta = 0.0;
    float level = 0.0f;
    float tailOff = 0.0f;
};
