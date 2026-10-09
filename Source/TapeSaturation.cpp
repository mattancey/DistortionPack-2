#include "PluginBase.h"
#include <vector>

class TapeSaturationProcessor : public DistortionBase
{
public:
    TapeSaturationProcessor()
        : DistortionBase ("Tape Saturation", makeLayout(), juce::Colour::fromRGB (225, 190, 90), 2)
    {
        driveP = apvts.getRawParameterValue ("drive");
        wowP   = apvts.getRawParameterValue ("wow");
        flutP  = apvts.getRawParameterValue ("flutter");
        toneP  = apvts.getRawParameterValue ("tone");
        for (int c = 0; c < 2; ++c) line[c].assign (kSize, 0.0f);
    }

protected:
    float processSample (float x, int ch) override
    {
        wowPhase  += wowInc;   if (wowPhase  >= 1.0f) wowPhase  -= 1.0f;
        flutPhase += flutInc;  if (flutPhase >= 1.0f) flutPhase -= 1.0f;

        const float mod = std::sin (wowPhase  * juce::MathConstants<float>::twoPi) * wow * 220.0f
                        + std::sin (flutPhase * juce::MathConstants<float>::twoPi) * flutter * 40.0f;

        float readPos = (float) writePos[ch] - (baseDelay + mod);
        while (readPos < 0.0f) readPos += (float) kSize;
        while (readPos >= (float) kSize) readPos -= (float) kSize;

        const int   i0   = (int) readPos;
        const float frac = readPos - (float) i0;
        const int   i1   = (i0 + 1) % kSize;

        const float d = line[ch][(size_t) i0] * (1.0f - frac) + line[ch][(size_t) i1] * frac;

        float s = std::tanh (d * drive);
        line[ch][(size_t) writePos[ch]] = x;
        writePos[ch] = (writePos[ch] + 1) % kSize;

        toneState[ch] += toneCoeff * (s - toneState[ch]);
        return toneState[ch];
    }

    void updateParams() override
    {
        drive   = driveP->load();
        wow     = wowP->load();
        flutter = flutP->load();

        const double fsOS = sampleRate * 4.0;
        baseDelay = (float) (0.003 * fsOS);
        wowInc    = (float) (0.5 / fsOS);
        flutInc   = (float) (6.5 / fsOS);

        const float f = toneP->load();
        toneCoeff = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * f / (float) fsOS);
    }

    void reset() override
    {
        for (int c = 0; c < 2; ++c)
        {
            std::fill (line[c].begin(), line[c].end(), 0.0f);
            writePos[c] = 0;
        }
        wowPhase = flutPhase = 0.0f;
    }

private:
    static constexpr int kSize = 8192;

    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> ("drive",   "Drive",
                juce::NormalisableRange<float> (1.0f, 12.0f, 0.01f, 0.5f), 2.5f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("wow",     "Wow",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("flutter", "Flutter",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("tone",    "Tone",
                juce::NormalisableRange<float> (1500.0f, 18000.0f, 1.0f, 0.3f), 9000.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("mix",     "Mix",
                juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> ("output",  "Output",
                juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -4.0f));
        return l;
    }

    std::atomic<float>* driveP = nullptr;
    std::atomic<float>* wowP   = nullptr;
    std::atomic<float>* flutP  = nullptr;
    std::atomic<float>* toneP  = nullptr;
    float drive = 2.0f, wow = 0.3f, flutter = 0.2f, toneCoeff = 1.0f;
    float baseDelay = 120.0f, wowInc = 0.0002f, flutInc = 0.002f;
    float wowPhase = 0.0f, flutPhase = 0.0f;
    std::vector<float> line[2];
    int writePos[2] { 0, 0 };
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TapeSaturationProcessor();
}
