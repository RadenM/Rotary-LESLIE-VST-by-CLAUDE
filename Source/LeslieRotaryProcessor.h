#pragma once
#include <JuceHeader.h>

namespace RotaryParams
{
    static constexpr const char* HORN_SPEED_SLOW   = "hornSpeedSlow";
    static constexpr const char* HORN_SPEED_FAST   = "hornSpeedFast";
    static constexpr const char* DRUM_SPEED_SLOW   = "drumSpeedSlow";
    static constexpr const char* DRUM_SPEED_FAST   = "drumSpeedFast";
    static constexpr const char* RAMP_TIME_UP      = "rampTimeUp";
    static constexpr const char* RAMP_TIME_DOWN    = "rampTimeDown";
    static constexpr const char* SPEED_CONTROL     = "speedControl"; // 0 slow,1 fast,2 brake
    static constexpr const char* BRAKE_ANGLE       = "brakeAngle";
    static constexpr const char* DOPPLER_DEPTH     = "dopplerDepth";
    static constexpr const char* AM_DEPTH          = "amDepth";
    static constexpr const char* STEREO_SPREAD     = "stereoSpread";
    static constexpr const char* HORN_DRUM_BALANCE = "hornDrumBalance";
    static constexpr const char* OUTPUT_MODE       = "outputMode"; // not used yet
    static constexpr const char* OUTPUT_LEVEL      = "outputLevel";
}

class RotaryEngine
{
public:
    RotaryEngine() = default;
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer);

    // control
    void setHornSpeed(float slowHz, float fastHz);
    void setDrumSpeed(float slowHz, float fastHz);
    void setRampTime(float upSec, float downSec);
    void setSpeedState(int state);
    void setBrakeAngle(float angleDeg);
    void setDopplerDepth(float depth);
    void setAmplitudeDepth(float depth);
    void setStereoSpread(float spread);
    void setBalance(float balance01);

    // UI feedback
    float getHornAngle() const noexcept { return hornAngle; }
    float getDrumAngle() const noexcept { return drumAngle; }
    float getCurrentHornSpeed() const noexcept { return currentHornSpeed; }
    float getCurrentDrumSpeed() const noexcept { return currentDrumSpeed; }
    bool  isRamping() const noexcept { return rampProgress < 1.0f; }

private:
    void updateTargetSpeeds();
    void processSpeedRamping();
    void applyStereoSpread(juce::AudioBuffer<float>& buffer);

    // parameters (atomic for audio thread safety)
    std::atomic<float> hornSpeedSlow { 0.8f }, hornSpeedFast { 6.8f };
    std::atomic<float> drumSpeedSlow { 0.4f }, drumSpeedFast { 5.5f };
    std::atomic<float> rampTimeUp { 1.8f }, rampTimeDown { 1.8f };
    std::atomic<int>   speedState { 0 }; // 0 slow,1 fast,2 brake
    std::atomic<float> brakeAngleDeg { 180.0f };
    std::atomic<float> dopplerDepth { 1.0f }, amDepth { 1.0f };
    std::atomic<float> stereoSpread { 1.0f };
    std::atomic<float> balance01 { 0.5f };

    // state
    double sampleRate { 44100.0 };
    float hornAngle { 0.0f }, drumAngle { 0.0f };
    float currentHornSpeed { 0.8f }, currentDrumSpeed { 0.4f };
    float targetHornSpeed { 0.8f }, targetDrumSpeed { 0.4f };
    float rampProgress { 1.0f }; // 0..1
    float rampDirection { 1.0f };

    // simple LFOs for modulation
    juce::dsp::Oscillator<float> hornLFO { [](float x){ return std::sin(x); } };
    juce::dsp::Oscillator<float> drumLFO { [](float x){ return std::sin(x); } };
};

class LeslieRotaryProcessor : public juce::AudioProcessor
{
public:
    LeslieRotaryProcessor();
    ~LeslieRotaryProcessor() override = default;

    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    RotaryEngine& getRotaryEngine() { return rotaryEngine; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState valueTreeState;
    RotaryEngine rotaryEngine;
};
