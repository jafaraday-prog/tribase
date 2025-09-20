#include "PluginEditor.h"
#include <cmath>

namespace
{
    const auto backgroundColour = juce::Colour (0xFF0A0E12);
    const auto accentColour = juce::Colour (0xFFAEEAFF);

    const juce::StringArray knobIds {
        "clickLevel",
        "bodyStartHz",
        "bodyEndHz",
        "sweepSemis",
        "bodyTimeMs",
        "bodyCurve",
        "toneHz",
        "driveDb",
        "tailLevel",
        "tailDecayMs"
    };

    juce::String midiNoteToString (int midiNote)
    {
        if (midiNote < 0 || midiNote > 127)
            return "--";

        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int noteIndex = midiNote % 12;
        const int octave = (midiNote / 12) - 1;
        return juce::String (names[noteIndex]) + juce::String (octave);
    }
}

//==============================================================================
TriBaseKickAudioProcessorEditor::OscilloscopeView::OscilloscopeView (TriBaseKickAudioProcessor& proc)
    : processor (proc)
{
    startTimerHz (60);
}

void TriBaseKickAudioProcessorEditor::OscilloscopeView::timerCallback()
{
    if (! processor.readScopeBlock (tempBuffer.data(), static_cast<int> (tempBuffer.size())))
        return;

    const int windowSize = static_cast<int> (windowBuffer.size());
    const int centre = static_cast<int> (tempBuffer.size() / 2);
    const int searchRadius = 300;
    int bestIndex = -1;
    int bestDistance = 1 << 30;

    const int start = juce::jmax (0, centre - searchRadius);
    const int end = juce::jmin (centre + searchRadius, static_cast<int> (tempBuffer.size()) - 2);

    for (int i = start; i <= end; ++i)
    {
        if (tempBuffer[i] <= 0.0f && tempBuffer[i + 1] > 0.0f)
        {
            const int distance = std::abs (i - centre);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }
    }

    if (bestIndex < 0)
        bestIndex = juce::jmax (0, centre - (windowSize / 2));

    if (bestIndex + windowSize >= static_cast<int> (tempBuffer.size()))
        bestIndex = static_cast<int> (tempBuffer.size()) - windowSize;

    for (int i = 0; i < windowSize; ++i)
        windowBuffer[i] = tempBuffer[bestIndex + i];

    float peak = 0.0f;
    for (float sample : windowBuffer)
        peak = juce::jmax (peak, std::abs (sample));

    const float normaliser = peak > 1.0e-4f ? (1.0f / peak) : 1.0f;
    for (float& sample : windowBuffer)
        sample *= normaliser;

    hasFrame = true;
    buildPath();
    repaint();
}

void TriBaseKickAudioProcessorEditor::OscilloscopeView::resized()
{
    buildPath();
}

void TriBaseKickAudioProcessorEditor::OscilloscopeView::buildPath()
{
    waveformPath.clear();
    if (! hasFrame)
        return;

    auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty())
        return;

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();
    const float centreY = bounds.getCentreY();
    const float scaleY = height * 0.45f;

    waveformPath.preallocateSpace (static_cast<int> (windowBuffer.size()) * 3);

    waveformPath.startNewSubPath (bounds.getX(), centreY - windowBuffer[0] * scaleY);

    const auto size = static_cast<int> (windowBuffer.size());
    for (int i = 1; i < size; ++i)
    {
        const float x = bounds.getX() + (width * static_cast<float> (i)) / static_cast<float> (size - 1);
        const float y = centreY - windowBuffer[static_cast<size_t> (i)] * scaleY;
        waveformPath.lineTo (x, y);
    }
}

void TriBaseKickAudioProcessorEditor::OscilloscopeView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll (backgroundColour);

    g.setColour (backgroundColour.brighter (0.12f));
    for (int i = 1; i < 4; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * i / 4.0f;
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());

        const float y = bounds.getY() + bounds.getHeight() * i / 4.0f;
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX(), bounds.getRight());
    }

    if (hasFrame && ! waveformPath.isEmpty())
    {
        g.setColour (accentColour.withAlpha (0.9f));
        g.strokePath (waveformPath, juce::PathStrokeType (1.5f));
    }
}

//==============================================================================
TriBaseKickAudioProcessorEditor::SpectrumView::SpectrumView (TriBaseKickAudioProcessor& proc)
    : processor (proc), fft (fftOrder), window (fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    smoothedDb.fill (-72.0f);
    startTimerHz (60);
}

