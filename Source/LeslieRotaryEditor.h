#pragma once
#include <JuceHeader.h>
#include <array>
#include "LeslieRotaryProcessor.h"

class RotaryKnob : public juce::Slider
{
public:
    RotaryKnob()
    {
        setSliderStyle(RotaryHorizontalVerticalDrag);
        setTextBoxStyle(TextBoxBelow, false, 60, 18);
    }
};

class RotorVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit RotorVisualizer(LeslieRotaryProcessor& p) : processor(p) { startTimerHz(60); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF101014));
        auto r = getLocalBounds().toFloat().reduced(10.0f);
        auto top = r.removeFromTop(r.getHeight() * 0.5f).reduced(10.0f);
        auto bot = r.reduced(10.0f);

        drawRotor(g, top, processor.getRotaryEngine().getHornAngle(), juce::Colours::deepskyblue, 4);
        drawRotor(g, bot, processor.getRotaryEngine().getDrumAngle(), juce::Colours::orange, 6);
    }

private:
    void timerCallback() override { repaint(); }

    static void drawRotor(juce::Graphics& g, juce::Rectangle<float> area, float angle, juce::Colour c, int blades)
    {
        g.setColour(area.withTrimmedTop(0).withTrimmedLeft(0).getWidth() > 0 ? c.darker(0.7f) : c);
        g.fillRoundedRectangle(area, 8.0f);
        auto center = area.getCentre();
        float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.35f;
        g.setColour(c);
        g.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2, 2.0f);
        for (int i = 0; i < blades; ++i)
        {
            float a = angle + juce::MathConstants<float>::twoPi * (float)i / (float)blades;
            juce::Point<float> p(center.x + std::cos(a) * radius, center.y + std::sin(a) * radius);
            g.drawLine(center.x, center.y, p.x, p.y, 2.0f);
        }
    }

    LeslieRotaryProcessor& processor;
};

class LeslieRotaryEditor : public juce::AudioProcessorEditor
{
public:
    explicit LeslieRotaryEditor(LeslieRotaryProcessor& p)
        : juce::AudioProcessorEditor(&p), processor(p), visual(p)
    {
        setSize(820, 500);

        addKnob(hornSlow,  hornSlowLabel,  "Horn Slow",  RotaryParams::HORN_SPEED_SLOW);
        addKnob(hornFast,  hornFastLabel,  "Horn Fast",  RotaryParams::HORN_SPEED_FAST);
        addKnob(drumSlow,  drumSlowLabel,  "Drum Slow",  RotaryParams::DRUM_SPEED_SLOW);
        addKnob(drumFast,  drumFastLabel,  "Drum Fast",  RotaryParams::DRUM_SPEED_FAST);
        addKnob(rampUp,    rampUpLabel,    "Ramp Up",    RotaryParams::RAMP_TIME_UP);
        addKnob(rampDown,  rampDownLabel,  "Ramp Down",  RotaryParams::RAMP_TIME_DOWN);
        addKnob(doppler,   dopplerLabel,   "Doppler",    RotaryParams::DOPPLER_DEPTH);
        addKnob(amDepth,   amLabel,        "AM Depth",   RotaryParams::AM_DEPTH);
        addKnob(stereo,    stereoLabel,    "Stereo",     RotaryParams::STEREO_SPREAD);
        addKnob(balance,   balanceLabel,   "Horn/Drum",  RotaryParams::HORN_DRUM_BALANCE);
        addKnob(output,    outputLabel,    "Output dB",  RotaryParams::OUTPUT_LEVEL);

        speedLabel.setText("Speed", juce::dontSendNotification);
        speedLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(speedLabel);
        speed.addItemList({"Slow","Fast","Brake"}, 1);
        addAndMakeVisible(speed);
        speedAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            processor.getValueTreeState(), RotaryParams::SPEED_CONTROL, speed);

        addAndMakeVisible(visual);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF0C0C10));
        juce::ColourGradient grad(juce::Colour(0xFF1A1A22), 0, 0, juce::Colour(0xFF0C0C10), 0, (float)getHeight(), false);
        g.setGradientFill(grad);
        g.fillAll();
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(10);
        auto top = r.removeFromTop(120);

        auto left = top.removeFromLeft(top.getWidth() / 2);
        layoutRow(left, hornSlow, hornFast, drumSlow, drumFast, rampUp, rampDown);

        auto right = top;
        layoutRow(right, doppler, amDepth, stereo, balance, output);

        auto speedRow = r.removeFromTop(40);
        speedLabel.setBounds(speedRow.removeFromLeft(60));
        speed.setBounds(speedRow.removeFromLeft(120));

        visual.setBounds(r.reduced(10));
    }

private:
    void addKnob(RotaryKnob& knob, juce::Label& label, const juce::String& text, const juce::String& paramID)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        addAndMakeVisible(knob);
        knobAttachments.add(new juce::AudioProcessorValueTreeState::SliderAttachment(
            processor.getValueTreeState(), paramID, knob));
    }

    template <typename... Ks>
    void layoutRow(juce::Rectangle<int> area, Ks&... knobs)
    {
        auto items = std::array<RotaryKnob*, sizeof...(knobs)>{ { &knobs... } };
        auto labels = std::array<juce::Label*, sizeof...(knobs)>{ { (&getLabelFor(knobs))... } };
        const int w = area.getWidth() / (int)items.size();
        for (size_t i = 0; i < items.size(); ++i)
        {
            auto cell = area.removeFromLeft(w).reduced(4);
            labels[i]->setBounds(cell.removeFromTop(20));
            items[i]->setBounds(cell.reduced(4));
        }
    }

    juce::Label& getLabelFor(RotaryKnob& k)
    {
        if (&k == &hornSlow)   return hornSlowLabel;
        if (&k == &hornFast)   return hornFastLabel;
        if (&k == &drumSlow)   return drumSlowLabel;
        if (&k == &drumFast)   return drumFastLabel;
        if (&k == &rampUp)     return rampUpLabel;
        if (&k == &rampDown)   return rampDownLabel;
        if (&k == &doppler)    return dopplerLabel;
        if (&k == &amDepth)    return amLabel;
        if (&k == &stereo)     return stereoLabel;
        if (&k == &balance)    return balanceLabel;
        return outputLabel;
    }

    LeslieRotaryProcessor& processor;

    // Controls
    RotaryKnob hornSlow, hornFast, drumSlow, drumFast, rampUp, rampDown;
    RotaryKnob doppler, amDepth, stereo, balance, output;
    juce::Label hornSlowLabel, hornFastLabel, drumSlowLabel, drumFastLabel, rampUpLabel, rampDownLabel;
    juce::Label dopplerLabel, amLabel, stereoLabel, balanceLabel, outputLabel;

    juce::ComboBox speed; juce::Label speedLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> speedAttachment;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> knobAttachments;

    RotorVisualizer visual;
};
