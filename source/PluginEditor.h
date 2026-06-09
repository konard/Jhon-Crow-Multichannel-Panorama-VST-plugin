#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "SceneState.h"

// ============================================================
// SpatialPad – the 2-D draggable scene view
// Displays listener + sources as draggable labelled dots.
// Supports top-down (X/Z) and front-view (X/Y) plane modes.
// ============================================================
class SpatialPad final : public juce::Component
{
public:
    enum class ViewPlane { XZ, XY };

    explicit SpatialPad (MultichannelPanoramaAudioProcessor& p);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;

    void setViewPlane (ViewPlane vp);

private:
    struct DotInfo
    {
        juce::Point<float> centre;
        juce::Colour colour;
        juce::String label;
        bool isListener = false;
        int sourceIndex = -1;
    };

    // Convert scene metres to pad pixels
    juce::Point<float> sceneToScreen (float horiz, float vert) const;
    // Convert pad pixels back to scene metres
    std::pair<float, float> screenToScene (juce::Point<float> pt) const;

    std::vector<DotInfo> buildDots() const;
    void applyDrag (juce::Point<float> screenPt);

    MultichannelPanoramaAudioProcessor& proc;
    ViewPlane viewPlane = ViewPlane::XZ;

    int dragTarget = -2;   // -1 = listener, 0..7 = source index, -2 = none
    static constexpr float dotRadius = 10.0f;
    static constexpr float sceneExtent = 30.0f;   // metres shown from centre
};

// ============================================================
// SourceControlStrip – sliders for a single selected source
// ============================================================
class SourceControlStrip final : public juce::Component
{
public:
    explicit SourceControlStrip (MultichannelPanoramaAudioProcessor& p);

    void setSelectedSource (int index);   // -1 = listener
    void resized() override;

private:
    MultichannelPanoramaAudioProcessor& proc;
    int selectedSource = 0;

    juce::Label titleLabel;

    juce::Label xLabel, yLabel, zLabel, gainLabel;
    juce::Slider xSlider, ySlider, zSlider, gainSlider;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> xAtt, yAtt, zAtt, gainAtt;
};

// ============================================================
// MultichannelPanoramaEditor – the main plugin window
// ============================================================
class MultichannelPanoramaEditor final : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    explicit MultichannelPanoramaEditor (MultichannelPanoramaAudioProcessor&);
    ~MultichannelPanoramaEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void timerCallback() override;

private:
    MultichannelPanoramaAudioProcessor& audioProc;

    SpatialPad spatialPad;
    SourceControlStrip controlStrip;

    juce::TextButton viewXZButton { "Top (X/Z)" };
    juce::TextButton viewXYButton { "Front (X/Y)" };

    // Source selector buttons
    std::array<juce::TextButton, Panorama::maxSources> sourceButtons;
    juce::TextButton listenerButton { "Listener" };

    juce::Label titleLabel;

    int selectedSource = 0;   // -1 = listener

    void selectSource (int index);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultichannelPanoramaEditor)
};
