#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr int maxSupportedChannels = 64;
static constexpr float positionRange = 30.0f;
static constexpr float smoothingTimeSec = 0.05f;

MultichannelPanoramaAudioProcessor::MultichannelPanoramaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("MultichannelPanoramaState"), createParameterLayout())
{
    // Activate source 0 by default so the plugin has a visible source out-of-the-box
    if (auto* p = parameters.getParameter (ParameterIDs::sourceActive (0)))
        p->setValueNotifyingHost (1.0f);
}

void MultichannelPanoramaAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    for (auto& s : smoothing)
    {
        s.leftGain.reset (sampleRate, smoothingTimeSec);
        s.rightGain.reset (sampleRate, smoothingTimeSec);
        s.distGain.reset (sampleRate, smoothingTimeSec);
    }
    syncSceneFromParameters();
}

void MultichannelPanoramaAudioProcessor::releaseResources() {}

bool MultichannelPanoramaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out.isDisabled())
        return false;
    // Require stereo output (binaural/panning mix-down)
    if (out != juce::AudioChannelSet::stereo())
        return false;
    // Input can be mono or stereo (sources pass through individually)
    const auto& in = layouts.getMainInputChannelSet();
    if (in.isDisabled())
        return false;
    return in.size() <= maxSupportedChannels;
}

void MultichannelPanoramaAudioProcessor::syncSceneFromParameters()
{
    auto load = [&] (const juce::String& id) -> float
    {
        if (const auto* p = parameters.getRawParameterValue (id))
            return p->load();
        return 0.0f;
    };

    sceneAtomics.listener.x.store (load (ParameterIDs::listenerX));
    sceneAtomics.listener.y.store (load (ParameterIDs::listenerY));
    sceneAtomics.listener.z.store (load (ParameterIDs::listenerZ));

    for (int i = 0; i < Panorama::maxSources; ++i)
    {
        sceneAtomics.sources[i].x.store      (load (ParameterIDs::sourceX (i)));
        sceneAtomics.sources[i].y.store      (load (ParameterIDs::sourceY (i)));
        sceneAtomics.sources[i].z.store      (load (ParameterIDs::sourceZ (i)));
        sceneAtomics.sources[i].gainDb.store (load (ParameterIDs::sourceGain (i)));
        sceneAtomics.sources[i].active.store (load (ParameterIDs::sourceActive (i)) >= 0.5f);
    }
}

void MultichannelPanoramaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const auto* bypassParam = parameters.getRawParameterValue (ParameterIDs::bypass);
    if (bypassParam != nullptr && bypassParam->load() >= 0.5f)
        return;

    syncSceneFromParameters();

    const int numOutputChannels = getTotalNumOutputChannels();
    const int numInputChannels  = getTotalNumInputChannels();
    const int numSamples        = buffer.getNumSamples();

    // Mix into stereo output
    juce::AudioBuffer<float> mixBuf (2, numSamples);
    mixBuf.clear();

    const Panorama::Vec3 listenerPos {
        sceneAtomics.listener.x.load(),
        sceneAtomics.listener.y.load(),
        sceneAtomics.listener.z.load()
    };

    // Each active source picks one input channel (channel index = source index)
    for (int i = 0; i < Panorama::maxSources; ++i)
    {
        if (!sceneAtomics.sources[i].active.load())
            continue;

        const int inputCh = i < numInputChannels ? i : 0;

        const Panorama::Vec3 srcPos {
            sceneAtomics.sources[i].x.load(),
            sceneAtomics.sources[i].y.load(),
            sceneAtomics.sources[i].z.load()
        };

        const float gainDb   = sceneAtomics.sources[i].gainDb.load();
        const float gainLin  = juce::Decibels::decibelsToGain (gainDb);
        const auto  rel      = srcPos.relativeTo (listenerPos);
        const float dist     = srcPos.distanceTo (listenerPos);
        const float distGain = Panorama::distanceToLinearGain (dist);
        const auto [leftG, rightG] = Panorama::panFromRelative (rel.x, rel.z);

        auto& sm = smoothing[i];
        sm.distGain.setTargetValue  (distGain * gainLin);
        sm.leftGain.setTargetValue  (leftG);
        sm.rightGain.setTargetValue (rightG);

        const float* src = buffer.getReadPointer (inputCh);
        float* outL = mixBuf.getWritePointer (0);
        float* outR = mixBuf.getWritePointer (1);

        for (int s = 0; s < numSamples; ++s)
        {
            const float dg = sm.distGain.getNextValue();
            const float lg = sm.leftGain.getNextValue();
            const float rg = sm.rightGain.getNextValue();
            outL[s] += src[s] * dg * lg;
            outR[s] += src[s] * dg * rg;
        }
    }

    // Write stereo mix to output buffer
    if (numOutputChannels >= 1)
        buffer.copyFrom (0, 0, mixBuf, 0, 0, numSamples);
    if (numOutputChannels >= 2)
        buffer.copyFrom (1, 0, mixBuf, 1, 0, numSamples);
    for (int ch = 2; ch < numOutputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);
}

void MultichannelPanoramaAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer,
                                                       juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (buffer, midiMessages);
    // Double-precision path: convert to float for now
}

juce::AudioProcessorEditor* MultichannelPanoramaAudioProcessor::createEditor()
{
    return new MultichannelPanoramaEditor (*this);
}

bool MultichannelPanoramaAudioProcessor::hasEditor() const { return true; }

const juce::String MultichannelPanoramaAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MultichannelPanoramaAudioProcessor::acceptsMidi()  const { return false; }
bool MultichannelPanoramaAudioProcessor::producesMidi() const { return false; }
bool MultichannelPanoramaAudioProcessor::isMidiEffect() const { return false; }
double MultichannelPanoramaAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MultichannelPanoramaAudioProcessor::getNumPrograms()   { return 1; }
int MultichannelPanoramaAudioProcessor::getCurrentProgram() { return 0; }

void MultichannelPanoramaAudioProcessor::setCurrentProgram (int) {}

const juce::String MultichannelPanoramaAudioProcessor::getProgramName (int)
{
    return {};
}

void MultichannelPanoramaAudioProcessor::changeProgramName (int, const juce::String&) {}

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

    const auto metres   = juce::AudioParameterFloatAttributes().withLabel ("m");
    const auto decibels = juce::AudioParameterFloatAttributes().withLabel ("dB");
    const juce::NormalisableRange<float> posRange (-positionRange, positionRange, 0.01f);
    const juce::NormalisableRange<float> gainRange (-60.0f, 12.0f, 0.1f);

    // Listener
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerX, 1), "Listener X", posRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerY, 1), "Listener Y", posRange, 0.0f, metres));
    layout.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID (ParameterIDs::listenerZ, 1), "Listener Z", posRange, 0.0f, metres));

    // Sources
    for (int i = 0; i < ParameterIDs::numSources; ++i)
    {
        const float defaultZ = (i == 0) ? 2.0f : 0.0f;

        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (ParameterIDs::sourceX (i), 1),
            "Source " + juce::String (i + 1) + " X", posRange, 0.0f, metres));
        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (ParameterIDs::sourceY (i), 1),
            "Source " + juce::String (i + 1) + " Y", posRange, 0.0f, metres));
        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (ParameterIDs::sourceZ (i), 1),
            "Source " + juce::String (i + 1) + " Z", posRange, defaultZ, metres));
        layout.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (ParameterIDs::sourceGain (i), 1),
            "Source " + juce::String (i + 1) + " Gain", gainRange, 0.0f, decibels));
        layout.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (ParameterIDs::sourceActive (i), 1),
            "Source " + juce::String (i + 1) + " Active", i == 0));
    }

    layout.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID (ParameterIDs::bypass, 1), "Bypass", false));

    return { layout.begin(), layout.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultichannelPanoramaAudioProcessor();
}
