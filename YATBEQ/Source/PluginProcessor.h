/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Cut_Slope
{
    Slope_12, Slope_24, Slope_36, Slope_48
};

struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Cut_Slope lowCutSlope{ Cut_Slope::Slope_12 }, highCutSlope{ Cut_Slope::Slope_12 };
};

ChainSettings getTreeStateChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

// declare an alias to some relatively unknown type
using MyCoefficients = Filter::CoefficientsPtr;

inline 
MyCoefficients makeThisPeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    MyCoefficients rtn = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
    return rtn;
}

// use the alias to declare a helper function
void updateCoefficients(MyCoefficients& old, const MyCoefficients& replacements);



using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& cutType, const CoefficientType& cutCoefficients, const Cut_Slope& cutSlope)
{

    cutType.template setBypassed<0>(true);
    cutType.template setBypassed<1>(true);
    cutType.template setBypassed<2>(true);
    cutType.template setBypassed<3>(true);

    switch (cutSlope)
    {
    case Slope_48:
    {
        update<3>(cutType, cutCoefficients);
    }
    case Slope_36:
    {
        update<2>(cutType, cutCoefficients);
    }
    case Slope_24:
    {
        update<1>(cutType, cutCoefficients);
    }
    case Slope_12:
    {
        update<0>(cutType, cutCoefficients);
    }
    }

}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        sampleRate,
        2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        sampleRate,
        2 * (chainSettings.highCutSlope + 1));
}


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

    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings& chainSettings);

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YATBEQAudioProcessor)
};
