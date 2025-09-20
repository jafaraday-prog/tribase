#pragma once

#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"
#include "shared/ui/TriBaseStyle.h"
#include "shared/ui/XenoLookAndFeel.h"

class TriBaseInstrumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TriBaseInstrumentAudioProcessorEditor (TriBaseInstrumentAudioProcessor&);
    ~TriBaseInstrumentAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TriBaseInstrumentAudioProcessor& audioProcessor;
    std::unique_ptr<XenoLookAndFeel> xenoLAF = nullptr;

    bool isOpaque() const { return true; } // enable fast bg fill

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseInstrumentAudioProcessorEditor)
};
