#pragma once

#include <JuceHeader.h>

class LookaheadDetector
{
public:
    void prepare (double newSampleRate, int newMaxBlock);
    void reset();

    void setLookaheadMs (float ms);
    void setModeRMS (bool rmsMode);
    void setFilter (int type, float f1, float f2);

    const float* processSidechain (const float* const* sc, int numChannels, int numSamples);

private:
    void resizeBuffers();
    void updateFilters();
    void updateSmoothing();

    double sampleRate { 44100.0 };
    int maxBlock { 512 };

    bool rms { true };
    int filtType { 0 };
    float lookaheadMs { 2.0f };
    float fLo { 30.0f };
    float fHi { 120.0f };

    float smoothingCoeff { 1.0f };
    float envelopeState { 0.0f };

    juce::AudioBuffer<float> envBuf;
    juce::AudioBuffer<float> scMono;

    juce::dsp::IIR::Filter<float> hpf1;
    juce::dsp::IIR::Filter<float> hpf2;
    juce::dsp::IIR::Filter<float> bpf1;
    juce::dsp::IIR::Filter<float> bpf2;

    juce::dsp::ProcessSpec spec { 44100.0, static_cast<juce::uint32> (512), 1u };
};
