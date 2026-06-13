#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "SceneState.h"

namespace ParameterIDs
{
// Listener
constexpr auto listenerX = "listenerX";
constexpr auto listenerY = "listenerY";
constexpr auto listenerZ = "listenerZ";

// Sources (8 sources, each with X/Y/Z and gain)
constexpr int numSources = Panorama::maxSources;

inline juce::String sourceX (int i)    { return "source" + juce::String (i) + "X"; }
inline juce::String sourceY (int i)    { return "source" + juce::String (i) + "Y"; }
inline juce::String sourceZ (int i)    { return "source" + juce::String (i) + "Z"; }
inline juce::String sourceGain (int i) { return "source" + juce::String (i) + "GainDb"; }
inline juce::String sourceActive (int i)  { return "source" + juce::String (i) + "Active"; }
inline juce::String sourceBypass (int i)  { return "source" + juce::String (i) + "Bypass"; }

constexpr auto bypass = "bypass";
}

class MultichannelPanoramaAudioProcessor final : public juce::AudioProcessor
{
public:
    using ParameterTree = juce::AudioProcessorValueTreeState;

    MultichannelPanoramaAudioProcessor();
    ~MultichannelPanoramaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    ParameterTree& getParameterTree() noexcept { return parameters; }
    const ParameterTree& getParameterTree() const noexcept { return parameters; }

    // Scene state for UI access
    Panorama::SceneAtomics& getSceneAtomics() noexcept { return sceneAtomics; }

    static ParameterTree::ParameterLayout createParameterLayout();

private:
    void syncSceneFromParameters();
    void autoActivateSources();

    ParameterTree parameters;
    Panorama::SceneAtomics sceneAtomics;

    // Smoothed gains for each source (left/right) to avoid zipper noise
    struct SourceSmoothing
    {
        juce::SmoothedValue<float> leftGain;
        juce::SmoothedValue<float> rightGain;
        juce::SmoothedValue<float> distGain;
    };
    std::array<SourceSmoothing, Panorama::maxSources> smoothing;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultichannelPanoramaAudioProcessor)
};
