#include "PluginEditor.h"

namespace
{
constexpr juce::uint32 dark = 0xFF0A0E12;
constexpr juce::uint32 textColour = 0xFFAEEAFF;
}

TriBaseInstrumentAudioProcessorEditor::TriBaseInstrumentAudioProcessorEditor (TriBaseInstrumentAudioProcessor& processor)
    : juce::AudioProcessorEditor (&processor), audioProcessor (processor)
{
    xenoLAF = std::make_unique<XenoLookAndFeel>();
    setLookAndFeel (xenoLAF.get());
    setOpaque (true);
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (dark));
    setSize (720, 420);
}

TriBaseInstrumentAudioProcessorEditor::~TriBaseInstrumentAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    xenoLAF.reset();
}

void TriBaseInstrumentAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (dark));
    g.setColour (juce::Colour (textColour).withAlpha (0.8f));
    g.setFont (16.0f);
    g.drawFittedText ("TriBase Instrument\nPrototype synthesiser",
                      getLocalBounds().reduced (16),
                      juce::Justification::centred, 2);
}

void TriBaseInstrumentAudioProcessorEditor::resized()
{
}