void TriBaseKickAudioProcessorEditor::SpectrumView::timerCallback()
{
    if (! processor.readScopeBlock (timeDomain.data(), fftSize))
        return;

    window.multiplyWithWindowingTable (timeDomain.data(), fftSize);

    std::fill (fftBuffer.begin(), fftBuffer.end(), 0.0f);
    std::copy (timeDomain.begin(), timeDomain.end(), fftBuffer.begin());

    fft.performRealOnlyForwardTransform (fftBuffer.data());

    const double sampleRate = juce::jmax (1.0, processor.getCurrentSampleRate());
    constexpr float minDb = -72.0f;
    constexpr float attack = 0.6f;
    constexpr float release = 0.25f;

    const size_t numBins = smoothedDb.size();
    for (size_t i = 0; i < numBins; ++i)
    {
        float real = 0.0f;
        float imag = 0.0f;

        if (i == 0)
        {
            real = fftBuffer[0];
        }
        else if (i == numBins - 1)
        {
            real = fftBuffer[1];
        }
        else
        {
            const size_t index = i * 2;
            real = fftBuffer[index];
            imag = fftBuffer[index + 1];
        }

        const float magnitude = std::sqrt (real * real + imag * imag) / static_cast<float> (fftSize);
        const float targetDb = juce::Decibels::gainToDecibels (magnitude, minDb);
        const float current = smoothedDb[i];
        const float coeff = targetDb > current ? attack : release;
        const float updated = current + coeff * (targetDb - current);
        smoothedDb[i] = juce::jlimit (minDb, 0.0f, updated);
    }

    hasFrame = true;
    rebuildPath();
    repaint();
}

void TriBaseKickAudioProcessorEditor::SpectrumView::resized()
{
    rebuildPath();
}

void TriBaseKickAudioProcessorEditor::SpectrumView::rebuildPath()
{
    spectrumPath.clear();
    if (! hasFrame)
        return;

    auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty())
        return;

    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 10000.0f;
    constexpr float minDb = -72.0f;

    const double sampleRate = juce::jmax (1.0, processor.getCurrentSampleRate());
    const size_t numBins = smoothedDb.size();
    const float logMin = std::log10 (minFreq);
    const float logMax = std::log10 (maxFreq);

    juce::Path path;
    path.preallocateSpace (static_cast<int> (numBins) * 3);

    const float bottom = bounds.getBottom();
    const float left = bounds.getX();
    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    path.startNewSubPath (left, bottom);

    for (size_t i = 1; i < numBins; ++i)
    {
        const double binFreq = (static_cast<double> (i) * sampleRate) / static_cast<double> (fftSize);
        const float limitedFreq = juce::jlimit (minFreq, maxFreq, static_cast<float> (binFreq));
        const float normX = (std::log10 (limitedFreq) - logMin) / (logMax - logMin);
        const float db = smoothedDb[i];
        const float normY = juce::jlimit (0.0f, 1.0f, (0.0f - db) / -minDb);

        const float x = left + width * normX;
        const float y = bottom - height * normY;
        path.lineTo (x, y);
    }

    path.lineTo (left + width, bottom);
    path.closeSubPath();
    spectrumPath = std::move (path);
}

void TriBaseKickAudioProcessorEditor::SpectrumView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll (backgroundColour);

    g.setColour (backgroundColour.brighter (0.12f));
    constexpr int verticalLines = 6;
    for (int i = 1; i < verticalLines; ++i)
    {
        const float x = bounds.getX() + bounds.getWidth() * i / verticalLines;
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());
    }

    for (int db = 0; db >= -72; db -= 12)
    {
        const float norm = (0.0f - static_cast<float> (db)) / 72.0f;
        const float y = bounds.getY() + bounds.getHeight() * norm;
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX(), bounds.getRight());
    }

    if (hasFrame && ! spectrumPath.isEmpty())
    {
        g.setColour (accentColour.withAlpha (0.35f));
        g.fillPath (spectrumPath);
        g.setColour (accentColour.withAlpha (0.8f));
        g.strokePath (spectrumPath, juce::PathStrokeType (1.2f));
    }

    const double markerHz = processor.getLastNoteHz();
    const int markerNote = processor.getLastMidiNote();
    if (markerHz > 0.0 && markerNote >= 0)
    {
        const float logMin = std::log10 (20.0f);
        const float logMax = std::log10 (10000.0f);
        const float clampedFreq = juce::jlimit (20.0f, 10000.0f, static_cast<float> (markerHz));
        const float normX = (std::log10 (clampedFreq) - logMin) / (logMax - logMin);
        const float x = bounds.getX() + bounds.getWidth() * normX;
        g.setColour (accentColour.withAlpha (0.7f));
        g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 1.2f);

        const auto name = midiNoteToString (markerNote);
        const juce::String label = name + juce::String (" ") + juce::String (markerHz, 1) + " Hz";
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawFittedText (label, juce::Rectangle<int> ((int) x - 80, (int) bounds.getY(), 160, 16), juce::Justification::centred, 1);
    }
}

