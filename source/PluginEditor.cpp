#include "PluginEditor.h"

//==============================================================================
// Colour palette
//==============================================================================
namespace Colors
{
static const juce::Colour background   { 0xff1a1a2e };
static const juce::Colour gridLine     { 0xff2a2a4a };
static const juce::Colour gridText     { 0xff5555aa };
static const juce::Colour listener     { 0xff44aaff };
static const juce::Colour source0     { 0xffff6644 };
static const juce::Colour source1     { 0xff44ff88 };
static const juce::Colour source2     { 0xffff44cc };
static const juce::Colour source3     { 0xffffcc44 };
static const juce::Colour source4     { 0xff44ccff };
static const juce::Colour source5     { 0xffcc44ff };
static const juce::Colour source6     { 0xffff8844 };
static const juce::Colour source7     { 0xff88ff44 };

static juce::Colour forSource (int i)
{
    switch (i)
    {
        case 0: return source0;
        case 1: return source1;
        case 2: return source2;
        case 3: return source3;
        case 4: return source4;
        case 5: return source5;
        case 6: return source6;
        case 7: return source7;
        default: return juce::Colours::white;
    }
}
}

//==============================================================================
// SpatialPad
//==============================================================================
SpatialPad::SpatialPad (MultichannelPanoramaAudioProcessor& p)
    : proc (p)
{
}

juce::Point<float> SpatialPad::sceneToScreen (float horiz, float vert) const
{
    const auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float scale = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f / sceneExtent;
    return { cx + horiz * scale, cy - vert * scale };   // vert inverted so +Z = up on screen
}

