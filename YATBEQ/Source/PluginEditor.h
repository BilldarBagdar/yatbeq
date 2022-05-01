/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


enum FFTOrder
{
    order2048 = 11, order4096 = 12, order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    void produceFFTDataRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity);
    void changeOrder(FFTOrder newOrder);
        int getFFTSize() const;
    int getNumAvailableFFTDataBlocks() const;
    bool getFFTData(BlockType& fftData);
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;

};

//template<typename BlockType>
//struct FFTDataGenerator
//{
//    FFTDataGenerator()
//    {
//        order = FFTOrder::order2048;
//    }
//
//    // produces the FFT data from an audio buffer
//    void produceFFTDataRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
//    {
//        const auto fftSize = getFFTSize();
//
//        fftData.assign(fftData.size(), 0);
//        auto* readIndex = audioData.getReadPointer(0);
//        std::copy(readIndex, readIndex + fftSize, fftData.begin());
//
//        window->multiplyWithWindowingTable(fftData.data(), fftSize);
//        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());
//
//        int numBins = (int)fftSize / 2;
//
//        //normalize the fft values
//        for (int i = 0; i < numBins; ++i)
//        {
//            fftData[i] /= (float)numBins;
//        }
//
//        // convert to decibels
//        for (int i = 0; i < numBins; ++i)
//        {
//            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
//        }
//        fftDataFifo.push(fftData);
//    }
//
//    void changeOrder(FFTOrder newOrder)
//    {
//        // when you change order:
//        //   recreate the window, forwardFFT, fifo, fftData
//        //   reset the fifoIndex
//        // recreate on heap with std::make_unique<>
//
//        order = newOrder;
//        auto fftSize = getFFTSize();
//
//        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
//        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);
//
//        fftData.clear();
//        fftData.resize(fftSize * 2, 0);
//        fftDataFifo.prepare(fftData.size());
//    }
//    //====================================================================================
//    int getFFTSize() const { return 1 << order; }
//    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForreading(); }
//    //====================================================================================
//    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
//private:
//    FFTOrder order;
//    BlockType fftData;
//    std::unique_ptr<juce::dsp::FFT> forwardFFT;
//    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
//
//    Fifo<BlockType> fftDataFifo;
//};

//=====================================================================================================

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
            juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap), suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
	juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const; 

private:
    LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::String suffix;
};

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    ResponseCurveComponent(YATBEQAudioProcessor&);
    ~ResponseCurveComponent() override;


    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

    void timerCallback() override;

    void paint(juce::Graphics& g) override;

    void resized();

private:
    YATBEQAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;
    void updateChain();

    juce::Image background;

    juce::Rectangle<int> getRenderedArea();
    juce::Rectangle<int> getAnalysisArea();

    SingleChannelSampleFifo<YATBEQAudioProcessor::BlockType>* leftChannelFifo;
    //SingleChannelSampleFifo<YATBEQAudioProcessor::BlockType>* rightChannelFifo;

    juce::AudioBuffer<float> monoBuffer;
    
    FFTDataGenerator <std::vector<float>> leftChannelFFTDataGenerator;
};


//==============================================================================
/**
*/
class YATBEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    YATBEQAudioProcessorEditor (YATBEQAudioProcessor&);
    ~YATBEQAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    YATBEQAudioProcessor& audioProcessor;

    RotarySliderWithLabels peakFreqSlider, peakGainSlider, peakQualitySlider,
        lowCutFreqSlider, highCutFreqSlider,
        lowCutSlopeSlider, highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment,
        lowCutFreqSliderAttachment, highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YATBEQAudioProcessorEditor)
};
