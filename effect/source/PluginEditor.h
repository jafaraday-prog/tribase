#pragma once

#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"
#include "ui/XenoLookAndFeel.h"

class TriBaseAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    class Scope : public juce::Component
    {
    public:
        void setColours (juce::Colour sc, juce::Colour gr) { scColour = sc; grColour = gr; }

        void push (float scDb, float grDb)
        {
            scHistory.add (juce::jlimit (-60.0f, 0.0f, scDb));
            grHistory.add (juce::jlimit (-48.0f, 0.0f, grDb));
            trim();
            repaint();
        }

        void resized() override { trim(); }

        void paint (juce::Graphics& g) override;

    private:
        void trim()
        {
            const int maxPts = juce::jmax (getWidth(), 64);
            while (scHistory.size() > maxPts) scHistory.remove (0);
            while (grHistory.size() > maxPts) grHistory.remove (0);
        }

        juce::Array<float> scHistory, grHistory;
        juce::Colour scColour, grColour;
    };

    explicit TriBaseAudioProcessorEditor (TriBaseAudioProcessor&);
    ~TriBaseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void timerCallback() override;

    TriBaseAudioProcessor& processor;

    juce::Slider lookaheadSlider;
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider depthSlider;
    juce::Slider mixSlider;
    juce::Slider makeupSlider;

    std::unique_ptr<SliderAttachment> lookaheadAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment;
    std::unique_ptr<SliderAttachment> ratioAttachment;
    std::unique_ptr<SliderAttachment> attackAttachment;
    std::unique_ptr<SliderAttachment> releaseAttachment;
    std::unique_ptr<SliderAttachment> depthAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> makeupAttachment;

    std::unique_ptr<juce::Timer> uiTimer;
    Scope scope;

    float lastGrDb { 0.0f };
    juce::Rectangle<int> meterBounds;

    std::unique_ptr<XenoLookAndFeel> xenoLAF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseAudioProcessorEditor)
};