TriBaseKickAudioProcessorEditor::TriBaseKickAudioProcessorEditor (TriBaseKickAudioProcessor& proc)
    : juce::AudioProcessorEditor (&proc),
      processor (proc),
      meter (proc),
      oscilloscope (proc),
      spectrum (proc)
{
    setSize (720, 420);
    setResizable (false, false);

    configureKnob (clickLevel, "Click Level");
    configureKnob (bodyStartHz, "Start Hz", " Hz");
    configureKnob (bodyEndHz, "End Hz", " Hz");
    configureKnob (sweepSemis, "Sweep ±Semis", " st");
    configureKnob (bodyTimeMs, "Sweep ms", " ms");
    configureKnob (bodyCurve, "Curve");
    configureKnob (toneHz, "Tone Hz", " Hz");
    configureKnob (driveDb, "Drive dB", " dB");
    configureKnob (tailLevel, "Tail Level");
    configureKnob (tailDecayMs, "Tail Decay ms", " ms");

    auto applyPercentDisplay = [] (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double value)
        {
            return juce::String (value * 100.0, 1) + " %";
        };
        slider.valueFromTextFunction = [] (const juce::String& text)
        {
            return text.upToFirstOccurrenceOf ("%", false, false).getDoubleValue() / 100.0;
        };
    };

    applyPercentDisplay (clickLevel);
    applyPercentDisplay (tailLevel);
    sweepSemis.setNumDecimalPlacesToDisplay (0);
    toneHz.setNumDecimalPlacesToDisplay (1);
    bodyStartHz.setNumDecimalPlacesToDisplay (1);
    bodyEndHz.setNumDecimalPlacesToDisplay (1);

    auto& vts = processor.getValueTreeState();
    juce::Slider* sliders[] = {
        &clickLevel,
        &bodyStartHz,
        &bodyEndHz,
        &sweepSemis,
        &bodyTimeMs,
        &bodyCurve,
        &toneHz,
        &driveDb,
        &tailLevel,
        &tailDecayMs
    };

    for (int i = 0; i < knobIds.size(); ++i)
        attachments.emplace_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (vts, knobIds[i], *sliders[i]));

    auto setupLabel = [] (juce::Label& label, juce::Component& comp, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, accentColour);
        label.setFont (juce::Font (juce::FontOptions (12.0f)));
        label.setInterceptsMouseClicks (false, false);
        label.attachToComponent (&comp, false);
    };

    addAndMakeVisible (clickLabel);      setupLabel (clickLabel, clickLevel, "Click Level");
    addAndMakeVisible (bodyStartLabel); setupLabel (bodyStartLabel, bodyStartHz, "Start Hz");
    addAndMakeVisible (bodyEndLabel);   setupLabel (bodyEndLabel, bodyEndHz, "End Hz");
    addAndMakeVisible (sweepLabel);     setupLabel (sweepLabel, sweepSemis, "Sweep ±Semis");
    addAndMakeVisible (bodyTimeLabel);  setupLabel (bodyTimeLabel, bodyTimeMs, "Sweep ms");
    addAndMakeVisible (bodyCurveLabel); setupLabel (bodyCurveLabel, bodyCurve, "Curve");
    addAndMakeVisible (toneLabel);      setupLabel (toneLabel, toneHz, "Tone Hz");
    addAndMakeVisible (driveLabel);     setupLabel (driveLabel, driveDb, "Drive dB");
    addAndMakeVisible (tailLevelLabel); setupLabel (tailLevelLabel, tailLevel, "Tail Level");
    addAndMakeVisible (tailDecayLabel); setupLabel (tailDecayLabel, tailDecayMs, "Tail Decay ms");

    addAndMakeVisible (noteReadout);
    noteReadout.setJustificationType (juce::Justification::centredRight);
    noteReadout.setColour (juce::Label::textColourId, accentColour);
    noteReadout.setFont (juce::Font (juce::FontOptions (16.0f)));
    noteReadout.setText ("Note: --", juce::dontSendNotification);

    addAndMakeVisible (modeLabel);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setColour (juce::Label::textColourId, accentColour);
    modeLabel.setFont (juce::Font (juce::FontOptions (14.0f)));

    waveButton.setRadioGroupId (1);
    spectrumButton.setRadioGroupId (1);
    waveButton.setClickingTogglesState (true);
    spectrumButton.setClickingTogglesState (true);
    waveButton.setToggleState (true, juce::dontSendNotification);
    spectrumButton.setToggleState (false, juce::dontSendNotification);
    addAndMakeVisible (waveButton);
    addAndMakeVisible (spectrumButton);

    waveButton.onClick = [this]
    {
        showingSpectrum = false;
        waveButton.setToggleState (true, juce::dontSendNotification);
        spectrumButton.setToggleState (false, juce::dontSendNotification);
        modeLabel.setText ("Waveform", juce::dontSendNotification);
        oscilloscope.setVisible (true);
        spectrum.setVisible (false);
    };

    spectrumButton.onClick = [this]
    {
        showingSpectrum = true;
        spectrumButton.setToggleState (true, juce::dontSendNotification);
        waveButton.setToggleState (false, juce::dontSendNotification);
        modeLabel.setText ("Spectrum", juce::dontSendNotification);
        oscilloscope.setVisible (false);
        spectrum.setVisible (true);
    };

    modeLabel.setText ("Waveform", juce::dontSendNotification);

    addAndMakeVisible (oscilloscope);
    addAndMakeVisible (spectrum);
    spectrum.setVisible (false);
    addAndMakeVisible (meter);

    addAndMakeVisible (clickLevel);
    addAndMakeVisible (bodyStartHz);
    addAndMakeVisible (bodyEndHz);
    addAndMakeVisible (sweepSemis);
    addAndMakeVisible (bodyTimeMs);
    addAndMakeVisible (bodyCurve);
    addAndMakeVisible (toneHz);
    addAndMakeVisible (driveDb);
    addAndMakeVisible (tailLevel);
    addAndMakeVisible (tailDecayMs);

    startTimerHz (30);
}

