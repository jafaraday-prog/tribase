#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TriBaseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit TriBaseAudioProcessorEditor (TriBaseAudioProcessor&);
    ~TriBaseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    TriBaseAudioProcessor& processor;

    juce::Slider lookaheadSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lookaheadAttachment;

    float meterValue = 0.0f;
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseAudioProcessorEditor)
};
