#include "PluginEditor.h"
#include "PluginProcessor.h"

TriBaseInstrumentAudioProcessorEditor::TriBaseInstrumentAudioProcessorEditor(TriBaseInstrumentAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor), audioProcessor(processor)
{
    infoLabel.setText("TriBase Instrument\nPrototype synthesiser", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(infoLabel);

    setSize(400, 300);
}

void TriBaseInstrumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void TriBaseInstrumentAudioProcessorEditor::resized()
{
    infoLabel.setBounds(getLocalBounds());
}
