#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <juce_dsp/juce_dsp.h>
#include "dsp/LookaheadDetector.h"

class TriBaseAudioProcessor : public juce::AudioProcessor
{
public:
    TriBaseAudioProcessor();
    ~TriBaseAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    bool supportsDoublePrecisionProcessing() const override;
    int getLatencySamples() const;
    const juce::String getName() const override;

    bool hasEditor() const override;
    juce::AudioProcessorEditor* createEditor() override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float> scLevel { 0.0f };
    std::atomic<float> meterScDb { -60.0f };
    std::atomic<float> meterGrDb { 0.0f };

    double sampleRateHz { 44100.0 };
    int maxBlock { 512 };
    int latencySamples { 0 };

    bool hasSidechainEnabled() const;

private:
    void applyParamUpdatesIfChanged();
    void refreshParams();
    float computeGainDb (float detectorDb);

    LookaheadDetector detector;
    float prevLookaheadMs { -1.0f };

    // user params cached
    float threshDb { -24.0f };
    float ratio { 4.0f };
    float atkMs { 5.0f };
    float relMs { 120.0f };
    float depthDb { 18.0f };
    float mix { 1.0f };
    float makeupDb { 0.0f };

    // smoothing
    float envDb { -96.0f };
    float grDb { 0.0f };

    // lookahead audio
    std::vector<juce::dsp::DelayLine<float>> delayLanes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseAudioProcessor)
};

inline TriBaseAudioProcessor::TriBaseAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                               .withInput ("Main In", juce::AudioChannelSet::stereo(), true)
                               .withOutput ("Main Out", juce::AudioChannelSet::stereo(), true)
                               .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

inline bool TriBaseAudioProcessor::hasSidechainEnabled() const
{
    if (auto* bus = getBus (true, 1))
        return bus->isEnabled();

    return false;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout TriBaseAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "lookaheadMs",
        "Lookahead (ms)",
        juce::NormalisableRange<float> (0.0f, 5.0f),
        2.0f));

    layout.add (std::make_unique<juce::AudioParameterChoice>(
        "detectorMode",
        "Detector Mode",
        juce::StringArray { "RMS", "Peak" },
        0));

    layout.add (std::make_unique<juce::AudioParameterChoice>(
        "scFilterType",
        "SC Filter Type",
        juce::StringArray { "Off", "HPF", "BP" },
        0));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "scFilterLoHz",
        "SC Filter Lo (Hz)",
        juce::NormalisableRange<float> (20.0f, 80.0f),
        30.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "scFilterHiHz",
        "SC Filter Hi (Hz)",
        juce::NormalisableRange<float> (80.0f, 150.0f),
        120.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "threshold",
        "Threshold (dB)",
        juce::NormalisableRange<float> (-60.0f, 0.0f),
        -24.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "ratio",
        "Ratio",
        juce::NormalisableRange<float> (1.0f, 20.0f),
        4.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "attackMs",
        "Attack (ms)",
        juce::NormalisableRange<float> (0.1f, 50.0f),
        5.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "releaseMs",
        "Release (ms)",
        juce::NormalisableRange<float> (10.0f, 500.0f),
        120.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "depthDb",
        "Depth (dB)",
        juce::NormalisableRange<float> (0.0f, 48.0f),
        18.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix (%)",
        juce::NormalisableRange<float> (0.0f, 100.0f),
        100.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat>(
        "makeupDb",
        "Makeup (dB)",
        juce::NormalisableRange<float> (-24.0f, 24.0f),
        0.0f));

    return layout;
}