void TriBaseKickAudioProcessorEditor::configureKnob (juce::Slider& slider, const juce::String& name, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setTextValueSuffix (suffix);
    slider.setName (name);
}

void TriBaseKickAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (backgroundColour);

    g.setColour (accentColour.withAlpha (0.8f));
    juce::Font titleFont (juce::FontOptions (24.0f));
    titleFont.setBold (true);
    g.setFont (titleFont);
    g.drawFittedText ("TriBase Kick", getLocalBounds().removeFromTop (40), juce::Justification::centred, 1);
}

void TriBaseKickAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (20);

    auto header = bounds.removeFromTop (40);
    noteReadout.setBounds (header.removeFromRight (220));

    auto buttonRow = bounds.removeFromTop (28);
    waveButton.setBounds (buttonRow.removeFromLeft (90).reduced (0, 4));
    spectrumButton.setBounds (buttonRow.removeFromLeft (110).reduced (0, 4));

    auto modeRow = bounds.removeFromTop (20);
    modeLabel.setBounds (modeRow);

    auto content = bounds;
    auto left = content.removeFromLeft (content.proportionOfWidth (0.6f));
    auto right = content.removeFromRight (content.getWidth());

    juce::Grid grid;
    grid.rowGap = juce::Grid::Px (16.0f);
    grid.columnGap = juce::Grid::Px (16.0f);
    grid.templateRows = {
        juce::Grid::TrackInfo (juce::Grid::Fr (1)),
        juce::Grid::TrackInfo (juce::Grid::Fr (1)),
        juce::Grid::TrackInfo (juce::Grid::Fr (1)),
        juce::Grid::TrackInfo (juce::Grid::Fr (1))
    };
    grid.templateColumns = { juce::Grid::TrackInfo (juce::Grid::Fr (1)), juce::Grid::TrackInfo (juce::Grid::Fr (1)), juce::Grid::TrackInfo (juce::Grid::Fr (1)) };

    grid.items = {
        juce::GridItem (clickLevel),
        juce::GridItem (bodyStartHz),
        juce::GridItem (bodyEndHz),
        juce::GridItem (sweepSemis),
        juce::GridItem (bodyTimeMs),
        juce::GridItem (bodyCurve),
        juce::GridItem (toneHz),
        juce::GridItem (driveDb),
        juce::GridItem (tailLevel),
        juce::GridItem (tailDecayMs),
        juce::GridItem(),
        juce::GridItem()
    };

    grid.performLayout (left);

    auto viewArea = right.removeFromTop (right.getHeight() - 80);
    oscilloscope.setBounds (viewArea.reduced (4));
    spectrum.setBounds (viewArea.reduced (4));

    meter.setBounds (right.reduced (4));
}

void TriBaseKickAudioProcessorEditor::timerCallback()
{
    const int note = processor.getLastMidiNote();
    const double freq = processor.getLastNoteHz();

    juce::String noteText = "Note: --";
    if (note >= 0)
    {
        const auto name = midiNoteToString (note);
        if (name != "--")
        {
            if (freq > 0.0)
                noteText = juce::String ("Note: ") + name + " " + juce::String (freq, 1) + " Hz";
            else
                noteText = juce::String ("Note: ") + name + " -- Hz";
        }
    }

    if (noteReadout.getText() != noteText)
        noteReadout.setText (noteText, juce::dontSendNotification);
}
