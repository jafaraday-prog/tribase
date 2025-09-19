#include "PluginEditor.h"

TriBaseAudioProcessorEditor::TriBaseAudioProcessorEditor (TriBaseAudioProcessor& proc)
    : juce::AudioProcessorEditor (&proc),
      processor (proc)
{
    setSize (360, 240);

    lookaheadSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lookaheadSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible (lookaheadSlider);

    lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "lookaheadMs", lookaheadSlider);

    startTimerHz (30);
}

TriBaseAudioProcessorEditor::~TriBaseAudioProcessorEditor() = default;

void TriBaseAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    g.setColour (juce::Colours::darkgrey);
    g.fillRect (meterBounds.toFloat());

    const auto clamped = juce::jlimit (0.0f, 1.0f, meterValue);
    const auto levelHeight = clamped * static_cast<float> (meterBounds.getHeight());

    auto levelRect = meterBounds.withY (meterBounds.getBottom() - static_cast<int> (levelHeight))
                                .withHeight (static_cast<int> (levelHeight));

    g.setColour (juce::Colours::limegreen);
    g.fillRect (levelRect.toFloat());
}

void TriBaseAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    meterBounds = area.removeFromRight (24);
    lookaheadSlider.setBounds (area);
}

void TriBaseAudioProcessorEditor::timerCallback()
{
    meterValue = processor.scLevel.load();
    repaint (meterBounds);
}
