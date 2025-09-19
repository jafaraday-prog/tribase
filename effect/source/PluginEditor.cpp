#include "PluginEditor.h"
#include "PluginProcessor.h"

TriBaseBassManagerAudioProcessorEditor::TriBaseBassManagerAudioProcessorEditor(TriBaseBassManagerAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor), audioProcessor(processor)
{
    infoLabel.setText("TriBase Bass Manager\nExternal sidechain ready", juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(infoLabel);

    setSize(400, 240);
}

void TriBaseBassManagerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void TriBaseBassManagerAudioProcessorEditor::resized()
{
    infoLabel.setBounds(getLocalBounds());
}
