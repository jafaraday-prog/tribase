#pragma once

#include <JuceHeader.h>

class TriBaseBassManagerAudioProcessor;

class TriBaseBassManagerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TriBaseBassManagerAudioProcessorEditor(TriBaseBassManagerAudioProcessor&);
    ~TriBaseBassManagerAudioProcessorEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TriBaseBassManagerAudioProcessor& audioProcessor;
    juce::Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriBaseBassManagerAudioProcessorEditor)
};
