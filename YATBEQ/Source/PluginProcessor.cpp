/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
YATBEQAudioProcessor::YATBEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

YATBEQAudioProcessor::~YATBEQAudioProcessor()
{
}

//==============================================================================
const juce::String YATBEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool YATBEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool YATBEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool YATBEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double YATBEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int YATBEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int YATBEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void YATBEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String YATBEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void YATBEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void YATBEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    // initialize filters with default settings
    updateFilters();
}

void YATBEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool YATBEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void YATBEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // make updates
    updateFilters();

    // run audio
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool YATBEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* YATBEQAudioProcessor::createEditor()
{
    //return new YATBEQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void YATBEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void YATBEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new YATBEQAudioProcessor();
    
}

//==============================================================================
//
// 
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout YATBEQAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout rtn;
    float startParamValue = 20.f;
    const float skew = 0.25f;
    rtn.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq", "LowCut Freq", 
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, skew),
        startParamValue));
    startParamValue = 20000.f;
    rtn.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq", "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, skew),
        startParamValue));
    startParamValue = 750.f;
    rtn.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq", "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, skew),
        startParamValue));
    startParamValue = 0.f;
    rtn.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain", "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.05f, 1.f),
        startParamValue));
    startParamValue = 1.f;
    rtn.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality", "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        startParamValue));

    juce::StringArray cutAmountChoices;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/oct";
        cutAmountChoices.add(str);
    }

    rtn.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope",
        cutAmountChoices, 0));
    rtn.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope",
        cutAmountChoices, 0));

    return rtn;
}

ChainSettings getTreeStateChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings rtn;

    rtn.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    rtn.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    rtn.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    rtn.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    rtn.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();

    // around 1:00:00 in the video, Chuck talks about these integer values, slope choices
    // the values are the (cutAmountChoices above) string array indexes in this case {0, 1, 2, 3}
    // which is why 2 * (chainSettings.lowCutSlope + 1) is correct for the filter order
    // parameter calculation in prepareToPlay()
    rtn.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    rtn.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return rtn;
}

void YATBEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void YATBEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

void YATBEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        getSampleRate(),
        2 * (chainSettings.lowCutSlope + 1));

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void YATBEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        getSampleRate(),
        2 * (chainSettings.highCutSlope + 1));

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void YATBEQAudioProcessor::updateFilters()
{
    auto chainSettings = getTreeStateChainSettings(apvts);
    updatePeakFilter(chainSettings);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
}