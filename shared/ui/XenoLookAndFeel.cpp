#include "TriBaseStyle.h"
#include "XenoLookAndFeel.h"
using namespace tribase::ui::tokens;
XenoLookAndFeel::XenoLookAndFeel() {
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour(bg0));
    setColour (juce::Label::textColourId,          juce::Colour(textPrimary));
    setColour (juce::Slider::thumbColourId,        juce::Colour(accent));
    setColour (juce::Slider::trackColourId,        juce::Colour(accent).withAlpha(0.35f));
    setColour (juce::ToggleButton::textColourId,   juce::Colour(textPrimary));
}
