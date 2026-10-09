#include "PluginBase.h"

class FuzzMachineProcessor : public DistortionBase
{
public:
    FuzzMachineProcessor()
        : DistortionBase ("Fuzz Machine", makeLayout(), juce::Colour::fromRGB (230, 60, 70), 2)
    {
        fuzzP = apvts.getRawParameterValue ("fuzz");
        gateP = apvts.getRawParameterValue ("gate");
        biasP = apvts.getRawParameterValue ("bias");
        toneP = apvts.getRawParameterValue ("tone");
    }

protected:
    float processSample (float x, int ch) override
    {
        float s = x * fuzz + bias;
        s = juce::jlimit (-1.0f, 1.0f, s);
        s = (s - (s * s * s) / 3.0f) * 1.5f;

        const float a = std::fabs (x);
        env[ch] = a > env[ch] ? a : env[ch] * 0.9994f;
        const float target = env[ch] < gate ? 0.0f : 1.0f;
        gateGain[ch] += 0.004f * (target - gateGain[ch]);
        s *= gateGain[ch];

        toneState[ch] += toneCoeff * (s - toneState[ch]);
        return toneState[ch];
    }

    void updateParams() override
    {
        fuzz = fuzzP->load();
        gate = gateP->load();
        bias = biasP->load();
        const float f = toneP->load();
        toneCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f
                                     / (float) (sampleRate * 4.0));
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c) { env[c] = 0.0f; gateGain[c] = 1.0f; }
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> ("fuzz",   "Fuzz",
                juce::NormalisableRange<float> (1.0f, 200.0f, 0.1f, 0.3f), 40.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("gate",   "Gate",
                juce::NormalisableRange<float> (0.0f, 0.2f, 0.0005f), 0.01f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("bias",   "Bias",
                juce::NormalisableRange<float> (-0.5f, 0.5f, 0.001f), 0.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("tone",   "Tone",
                juce::NormalisableRange<float> (300.0f, 16000.0f, 1.0f, 0.3f), 6000.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("mix",    "Mix",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("output", "Output",
                juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -10.0f));
        return l;
    }

    std::atomic<float>* fuzzP = nullptr;
    std::atomic<float>* gateP = nullptr;
    std::atomic<float>* biasP = nullptr;
    std::atomic<float>* toneP = nullptr;
    float fuzz = 30.0f, gate = 0.01f, bias = 0.0f, toneCoeff = 0.5f;
    float env[2] { 0.0f, 0.0f };
    float gateGain[2] { 1.0f, 1.0f };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuzzMachineProcessor();
}
