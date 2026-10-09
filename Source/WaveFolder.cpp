#include "PluginBase.h"

class WaveFolderProcessor : public DistortionBase
{
public:
    WaveFolderProcessor()
        : DistortionBase ("Wave Folder", makeLayout(), juce::Colour::fromRGB (150, 120, 255), 2)
    {
        foldP = apvts.getRawParameterValue ("fold");
        symP  = apvts.getRawParameterValue ("symmetry");
        toneP = apvts.getRawParameterValue ("tone");
    }

protected:
    float processSample (float x, int ch) override
    {
        float s = x * fold + symmetry;
        float t = (s + 1.0f) * 0.5f;
        t -= std::floor (t);
        s = 4.0f * std::fabs (t - 0.5f) - 1.0f;

        toneState[ch] += toneCoeff * (s - toneState[ch]);
        return toneState[ch];
    }

    void updateParams() override
    {
        fold     = foldP->load();
        symmetry = symP->load();
        const float f = toneP->load();
        toneCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f
                                     / (float) (sampleRate * 4.0));
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> ("fold",     "Fold",
                juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.4f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("symmetry", "Symmetry",
                juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("tone",     "Tone",
                juce::NormalisableRange<float> (500.0f, 20000.0f, 1.0f, 0.3f), 12000.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("mix",      "Mix",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("output",   "Output",
                juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -6.0f));
        return l;
    }

    std::atomic<float>* foldP = nullptr;
    std::atomic<float>* symP  = nullptr;
    std::atomic<float>* toneP = nullptr;
    float fold = 1.0f, symmetry = 0.0f, toneCoeff = 1.0f;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveFolderProcessor();
}
