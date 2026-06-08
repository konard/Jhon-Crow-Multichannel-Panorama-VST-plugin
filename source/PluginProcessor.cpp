#include "PluginProcessor.h"

namespace
{
constexpr int maxSupportedChannels = 64;

template <typename SampleType>
void clearUnusedOutputs (juce::AudioBuffer<SampleType>& buffer, int inputChannels, int outputChannels)
{
    for (auto channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());
}

template <typename SampleType>
void applyScaffoldProcessing (juce::AudioBuffer<SampleType>& buffer,
                              int inputChannels,
                              int outputChannels,
                              juce::AudioProcessorValueTreeState& parameters)
{
    clearUnusedOutputs (buffer, inputChannels, outputChannels);

    const auto* bypassParam = parameters.getRawParameterValue (ParameterIDs::bypass);
    if (bypassParam != nullptr && bypassParam->load() >= 0.5f)
        return;

    const auto* gainParam = parameters.getRawParameterValue (ParameterIDs::source1GainDb);
    const auto gainDb = gainParam != nullptr ? gainParam->load() : 0.0f;
    buffer.applyGain (static_cast<SampleType> (juce::Decibels::decibelsToGain (gainDb)));
}
}

MultichannelPanoramaAudioProcessor::MultichannelPanoramaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("MultichannelPanoramaState"), createParameterLayout())
{
}

void MultichannelPanoramaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void MultichannelPanoramaAudioProcessor::releaseResources()
{
}

bool MultichannelPanoramaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto inputLayout = layouts.getMainInputChannelSet();
    const auto outputLayout = layouts.getMainOutputChannelSet();

    if (inputLayout.isDisabled() || outputLayout.isDisabled())
        return false;

    if (inputLayout != outputLayout)
        return false;

    const auto channels = outputLayout.size();
    return channels > 0 && channels <= maxSupportedChannels;
}

void MultichannelPanoramaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    applyScaffoldProcessing (buffer, getTotalNumInputChannels(), getTotalNumOutputChannels(), parameters);
}

void MultichannelPanoramaAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    applyScaffoldProcessing (buffer, getTotalNumInputChannels(), getTotalNumOutputChannels(), parameters);
}

juce::AudioProcessorEditor* MultichannelPanoramaAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

bool MultichannelPanoramaAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String MultichannelPanoramaAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MultichannelPanoramaAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MultichannelPanoramaAudioProcessor::producesMidi() const
{
    return false;
}

bool MultichannelPanoramaAudioProcessor::isMidiEffect() const
{
    return false;
}

double MultichannelPanoramaAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MultichannelPanoramaAudioProcessor::getNumPrograms()
{
    return 1;
}

int MultichannelPanoramaAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MultichannelPanoramaAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String MultichannelPanoramaAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void MultichannelPanoramaAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void MultichannelPanoramaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = parameters.copyState();
    if (const auto xml = state.createXml(); xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void MultichannelPanoramaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (const auto xml = getXmlFromBinary (data, sizeInBytes);
        xml != nullptr && xml->hasTagName (parameters.state.getType()))
    {
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

MultichannelPanoramaAudioProcessor::ParameterTree::ParameterLayout
MultichannelPanoramaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;

    const auto metres = juce::AudioParameterFloatAttributes().withLabel ("m");
    const auto decibels = juce::AudioParameterFloatAttributes().withLabel ("dB");
    const auto positionRange = juce::NormalisableRange<float> (-30.0f, 30.0f, 0.01f);
    const auto gainRange = juce::NormalisableRange<float> (-96.0f, 24.0f, 0.01f);

    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerX, 1), "Listener X", positionRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerY, 1), "Listener Y", positionRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerZ, 1), "Listener Z", positionRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::source1X, 1), "Source 1 X", positionRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::source1Y, 1), "Source 1 Y", positionRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::source1Z, 1), "Source 1 Z", positionRange, 1.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::source1GainDb, 1), "Source 1 Gain", gainRange, 0.0f, decibels));
    layout.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ParameterIDs::bypass, 1), "Bypass", false));

    return { layout.begin(), layout.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultichannelPanoramaAudioProcessor();
}
