#include "PluginBase.h"

class BitCrusherProcessor : public DistortionBase
{
public:
    BitCrusherProcessor()
        : DistortionBase ("Bit Crusher", makeLayout(), juce::Colour::fromRGB (70, 200, 235), 1)
    {
        bitsP  = apvts.getRawParameterValue ("bits");
        downP  = apvts.getRawParameterValue ("down");
        driveP = apvts.getRawParameterValue ("drive");
        toneP  = apvts.getRawParameterValue ("tone");
    }

protected:
    float processSample (float x, int ch) override
    {
        if (--counter[ch] <= 0)
        {
            counter[ch] = (int) downsample;
            held[ch] = x * drive;
        }
        float s = std::round (held[ch] * levels) / levels;
        s = juce::jlimit (-1.0f, 1.0f, s);
        toneState[ch] += toneCoeff * (s - toneState[ch]);
        return toneState[ch];
    }

    void updateParams() override
    {
        bits       = bitsP->load();
        downsample = downP->load();
        drive      = driveP->load();
        levels     = std::pow (2.0f, bits - 1.0f);
        const float f = toneP->load();
        toneCoeff  = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f
                                      / (float) (sampleRate * 2.0));
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c) { held[c] = 0.0f; counter[c] = 1; }
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> ("bits",   "Bits",
                juce::NormalisableRange<float> (1.0f, 16.0f, 0.01f), 8.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("down",   "Rate",
                juce::NormalisableRange<float> (1.0f, 64.0f, 0.01f, 0.4f), 4.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("drive",  "Drive",
                juce::NormalisableRange<float> (1.0f, 24.0f, 0.01f, 0.4f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("tone",   "Tone",
                juce::NormalisableRange<float> (500.0f, 20000.0f, 1.0f, 0.3f), 16000.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("mix",    "Mix",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("output", "Output",
                juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -3.0f));
        return l;
    }

    std::atomic<float>* bitsP  = nullptr;
    std::atomic<float>* downP  = nullptr;
    std::atomic<float>* driveP = nullptr;
    std::atomic<float>* toneP  = nullptr;
    float bits = 8.0f, downsample = 4.0f, drive = 1.0f, toneCoeff = 1.0f;
    float levels = 256.0f;
    float held[2] { 0.0f, 0.0f };
    int   counter[2] { 1, 1 };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BitCrusherProcessor();
}
