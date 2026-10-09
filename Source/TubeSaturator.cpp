#include "PluginBase.h"

class TubeSaturatorProcessor : public DistortionBase
{
public:
    TubeSaturatorProcessor()
        : DistortionBase ("Tube Saturator", makeLayout(), juce::Colour::fromRGB (255, 145, 45), 2)
    {
        driveP = apvts.getRawParameterValue ("drive");
        biasP  = apvts.getRawParameterValue ("bias");
        toneP  = apvts.getRawParameterValue ("tone");
    }

protected:
    float processSample (float x, int ch) override
    {
        float s = x * drive + bias;
        s = s / (1.0f + std::fabs (s));
        toneState[ch] += toneCoeff * (s - toneState[ch]);
        return toneState[ch];
    }

    void updateParams() override
    {
        drive = driveP->load();
        bias  = biasP->load();
        const float f = toneP->load();
        toneCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f
                                     / (float) (sampleRate * 4.0));
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> ("drive",  "Drive",
                juce::NormalisableRange<float> (1.0f, 40.0f, 0.01f, 0.35f), 8.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("bias",   "Bias",
                juce::NormalisableRange<float> (-0.5f, 0.5f, 0.001f), 0.05f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("tone",   "Tone",
                juce::NormalisableRange<float> (200.0f, 18000.0f, 1.0f, 0.3f), 9000.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("mix",    "Mix",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("output", "Output",
                juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -7.0f));
        return l;
    }

    std::atomic<float>* driveP = nullptr;
    std::atomic<float>* biasP  = nullptr;
    std::atomic<float>* toneP  = nullptr;
    float drive = 8.0f, bias = 0.0f, toneCoeff = 0.5f;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TubeSaturatorProcessor();
}
