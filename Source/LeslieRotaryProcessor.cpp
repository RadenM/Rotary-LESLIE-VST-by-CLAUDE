#include "LeslieRotaryProcessor.h"
// Include custom editor
#include "LeslieRotaryEditor.h"

//====================== RotaryEngine ======================
void RotaryEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    hornLFO.prepare(spec);
    drumLFO.prepare(spec);
    hornLFO.setFrequency(currentHornSpeed);
    drumLFO.setFrequency(currentDrumSpeed);
    reset();
}

void RotaryEngine::reset()
{
    hornAngle = 0.0f;
    drumAngle = 0.0f;
    rampProgress = 1.0f;
    hornLFO.reset();
    drumLFO.reset();
}

void RotaryEngine::setHornSpeed(float slowHz, float fastHz)
{
    hornSpeedSlow.store(juce::jlimit(0.1f, 6.0f, slowHz));
    hornSpeedFast.store(juce::jlimit(1.0f, 15.0f, fastHz));
}

void RotaryEngine::setDrumSpeed(float slowHz, float fastHz)
{
    drumSpeedSlow.store(juce::jlimit(0.1f, 3.0f, slowHz));
    drumSpeedFast.store(juce::jlimit(0.5f, 10.0f, fastHz));
}

void RotaryEngine::setRampTime(float upSec, float downSec)
{
    rampTimeUp.store(juce::jlimit(0.015f, 10.0f, upSec));
    rampTimeDown.store(juce::jlimit(0.015f, 10.0f, downSec));
}

void RotaryEngine::setSpeedState(int state)
{
    speedState.store(juce::jlimit(0, 2, state));
}

void RotaryEngine::setBrakeAngle(float angleDeg)
{
    brakeAngleDeg.store(std::fmod(angleDeg, 360.0f));
}

void RotaryEngine::setDopplerDepth(float depth)
{
    dopplerDepth.store(juce::jlimit(0.0f, 2.0f, depth));
}

void RotaryEngine::setAmplitudeDepth(float depth)
{
    amDepth.store(juce::jlimit(0.0f, 2.0f, depth));
}

void RotaryEngine::setStereoSpread(float spread)
{
    stereoSpread.store(juce::jlimit(0.0f, 2.0f, spread));
}

void RotaryEngine::setBalance(float balance)
{
    balance01.store(juce::jlimit(0.0f, 1.0f, balance));
}

void RotaryEngine::updateTargetSpeeds()
{
    const int state = speedState.load();
    switch (state)
    {
        case 0: targetHornSpeed = hornSpeedSlow.load(); targetDrumSpeed = drumSpeedSlow.load(); break; // slow
        case 1: targetHornSpeed = hornSpeedFast.load(); targetDrumSpeed = drumSpeedFast.load(); break; // fast
        case 2: targetHornSpeed = 0.0f; targetDrumSpeed = 0.0f; break; // brake
        default: break;
    }
}

void RotaryEngine::processSpeedRamping()
{
    updateTargetSpeeds();

    const float tol = 0.01f;
    if (std::abs(currentHornSpeed - targetHornSpeed) > tol ||
        std::abs(currentDrumSpeed - targetDrumSpeed) > tol)
    {
        if (rampProgress >= 1.0f)
        {
            rampProgress = 0.0f;
            const bool speedingUp = (targetHornSpeed > currentHornSpeed) || (targetDrumSpeed > currentDrumSpeed);
            rampDirection = speedingUp ? 1.0f : -1.0f;
        }
    }

    if (rampProgress < 1.0f)
    {
        const float rampTime = (rampDirection > 0.0f) ? rampTimeUp.load() : rampTimeDown.load();
        const float inc = 1.0f / juce::jmax(1.0f, rampTime * (float) sampleRate);
        rampProgress = juce::jmin(1.0f, rampProgress + inc);
        const float t = 0.5f * (1.0f - std::cos(rampProgress * juce::MathConstants<float>::pi));
        currentHornSpeed = currentHornSpeed + t * (targetHornSpeed - currentHornSpeed);
        currentDrumSpeed = currentDrumSpeed + t * (targetDrumSpeed - currentDrumSpeed);
    }
}

void RotaryEngine::applyStereoSpread(juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2) return;
    const int n = buffer.getNumSamples();
    float* L = buffer.getWritePointer(0);
    float* R = buffer.getWritePointer(1);
    const float spread = stereoSpread.load();
    if (std::abs(spread - 1.0f) < 1e-6f) return;

    for (int i = 0; i < n; ++i)
    {
        const float mid  = 0.5f * (L[i] + R[i]);
        float       side = 0.5f * (L[i] - R[i]);
        side *= spread;
        L[i] = mid + side;
        R[i] = mid - side;
    }
}

void RotaryEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numSamples == 0 || numCh == 0) return;

    processSpeedRamping();

    hornLFO.setFrequency(currentHornSpeed);
    drumLFO.setFrequency(currentDrumSpeed);

    const float dopplerAmt = dopplerDepth.load() * 0.002f; // ~±0.2%
    const float amAmt = amDepth.load() * 0.5f;             // ±50%
    const float bal = balance01.load();

    for (int s = 0; s < numSamples; ++s)
    {
        float hornL = hornLFO.processSample(0.0f);
        float drumL = drumLFO.processSample(0.0f);

        // Update angles
        hornAngle += (currentHornSpeed * 2.0f * juce::MathConstants<float>::pi) / (float) sampleRate;
        drumAngle += (currentDrumSpeed * 2.0f * juce::MathConstants<float>::pi) / (float) sampleRate;
        if (hornAngle >= 2.0f * juce::MathConstants<float>::pi) hornAngle -= 2.0f * juce::MathConstants<float>::pi;
        if (drumAngle >= 2.0f * juce::MathConstants<float>::pi) drumAngle -= 2.0f * juce::MathConstants<float>::pi;

        if (speedState.load() == 2)
        {
            const float a = juce::degreesToRadians(brakeAngleDeg.load());
            hornAngle = a; drumAngle = a;
            hornL = std::sin(a);
            drumL = std::sin(a);
        }

        const float hornDoppler = 1.0f + hornL * dopplerAmt;
        const float drumDoppler = 1.0f + drumL * dopplerAmt;
        const float hornAM = 1.0f + (hornL * amAmt * 0.5f);
        const float drumAM = 1.0f + (drumL * amAmt * 0.5f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer(ch);
            float x = d[s];

            // crude crossover placeholder
            float horn = x * 0.7f * hornDoppler * hornAM;
            float drum = x * 0.8f * drumDoppler * drumAM;

            d[s] = horn * bal + drum * (1.0f - bal);
        }
    }

    if (numCh >= 2)
        applyStereoSpread(buffer);
}

//====================== LeslieRotaryProcessor ======================
static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::HORN_SPEED_SLOW, "Horn Slow",
        juce::NormalisableRange<float>(0.1f, 6.0f, 0.1f), 0.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::HORN_SPEED_FAST, "Horn Fast",
        juce::NormalisableRange<float>(1.0f, 15.0f, 0.1f), 6.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::DRUM_SPEED_SLOW, "Drum Slow",
        juce::NormalisableRange<float>(0.1f, 3.0f, 0.1f), 0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::DRUM_SPEED_FAST, "Drum Fast",
        juce::NormalisableRange<float>(0.5f, 10.0f, 0.1f), 5.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::RAMP_TIME_UP, "Ramp Up",
        juce::NormalisableRange<float>(0.015f, 10.0f, 0.001f, 0.3f), 1.8f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::RAMP_TIME_DOWN, "Ramp Down",
        juce::NormalisableRange<float>(0.015f, 10.0f, 0.001f, 0.3f), 1.8f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        RotaryParams::SPEED_CONTROL, "Speed",
        juce::StringArray{ "Slow", "Fast", "Brake" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::BRAKE_ANGLE, "Brake Angle",
        juce::NormalisableRange<float>(0.0f, 360.0f, 1.0f), 180.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::DOPPLER_DEPTH, "Doppler",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::AM_DEPTH, "AM Depth",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::STEREO_SPREAD, "Stereo Spread",
        juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::HORN_DRUM_BALANCE, "Horn/Drum",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::OUTPUT_LEVEL, "Output",
        juce::NormalisableRange<float>(-60.0f, 12.0f, 0.1f), 0.0f));

#if JucePlugin_Build_Standalone
    // Standalone utility parameters (not exposed in VST/AU)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        RotaryParams::TEST_TONE_ON, "Test Tone", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        RotaryParams::TEST_TONE_FREQ, "Test Freq",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.01f, 0.3f), 440.0f));
#endif

    return { params.begin(), params.end() };
}

juce::AudioProcessorValueTreeState::ParameterLayout LeslieRotaryProcessor::createParameterLayout()
{
    return makeLayout();
}

