#pragma once

#include <JuceHeader.h>

class TriBaseKickAudioProcessor : public juce::AudioProcessor
{
public:
    TriBaseKickAudioProcessor();
    ~TriBaseKickAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "TriBase Kick"; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return state; }
    float getAndResetPeak();
    int getLastMidiNote() const { return uiNote.load(); }
    double getLastNoteHz() const { return uiNoteHz.load(); }
    double getCurrentSampleRate() const noexcept { return getSampleRate(); }

    bool readScopeBlock (float* dst, int num);

    enum ParamIndex
    {
        clickLevel,
        bodyStartHz,
        bodyEndHz,
        sweepSemis,
        bodyTimeMs,
        bodyCurve,
        toneHz,
        driveDb,
        tailLevel,
        tailDecayMs,
        outputDb,
        noteTune,
        tuneOffsetSemis,
        velToLevel,
        glideMs,
        paramCount
    };

    static constexpr std::size_t parameterCount = static_cast<std::size_t> (paramCount);

private:
    //==============================================================================
    template <typename FloatType>
    void processBlockInternal (juce::AudioBuffer<FloatType>&, juce::MidiBuffer&);

    juce::AudioProcessorValueTreeState state;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct KickParams
    {
        double clickLevel = 0.3;
        double bodyStartHz = 120.0;
        double bodyEndHz = 50.0;
        double bodyTimeSec = 0.06;
        double bodyCurve = 0.0;
        double toneHz = 1500.0;
        double driveGain = juce::Decibels::decibelsToGain (6.0);
        double tailLevel = 0.5;
        double tailDecaySec = 0.18;
        double outputGain = juce::Decibels::decibelsToGain (0.0);
    };

    KickParams makeTargetParams() const;

    struct KickVoice
    {
        void prepare (double newSampleRate);
        void setTargetParameters (const KickParams& newTarget);
        void trigger (const KickParams& params, double velocity);
        double renderSample();
        bool isActive() const { return active; }

    private:
        double sampleRate = 44100.0;
        double smoothingAlpha = 0.0;

        KickParams target;
        KickParams current;

        double velocity = 0.0;
        double time = 0.0;
        double bodyPhase = 0.0;
        double tailPhase = 0.0;
        double tailEnv = 0.0;
        double clickTime = 0.0;
        double toneState = 0.0;
        double clickLP = 0.0;
        juce::Random random;
        bool active = false;

        static double applyCurve (double t, double curve);
    };

    KickVoice voice;

    std::array<std::atomic<float>*, paramCount> rawParams {};

    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> lastNoteNumber { -1 };
    std::atomic<double> uiNoteHz { 0.0 };
    std::atomic<int> uiNote { -1 };

    // Scope feed (UI pulls from here)
    juce::AudioBuffer<float> scopeBuffer;   // 1 ch, N samples
    std::unique_ptr<juce::AbstractFifo> scopeFifo; // size = SCOPE_CAP
    static constexpr int SCOPE_CAP = 16384; // power of two not required

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriBaseKickAudioProcessor)
};
