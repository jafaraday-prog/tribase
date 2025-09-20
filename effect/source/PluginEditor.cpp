#include "PluginEditor.h"
#include <array>

namespace
{
constexpr juce::uint32 dark = 0xFF0A0E12;
[[maybe_unused]] constexpr juce::uint32 text = 0xFFAEEAFF;
constexpr float kMinDb = -60.0f;
constexpr float kMinGrDb = -48.0f;
constexpr float kMaxDb = 0.0f;
}

void TriBaseAudioProcessorEditor::Scope::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    auto bounds = getLocalBounds().toFloat();

    const auto drawGridLine = [&] (float db, juce::Colour colour)
    {
        const float y = juce::jmap (db, kMinDb, kMaxDb, bounds.getBottom(), bounds.getY());
        g.setColour (colour.withMultipliedAlpha (0.3f));
        g.drawLine (bounds.getX(), y, bounds.getRight(), y, db == 0.0f ? 1.5f : 1.0f);
    };

    drawGridLine (-48.0f, juce::Colours::grey);
    drawGridLine (-24.0f, juce::Colours::grey);
    drawGridLine (-12.0f, juce::Colours::grey);
    drawGridLine (-6.0f, juce::Colours::grey);
    drawGridLine (0.0f, juce::Colours::white);

    const auto drawTrace = [&] (const juce::Array<float>& values, juce::Colour colour, float minDb, float maxDb)
    {
        if (values.isEmpty())
            return;

        juce::Path path;
        const int size = values.size();
        const float step = bounds.getWidth() / juce::jmax (1, size - 1);

        for (int i = 0; i < size; ++i)
        {
            const float x = bounds.getX() + step * static_cast<float> (i);
            const float y = juce::jmap (values.getUnchecked (i), minDb, maxDb, bounds.getBottom(), bounds.getY());

            if (i == 0)
                path.startNewSubPath (x, y);
            else
                path.lineTo (x, y);
        }

        g.setColour (colour.withAlpha (0.9f));
        g.strokePath (path, juce::PathStrokeType (1.5f));
    };

    drawTrace (scHistory, scColour, kMinDb, kMaxDb);
    drawTrace (grHistory, grColour, kMinGrDb, 0.0f);
}

TriBaseAudioProcessorEditor::TriBaseAudioProcessorEditor (TriBaseAudioProcessor& proc)
    : juce::AudioProcessorEditor (&proc),
      processor (proc)
{
    xenoLAF = std::make_unique<XenoLookAndFeel>();
    setLookAndFeel (xenoLAF.get());
    setOpaque (true);
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (dark));

    setSize (720, 420);

    auto configureSlider = [] (juce::Slider& slider, const juce::String& suffix, double min, double max, double step, double defaultValue)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        slider.setRange (min, max, step);
        slider.setNumDecimalPlacesToDisplay (2);
        slider.setDoubleClickReturnValue (true, defaultValue);
        slider.setTextValueSuffix (suffix);
    };

    configureSlider (lookaheadSlider, " ms", 0.0, 5.0, 0.01, 2.0);
    configureSlider (thresholdSlider, " dB", -60.0, 0.0, 0.1, -24.0);
    configureSlider (ratioSlider, " : 1", 1.0, 20.0, 0.1, 4.0);
    configureSlider (attackSlider, " ms", 0.1, 50.0, 0.1, 5.0);
    configureSlider (releaseSlider, " ms", 10.0, 500.0, 1.0, 120.0);
    configureSlider (depthSlider, " dB", 0.0, 48.0, 0.1, 18.0);
    configureSlider (mixSlider, " %", 0.0, 100.0, 0.1, 100.0);
    configureSlider (makeupSlider, " dB", -24.0, 24.0, 0.1, 0.0);

    auto addSlider = [this] (juce::Slider& slider)
    {
        addAndMakeVisible (slider);
    };

    addSlider (lookaheadSlider);
    addSlider (thresholdSlider);
    addSlider (ratioSlider);
    addSlider (attackSlider);
    addSlider (releaseSlider);
    addSlider (depthSlider);
    addSlider (mixSlider);
    addSlider (makeupSlider);

    addAndMakeVisible (scope);
    scope.setColours (juce::Colours::orange, juce::Colours::cyan);

    auto& apvts = processor.apvts;
    lookaheadAttachment = std::make_unique<SliderAttachment> (apvts, "lookaheadMs", lookaheadSlider);
    thresholdAttachment = std::make_unique<SliderAttachment> (apvts, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<SliderAttachment> (apvts, "ratio", ratioSlider);
    attackAttachment = std::make_unique<SliderAttachment> (apvts, "attackMs", attackSlider);
    releaseAttachment = std::make_unique<SliderAttachment> (apvts, "releaseMs", releaseSlider);
    depthAttachment = std::make_unique<SliderAttachment> (apvts, "depthDb", depthSlider);
    mixAttachment = std::make_unique<SliderAttachment> (apvts, "mix", mixSlider);
    makeupAttachment = std::make_unique<SliderAttachment> (apvts, "makeupDb", makeupSlider);

    startTimerHz (60);
}

TriBaseAudioProcessorEditor::~TriBaseAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
    xenoLAF.reset();
}

void TriBaseAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (dark));
}

void TriBaseAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);
    meterBounds = area.removeFromRight (60);

    auto scopeArea = area.removeFromRight (juce::roundToInt (area.getWidth() * 0.45f)).reduced (8);
    scope.setBounds (scopeArea);

    auto gridArea = area.reduced (8);

    constexpr int columns = 4;
    constexpr int rows = 2;
    const int cellWidth = gridArea.getWidth() / columns;
    const int cellHeight = gridArea.getHeight() / rows;

    std::array<juce::Slider*, 8> sliders
    {
        &lookaheadSlider,
        &thresholdSlider,
        &ratioSlider,
        &attackSlider,
        &releaseSlider,
        &depthSlider,
        &mixSlider,
        &makeupSlider
    };

    for (size_t index = 0; index < sliders.size(); ++index)
    {
        const int row = static_cast<int> (index) / columns;
        const int column = static_cast<int> (index) % columns;

        auto bounds = juce::Rectangle<int> (cellWidth * column,
                                            cellHeight * row,
                                            cellWidth,
                                            cellHeight);

        sliders[index]->setBounds (bounds.translated (gridArea.getX(), gridArea.getY()).withTrimmedBottom (20));
    }
}

void TriBaseAudioProcessorEditor::timerCallback()
{
    const auto sc = processor.meterScDb.load();
    const auto gr = processor.meterGrDb.load();
    lastGrDb = gr;
    scope.push (sc, gr);
    repaint (meterBounds);
}
