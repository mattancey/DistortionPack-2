#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class GenericEditor : public juce::AudioProcessorEditor
{
public:
    GenericEditor (juce::AudioProcessor& p,
                   juce::AudioProcessorValueTreeState& s,
                   juce::String title,
                   juce::Colour accent)
        : AudioProcessorEditor (&p), apvts (s), titleText (std::move (title)), accentColour (accent)
    {
        for (auto* param : p.getParameters())
        {
            auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param);
            if (rp == nullptr) continue;
            const auto id = rp->getParameterID();

            auto* sl = new juce::Slider();
            sl->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            sl->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 16);
            sl->setColour (juce::Slider::rotarySliderFillColourId, accentColour);
            sl->setColour (juce::Slider::rotarySliderOutlineColourId,
                           juce::Colours::black.withAlpha (0.35f));
            sl->setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
            sl->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            addAndMakeVisible (sl);
            attachments.add (new juce::AudioProcessorValueTreeState::SliderAttachment (apvts, id, *sl));
            sliders.add (sl);

            auto* lb = new juce::Label();
            lb->setText (rp->getName (24).toUpperCase(), juce::dontSendNotification);
            lb->setJustificationType (juce::Justification::centred);
            lb->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
            addAndMakeVisible (lb);
            labels.add (lb);
        }
        const int n = juce::jmax (1, (int) sliders.size());
        setSize (juce::jmax (420, n * 92), 210);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour::fromRGB (24, 26, 32));
        auto r = getLocalBounds().removeFromTop (30);
        g.setColour (accentColour);
        g.setFont (juce::Font (17.0f, juce::Font::bold));
        g.drawText (titleText, r.reduced (12, 0), juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (8);
        area.removeFromTop (26);
        const int n = (int) sliders.size();
        if (n == 0) return;
        const int w = area.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto col = area.removeFromLeft (w);
            labels[i]->setBounds (col.removeFromTop (18));
            sliders[i]->setBounds (col.reduced (4));
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String titleText;
    juce::Colour accentColour;
    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label>  labels;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> attachments;
};

class DistortionBase : public juce::AudioProcessor
{
public:
    DistortionBase (const juce::String& name,
                    juce::AudioProcessorValueTreeState::ParameterLayout layout,
                    juce::Colour accent,
                    int oversampleStages = 2)
        : juce::AudioProcessor (BusesProperties()
              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "PARAMS", std::move (layout)),
          displayName (name),
          oversampling (2, (size_t) juce::jmax (1, oversampleStages),
                        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR),
          accentColour (accent)
    {
        mixParam = apvts.getRawParameterValue ("mix");
        outParam = apvts.getRawParameterValue ("output");
    }

    juce::AudioProcessorValueTreeState apvts;

    void prepareToPlay (double sr, int blockSize) override
    {
        sampleRate = sr;
        const int bs = juce::jmax (blockSize, 64);
        oversampling.initProcessing ((size_t) bs);
        oversampling.reset();
        dryBuffer.setSize (2, bs, false, false, true);
        for (int ch = 0; ch < 2; ++ch) { dcState[ch] = 0.0f; dcPrev[ch] = 0.0f; toneState[ch] = 0.0f; }
        reset();
        updateParams();
    }

    void releaseResources() override { oversampling.reset(); }

    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
            && l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        const int numCh = juce::jmin (buffer.getNumChannels(), 2);
        const int numS  = buffer.getNumSamples();
        if (numS == 0) return;

        if (dryBuffer.getNumSamples() < numS)
            dryBuffer.setSize (2, numS, false, false, true);
        for (int ch = 0; ch < numCh; ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numS);

        updateParams();

        juce::dsp::AudioBlock<float> block (buffer);
        auto up = oversampling.processSamplesUp (block);
        const int upCh = (int) up.getNumChannels();
        const int upN  = (int) up.getNumSamples();

        for (int ch = 0; ch < upCh; ++ch)
        {
            auto* d = up.getChannelPointer ((size_t) ch);
            const int c = juce::jmin (ch, 1);
            for (int i = 0; i < upN; ++i)
            {
                const float wet = processSample (d[i], c);
                dcState[c] = wet - dcPrev[c] + 0.9995f * dcState[c];
                dcPrev[c]  = wet;
                d[i] = dcState[c];
            }
        }
        oversampling.processSamplesDown (block);

        const float mix  = mixParam->load();
        const float outG = juce::Decibels::decibelsToGain (outParam->load());

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* w  = buffer.getWritePointer (ch);
            auto* dr = dryBuffer.getReadPointer (ch);
            for (int i = 0; i < numS; ++i)
                w[i] = (dr[i] * (1.0f - mix) + w[i] * mix) * outG;
        }
    }

    juce::AudioProcessorEditor* createEditor() override
    { return new GenericEditor (*this, apvts, displayName, accentColour); }

    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return displayName; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        auto st = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml (st.createXml());
        copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
        if (xml && xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }

    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

protected:
    virtual float processSample (float x, int ch) = 0;
    virtual void  updateParams() {}
    void reset() override {}

    juce::String displayName;
    double sampleRate = 44100.0;
    float  toneState[2] { 0.0f, 0.0f };

private:
    juce::dsp::Oversampling<float> oversampling;
    juce::AudioBuffer<float> dryBuffer;
    float dcState[2] { 0.0f, 0.0f };
    float dcPrev[2]  { 0.0f, 0.0f };
    juce::Colour accentColour;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistortionBase)
};
