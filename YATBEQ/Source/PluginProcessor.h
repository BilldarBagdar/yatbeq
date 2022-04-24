/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12, Slope_24, Slope_36, Slope_48
};

struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

ChainSettings getTreeStateChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class YATBEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    YATBEQAudioProcessor();
    ~YATBEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameters() };

    //==============================================================================

private:
    //==============================================================================
    //==============================================================================
    using Filter = juce::dsp::IIR::Filter<float>;

    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

    MonoChain leftChain, rightChain;

    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };

    void updatePeakFilter(const ChainSettings& chainSettings);

    // declare an alias to some relatively unknown type
    using Coefficients = Filter::CoefficientsPtr;
    // use the alias to declare a helper function
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);

    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& cutType, const CoefficientType& cutCoefficients, const Slope& cutSlope)
    {

        cutType.template setBypassed<0>(true);
        cutType.template setBypassed<1>(true);
        cutType.template setBypassed<2>(true);
        cutType.template setBypassed<3>(true);

        switch (cutSlope)
        {
        case Slope_12:
        {
            updateCoefficients(cutType.template get<0>().coefficients, cutCoefficients[0]);
            cutType.template setBypassed<0>(false);
            break;
        }
        case Slope_24:
        {
            updateCoefficients(cutType.template get<0>().coefficients, cutCoefficients[0]);
            updateCoefficients(cutType.template get<1>().coefficients, cutCoefficients[1]);
            cutType.template setBypassed<0>(false);
            cutType.template setBypassed<1>(false);
            break;
        }
        case Slope_36:
        {
            updateCoefficients(cutType.template get<0>().coefficients, cutCoefficients[0]);
            updateCoefficients(cutType.template get<1>().coefficients, cutCoefficients[1]);
            updateCoefficients(cutType.template get<2>().coefficients, cutCoefficients[2]);
            cutType.template setBypassed<0>(false);
            cutType.template setBypassed<1>(false);
            cutType.template setBypassed<2>(false);
            break;
        }
        case Slope_48:
        {
            updateCoefficients(cutType.template get<0>().coefficients, cutCoefficients[0]);
            updateCoefficients(cutType.template get<1>().coefficients, cutCoefficients[1]);
            updateCoefficients(cutType.template get<2>().coefficients, cutCoefficients[2]);
            updateCoefficients(cutType.template get<3>().coefficients, cutCoefficients[3]);
            cutType.template setBypassed<0>(false);
            cutType.template setBypassed<1>(false);
            cutType.template setBypassed<2>(false);
            cutType.template setBypassed<3>(false);
            break;
        }
        }

    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YATBEQAudioProcessor)
};