std::pair<float, float> SpatialPad::screenToScene (juce::Point<float> pt) const
{
    const auto bounds = getLocalBounds().toFloat().reduced (8.0f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float scale = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f / sceneExtent;
    const float horiz = (pt.x - cx) / scale;
    const float vert  = -(pt.y - cy) / scale;
    return { juce::jlimit (-sceneExtent, sceneExtent, horiz),
             juce::jlimit (-sceneExtent, sceneExtent, vert) };
}

std::vector<SpatialPad::DotInfo> SpatialPad::buildDots() const
{
    std::vector<DotInfo> dots;

    auto& at = proc.getSceneAtomics();

    // Listener
    const float lx = at.listener.x.load();
    const float ly = at.listener.y.load();
    const float lz = at.listener.z.load();

    const float listH = (viewPlane == ViewPlane::XZ) ? lx : lx;
    const float listV = (viewPlane == ViewPlane::XZ) ? lz : ly;
    DotInfo listDot;
    listDot.centre    = sceneToScreen (listH, listV);
    listDot.colour    = Colors::listener;
    listDot.label     = "L";
    listDot.isListener = true;
    dots.push_back (listDot);

    // Sources
    for (int i = 0; i < Panorama::maxSources; ++i)
    {
        if (!at.sources[i].active.load())
            continue;

        const float sx = at.sources[i].x.load();
        const float sy = at.sources[i].y.load();
        const float sz = at.sources[i].z.load();

        const float horiz = (viewPlane == ViewPlane::XZ) ? sx : sx;
        const float vert  = (viewPlane == ViewPlane::XZ) ? sz : sy;

        DotInfo dot;
        dot.centre      = sceneToScreen (horiz, vert);
        dot.colour      = Colors::forSource (i);
        dot.label       = juce::String (i + 1);
        dot.sourceIndex = i;
        dots.push_back (dot);
    }

    return dots;
}

void SpatialPad::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour (Colors::background.brighter (0.05f));
    g.fillRoundedRectangle (bounds, 6.0f);

    // Grid
    g.setColour (Colors::gridLine);
    const auto inner = bounds.reduced (8.0f);
    const float cx   = inner.getCentreX();
    const float cy   = inner.getCentreY();
    g.drawLine (cx, inner.getY(), cx, inner.getBottom(), 1.0f);
    g.drawLine (inner.getX(), cy, inner.getRight(), cy, 1.0f);

    // Concentric rings at 5 m intervals
    const float scale = std::min (inner.getWidth(), inner.getHeight()) * 0.5f / sceneExtent;
    for (float r = 5.0f; r <= sceneExtent; r += 5.0f)
    {
        const float px = r * scale;
        g.drawEllipse (cx - px, cy - px, px * 2.0f, px * 2.0f, 0.5f);
    }

    // Axis labels
    g.setColour (Colors::gridText);
    g.setFont (10.0f);
    const juce::String horizLabel = "X";
    const juce::String vertLabel  = (viewPlane == ViewPlane::XZ) ? "Z" : "Y";
    g.drawText ("+" + horizLabel, (int)inner.getRight() - 14, (int)cy - 6, 14, 12, juce::Justification::right);
    g.drawText ("+" + vertLabel,  (int)cx + 2,  (int)inner.getY(),  14, 12, juce::Justification::left);

    // Dots
    for (const auto& dot : buildDots())
    {
        const float r = dotRadius;
        const auto  c = dot.centre;

        // Drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillEllipse (c.x - r + 1.5f, c.y - r + 1.5f, r * 2.0f, r * 2.0f);

        // Fill
        g.setColour (dot.colour);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // Outline
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 1.5f);

        // Label
        g.setColour (juce::Colours::white);
        g.setFont (juce::Font (juce::FontOptions().withHeight (10.0f).withStyle ("Bold")));
        g.drawText (dot.label,
                    (int)(c.x - r), (int)(c.y - r),
                    (int)(r * 2.0f), (int)(r * 2.0f),
                    juce::Justification::centred);
    }

    // Border
    g.setColour (Colors::gridLine.brighter (0.3f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void SpatialPad::mouseDown (const juce::MouseEvent& e)
{
    dragTarget = -2;
    const auto pt = e.position;

    for (const auto& dot : buildDots())
    {
        if (pt.getDistanceFrom (dot.centre) <= dotRadius + 4.0f)
        {
            dragTarget = dot.isListener ? -1 : dot.sourceIndex;
            break;
        }
    }
}

void SpatialPad::mouseDrag (const juce::MouseEvent& e)
{
    if (dragTarget == -2)
        return;
    applyDrag (e.position);
    repaint();
}

void SpatialPad::mouseUp (const juce::MouseEvent&)
{
    dragTarget = -2;
}

void SpatialPad::applyDrag (juce::Point<float> screenPt)
{
    auto [horiz, vert] = screenToScene (screenPt);

    auto setParam = [&] (const juce::String& id, float val)
    {
        if (auto* p = proc.getParameterTree().getParameter (id))
        {
            const auto range = proc.getParameterTree().getParameterRange (id);
            p->setValueNotifyingHost (range.convertTo0to1 (val));
        }
    };

    if (dragTarget == -1)
    {
        // Listener
        setParam (ParameterIDs::listenerX, horiz);
        if (viewPlane == ViewPlane::XZ)
            setParam (ParameterIDs::listenerZ, vert);
        else
            setParam (ParameterIDs::listenerY, vert);
    }
    else if (dragTarget >= 0)
    {
        // Source
        setParam (ParameterIDs::sourceX (dragTarget), horiz);
        if (viewPlane == ViewPlane::XZ)
            setParam (ParameterIDs::sourceZ (dragTarget), vert);
        else
            setParam (ParameterIDs::sourceY (dragTarget), vert);
    }
}

void SpatialPad::setViewPlane (ViewPlane vp)
{
    viewPlane = vp;
    repaint();
}

//==============================================================================
// SourceControlStrip
//==============================================================================
SourceControlStrip::SourceControlStrip (MultichannelPanoramaAudioProcessor& p)
    : proc (p)
{
    titleLabel.setFont (juce::Font (juce::FontOptions().withHeight (13.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (titleLabel);

    auto setupSlider = [&] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, juce::Colours::lightgrey);
        l.setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
        addAndMakeVisible (s);
        addAndMakeVisible (l);
    };

    setupSlider (xSlider, xLabel, "X (m)");
    setupSlider (ySlider, yLabel, "Y (m)");
    setupSlider (zSlider, zLabel, "Z (m)");
    setupSlider (gainSlider, gainLabel, "Gain (dB)");

    setSelectedSource (0);
}

void SourceControlStrip::setSelectedSource (int index)
{
    selectedSource = index;
    xAtt.reset();
    yAtt.reset();
    zAtt.reset();
    gainAtt.reset();

    auto& tree = proc.getParameterTree();

    if (index == -1)
    {
        titleLabel.setText ("Listener", juce::dontSendNotification);
        xAtt   = std::make_unique<Attachment> (tree, ParameterIDs::listenerX, xSlider);
        yAtt   = std::make_unique<Attachment> (tree, ParameterIDs::listenerY, ySlider);
        zAtt   = std::make_unique<Attachment> (tree, ParameterIDs::listenerZ, zSlider);
        gainSlider.setEnabled (false);
        gainLabel.setEnabled (false);
    }
    else
    {
        titleLabel.setText ("Source " + juce::String (index + 1), juce::dontSendNotification);
        xAtt   = std::make_unique<Attachment> (tree, ParameterIDs::sourceX (index),    xSlider);
        yAtt   = std::make_unique<Attachment> (tree, ParameterIDs::sourceY (index),    ySlider);
        zAtt   = std::make_unique<Attachment> (tree, ParameterIDs::sourceZ (index),    zSlider);
        gainAtt = std::make_unique<Attachment> (tree, ParameterIDs::sourceGain (index), gainSlider);
        gainSlider.setEnabled (true);
        gainLabel.setEnabled (true);
    }
}

void SourceControlStrip::resized()
{
    auto area = getLocalBounds().reduced (4);
    titleLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (2);

    const int rowH = 24;
    const int labelW = 58;

    auto placeRow = [&] (juce::Label& lbl, juce::Slider& s)
    {
        auto row = area.removeFromTop (rowH);
        lbl.setBounds (row.removeFromLeft (labelW));
        s.setBounds (row);
        area.removeFromTop (2);
    };

    placeRow (xLabel, xSlider);
    placeRow (yLabel, ySlider);
    placeRow (zLabel, zSlider);
    placeRow (gainLabel, gainSlider);
}

//==============================================================================
// MultichannelPanoramaEditor
//==============================================================================
MultichannelPanoramaEditor::MultichannelPanoramaEditor (MultichannelPanoramaAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProc (p),
      spatialPad (p),
      controlStrip (p)
{
    setSize (680, 480);
    setResizable (true, true);
    setResizeLimits (520, 400, 1280, 960);

    // Title
    titleLabel.setText ("Multichannel Panorama", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions().withHeight (16.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    // View plane buttons
    viewXZButton.setClickingTogglesState (false);
    viewXYButton.setClickingTogglesState (false);
    viewXZButton.onClick = [this] { spatialPad.setViewPlane (SpatialPad::ViewPlane::XZ); };
    viewXYButton.onClick = [this] { spatialPad.setViewPlane (SpatialPad::ViewPlane::XY); };
    addAndMakeVisible (viewXZButton);
    addAndMakeVisible (viewXYButton);

    // Source selector buttons
    listenerButton.onClick = [this] { selectSource (-1); };
    addAndMakeVisible (listenerButton);

    for (int i = 0; i < Panorama::maxSources; ++i)
    {
        sourceButtons[i].setButtonText (juce::String (i + 1));
        sourceButtons[i].onClick = [this, i] { selectSource (i); };
        addAndMakeVisible (sourceButtons[i]);
    }

    addAndMakeVisible (spatialPad);
    addAndMakeVisible (controlStrip);

    selectSource (0);
    startTimerHz (20);
}

MultichannelPanoramaEditor::~MultichannelPanoramaEditor()
{
    stopTimer();
}

void MultichannelPanoramaEditor::timerCallback()
{
    spatialPad.repaint();
}

void MultichannelPanoramaEditor::selectSource (int index)
{
    selectedSource = index;
    controlStrip.setSelectedSource (index);
}

void MultichannelPanoramaEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colors::background);
}

void MultichannelPanoramaEditor::resized()
{
    auto area = getLocalBounds().reduced (8);

    // Title bar
    auto topBar = area.removeFromTop (28);
    titleLabel.setBounds (topBar.removeFromLeft (240));

    // View buttons on the right of title
    topBar.removeFromLeft (8);
    viewXZButton.setBounds (topBar.removeFromLeft (90));
    topBar.removeFromLeft (4);
    viewXYButton.setBounds (topBar.removeFromLeft (90));

    area.removeFromTop (6);

    // Source selector strip
    auto selectorRow = area.removeFromTop (28);
    listenerButton.setBounds (selectorRow.removeFromLeft (70));
    selectorRow.removeFromLeft (4);
    const int btnW = 32;
    for (int i = 0; i < Panorama::maxSources; ++i)
    {
        sourceButtons[i].setBounds (selectorRow.removeFromLeft (btnW));
        selectorRow.removeFromLeft (2);
    }

    area.removeFromTop (6);

    // Right panel: control strip
    auto rightPanel = area.removeFromRight (230);
    controlStrip.setBounds (rightPanel);

    area.removeFromRight (6);

    // Spatial pad takes remaining space
    spatialPad.setBounds (area);
}
