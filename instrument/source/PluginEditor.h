#pragma once

#include <JuceHeader.h>

class TriBaseInstrumentAudioProcessor;

class TriBaseInstrumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TriBaseInstrumentAudioProcessorEditor(TriBaseInstrumentAudioProcessor&);
    ~TriBaseInstrumentAudioProcessorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TriBaseInstrumentAudioProcessor& audioProcessor;
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriBaseInstrumentAudioProcessorEditor)
};
