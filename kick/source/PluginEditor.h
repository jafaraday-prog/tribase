#pragma once

#include <JuceHeader.h>
#include <array>
#include "PluginProcessor.h"

class TriBaseKickAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit TriBaseKickAudioProcessorEditor (TriBaseKickAudioProcessor&);
    ~TriBaseKickAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class OscilloscopeView : public juce::Component, private juce::Timer
    {
    public:
        explicit OscilloscopeView (TriBaseKickAudioProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        void buildPath();

        TriBaseKickAudioProcessor& processor;
        std::array<float, 2048> tempBuffer {};
        std::array<float, 1024> windowBuffer {};
        juce::Path waveformPath;
        bool hasFrame = false;
    };

    class SpectrumView : public juce::Component, private juce::Timer
    {
    public:
        explicit SpectrumView (TriBaseKickAudioProcessor&);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        void rebuildPath();

        TriBaseKickAudioProcessor& processor;
        static constexpr int fftOrder = 10;
        static constexpr int fftSize = 1 << fftOrder;
        std::array<float, fftSize> timeDomain {};
        std::array<float, fftSize * 2> fftBuffer {};
        std::array<float, (fftSize / 2) + 1> smoothedDb {};
        juce::dsp::FFT fft;
        juce::dsp::WindowingFunction<float> window;
        juce::Path spectrumPath;
        bool hasFrame = false;
    };

    void timerCallback() override;
    void configureKnob (juce::Slider& slider, const juce::String& name, const juce::String& suffix = {});

    TriBaseKickAudioProcessor& processor;

    juce::Slider clickLevel;
    juce::Slider bodyStartHz;
    juce::Slider bodyEndHz;
    juce::Slider sweepSemis;
    juce::Slider bodyTimeMs;
    juce::Slider bodyCurve;
    juce::Slider toneHz;
    juce::Slider driveDb;
    juce::Slider tailLevel;
    juce::Slider tailDecayMs;

    juce::Label clickLabel;
    juce::Label bodyStartLabel;
    juce::Label bodyEndLabel;
    juce::Label sweepLabel;
    juce::Label bodyTimeLabel;
    juce::Label bodyCurveLabel;
    juce::Label toneLabel;
    juce::Label driveLabel;
    juce::Label tailLevelLabel;
    juce::Label tailDecayLabel;

    juce::Label noteReadout;
    juce::Label modeLabel;
    juce::TextButton waveButton { "Wave" };
    juce::TextButton spectrumButton { "Spectrum" };

    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;

    class PeakMeter : public juce::Component, private juce::Timer
    {
    public:
        explicit PeakMeter (TriBaseKickAudioProcessor& proc) : processor (proc)
        {
            startTimerHz (30);
        }

        void paint (juce::Graphics& g) override
        {
            const auto background = juce::Colour (0xFF101317);
            const auto fill = juce::Colour (0xFF3AD5FF);

            g.fillAll (background);

            const auto bounds = getLocalBounds().toFloat();
            auto meterBounds = bounds.reduced (4.0f);
            meterBounds.removeFromTop (meterBounds.getHeight() * (1.0f - level));
            g.setColour (fill);
            g.fillRect (meterBounds);
        }

    private:
        void timerCallback() override
        {
            const auto peak = processor.getAndResetPeak();
            level = juce::jmax (peak, level * 0.8f);
            repaint();
        }

        TriBaseKickAudioProcessor& processor;
        float level = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeakMeter)
    } meter;

    OscilloscopeView oscilloscope;
    SpectrumView spectrum;
    bool showingSpectrum = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseKickAudioProcessorEditor)
};
