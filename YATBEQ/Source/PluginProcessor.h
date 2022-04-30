/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
            "prepare(numChannels, numSamples) should only be called when the FIFO is holding juce::AudioBuffer<float>");

	    for (auto& buffer : buffers)
	    {
            buffer.setSize(numChannels, numSamples, false, true, true);
            buffer.clear();
	    }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
            "prepare(numSamples) should only be called when the FIFO is holding std::vector<float>");

        for (auto& buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if(write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        return false;
    }

    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if(read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }
        return false;
    }

    private:
        static constexpr int Capacity = 30;
        std::array<T, Capacity> buffers;
        juce::AbstractFifo fifo{ Capacity };
};

enum Channel
{
	Right,// effectively 0
    Left  // effectively 1
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
	SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
	{
        prepared.set(false);
	}

    void update(const BlockType& buffer)
	{
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for(int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFiFo(channelPtr[i]);
        }
	}

    void prepare(int bufferSize)
	{
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1, bufferSize, false, true, true);
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
        prepared.set(true);
	}
    //=========================================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==========================================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
	    if(fifoIndex == bufferToFill.getNumSamples())
	    {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);
            fifoIndex = 0;
	    }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

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

    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo <BlockType> leftChannelFifo{ Channel::Left };
    SingleChannelSampleFifo <BlockType> rightChannelFifo{ Channel::Right };

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