LeslieRotaryProcessor::LeslieRotaryProcessor()
: juce::AudioProcessor (BusesProperties()
    .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
, valueTreeState(*this, nullptr, "PARAMS", createParameterLayout())
{
}

void LeslieRotaryProcessor::prepareToPlay(double sr, int spb)
{
    juce::dsp::ProcessSpec spec; spec.sampleRate = sr; spec.maximumBlockSize = (juce::uint32) spb; spec.numChannels = (juce::uint32) getTotalNumOutputChannels();
    rotaryEngine.prepare(spec);
    testPhase = 0.0;

    // prime engine with current params
    rotaryEngine.setHornSpeed(*valueTreeState.getRawParameterValue(RotaryParams::HORN_SPEED_SLOW),
                              *valueTreeState.getRawParameterValue(RotaryParams::HORN_SPEED_FAST));
    rotaryEngine.setDrumSpeed(*valueTreeState.getRawParameterValue(RotaryParams::DRUM_SPEED_SLOW),
                              *valueTreeState.getRawParameterValue(RotaryParams::DRUM_SPEED_FAST));
    rotaryEngine.setRampTime(*valueTreeState.getRawParameterValue(RotaryParams::RAMP_TIME_UP),
                             *valueTreeState.getRawParameterValue(RotaryParams::RAMP_TIME_DOWN));
    rotaryEngine.setSpeedState((int) *valueTreeState.getRawParameterValue(RotaryParams::SPEED_CONTROL));
    rotaryEngine.setBrakeAngle(*valueTreeState.getRawParameterValue(RotaryParams::BRAKE_ANGLE));
    rotaryEngine.setDopplerDepth(*valueTreeState.getRawParameterValue(RotaryParams::DOPPLER_DEPTH));
    rotaryEngine.setAmplitudeDepth(*valueTreeState.getRawParameterValue(RotaryParams::AM_DEPTH));
    rotaryEngine.setStereoSpread(*valueTreeState.getRawParameterValue(RotaryParams::STEREO_SPREAD));
    rotaryEngine.setBalance(*valueTreeState.getRawParameterValue(RotaryParams::HORN_DRUM_BALANCE));
}

void LeslieRotaryProcessor::releaseResources()
{
    rotaryEngine.reset();
}

bool LeslieRotaryProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out == juce::AudioChannelSet::disabled()) return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet() != out) return false;
    return true;
}

void LeslieRotaryProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

#if JucePlugin_Build_Standalone
    // Generate a test tone (replaces input) when enabled in Standalone build
    const bool toneOn = (*valueTreeState.getRawParameterValue(RotaryParams::TEST_TONE_ON) > 0.5f);
    if (toneOn)
    {
        const float freq = *valueTreeState.getRawParameterValue(RotaryParams::TEST_TONE_FREQ);
        const double sr = getSampleRate();
        const int n = buffer.getNumSamples();
        const int chs = buffer.getNumChannels();
        for (int s = 0; s < n; ++s)
        {
            float x = (float) std::sin(testPhase);
            testPhase += 2.0 * juce::MathConstants<double>::pi * (double) freq / juce::jmax(1.0, sr);
            if (testPhase >= 2.0 * juce::MathConstants<double>::pi)
                testPhase -= 2.0 * juce::MathConstants<double>::pi;
            for (int ch = 0; ch < chs; ++ch)
                buffer.setSample(ch, s, x);
        }
    }
#endif

    // update any runtime-changed params cheaply (no listeners for now)
    rotaryEngine.setSpeedState((int) *valueTreeState.getRawParameterValue(RotaryParams::SPEED_CONTROL));
    rotaryEngine.setBrakeAngle(*valueTreeState.getRawParameterValue(RotaryParams::BRAKE_ANGLE));
    rotaryEngine.setDopplerDepth(*valueTreeState.getRawParameterValue(RotaryParams::DOPPLER_DEPTH));
    rotaryEngine.setAmplitudeDepth(*valueTreeState.getRawParameterValue(RotaryParams::AM_DEPTH));
    rotaryEngine.setStereoSpread(*valueTreeState.getRawParameterValue(RotaryParams::STEREO_SPREAD));
    rotaryEngine.setBalance(*valueTreeState.getRawParameterValue(RotaryParams::HORN_DRUM_BALANCE));

    rotaryEngine.processBlock(buffer);

    const float outDb = *valueTreeState.getRawParameterValue(RotaryParams::OUTPUT_LEVEL);
    const float outGain = juce::Decibels::decibelsToGain(outDb);
    if (outGain != 1.0f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), outGain);
    }
}

juce::AudioProcessorEditor* LeslieRotaryProcessor::createEditor()
{
    return new LeslieRotaryEditor(*this);
}

void LeslieRotaryProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void LeslieRotaryProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = std::unique_ptr<juce::XmlElement>(getXmlFromBinary(data, sizeInBytes)))
        if (xml->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}

// Factory function required by JUCE to create the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LeslieRotaryProcessor();
}
