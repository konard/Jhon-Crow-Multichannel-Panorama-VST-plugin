# Proposed Architecture

## Recommended Direction

Use JUCE with CMake and target VST3 first. This gives the project a practical path to FL Studio compatibility, Windows 32/64-bit CI builds, parameter automation, state persistence, and a future custom editor.

The repository now contains a minimal scaffold that proves this direction:

- A JUCE VST3 plugin target named `MultichannelPanorama`.
- Listener and source XYZ parameters.
- Source gain and bypass parameters.
- State save/restore through `AudioProcessorValueTreeState`.
- A generic parameter editor for early host testing.
- A GitHub Actions matrix for `Win32` and `x64` builds.

## Format Strategy

Primary target:

- VST3 for Windows 32-bit and Windows 64-bit.

Deferred target:

- VST2 `.dll`, only if the project can legally provide the legacy VST2 SDK and accepts the maintenance risk.

Reasoning:

- FL Studio supports VST3 on Windows.
- Steinberg's current SDK path is VST3.
- VST3 supports dynamic I/O, sample-accurate automation, 3D speaker configurations, and resizeable editors.
- The issue asks for `.dll` artifacts, but modern VST3 Windows delivery uses `.vst3` packages. Treating `.dll` as a strict VST2 requirement would create a legal and compatibility blocker.

## Core Modules

### Plugin Shell

Responsibilities:

- Own JUCE `AudioProcessor`.
- Declare input/output bus support.
- Expose automatable parameters.
- Save and restore state.
- Bridge host callbacks to the internal scene and renderer.

Current scaffold:

- Pass-through processing with source gain.
- Up to 64 matching input/output channels accepted by bus layout validation.

### Scene Model

Suggested data types:

- `ListenerPose`: X/Y/Z position plus yaw/pitch/roll.
- `SourcePose`: source id, label, X/Y/Z, gain, width, mute/solo, optional automation smoothing state.
- `SceneState`: listener pose, all source poses, selected object id, output mode, room model, units.
- `CoordinateSystem`: conversion between UI axes, Cartesian coordinates, and renderer coordinates.

The scene model should be independent of JUCE UI classes and renderer dependencies so it can be unit-tested.

### Parameter Layer

Use `AudioProcessorValueTreeState` as the first persistent parameter store.

Parameter groups should include:

- Listener: X, Y, Z, yaw, pitch, roll.
- Selected source: X, Y, Z, azimuth, elevation, distance, gain, width.
- Output: binaural, ambisonic order, speaker layout, master gain.
- Room/reflections: room size, wall distances, reflection amount, reverb send, occlusion.
- Performance: quality mode, interpolation mode, static-source optimization.

Fast position movement must be smoothed on the audio thread to prevent zipper noise.

### Renderer

The renderer should start simple and be replaceable:

1. MVP: gain-preserving pass-through plus panning/attenuation experiments.
2. Prototype: object-to-binaural rendering using an HRTF-capable library.
3. Production: object/HOA renderer with output modes for stereo binaural, Ambisonics, and multichannel layouts.

Candidate renderer approaches:

- Steam Audio C API for HRTF, occlusion, and reflections.
- libspatialaudio for object/HOA/speaker/binaural rendering.
- A custom Ambisonics path using IEM/SPARTA concepts as references.

Renderer constraints:

- No heap allocation on the audio thread.
- No locks on the audio thread.
- Position updates must be double-buffered or atomically transferred.
- HRTF changes and convolution resources must be prepared outside real-time processing.

### UI

The eventual UI should include:

- Main XYZ pad with draggable source/listener dots.
- Dot labels for channel/source names.
- Selection state for source or listener.
- X/Z, Y/Z, and X/Y display modes.
- Numeric sliders/fields for X, Y, Z.
- Optional polar controls for azimuth, elevation, and distance.
- Zoom levels matching practical room scales.
- Output mode and quality controls.

DearVR-like behavior should be implemented as interaction patterns, not copied artwork.

### Multi-Source Operation in FL Studio

There are three viable designs:

1. One source plugin per mixer channel.
   - Each instance spatializes its own channel.
   - Easiest DAW routing.
   - Shared listener/global scene requires inter-instance state.

2. Central renderer plugin with sidechain/multibus inputs.
   - One UI can show every routed source.
   - Audio routing depends heavily on FL Studio's VST3 bus/sidechain behavior.
   - Source names may not map cleanly from the host.

3. Hybrid source plugins plus central monitor/scene plugin.
   - Source instances expose per-channel controls.
   - A central plugin manages listener/global view.
   - Audio still needs normal DAW routing; avoid unsupported inter-plugin audio transport.

Recommended path:

- Build MVP as one source plugin per channel with listener/source controls.
- Add shared scene state only after validating FL Studio instance behavior.
- Investigate a central renderer only after confirming FL Studio VST3 sidechain bus limits.

## CI and Artifact Architecture

The workflow has two layers:

- `validate-preparation`: runs on Ubuntu, checks required files, and runs a no-fetch CMake configure smoke test.
- `build-windows-vst3`: runs on Windows with Visual Studio 2022 for `Win32` and `x64`, builds `MultichannelPanorama_VST3`, stages the copied VST3 package, and uploads one artifact per architecture.

Future CI additions:

- Run Steinberg VST3 validator.
- Run pluginval at strictness level 5 or higher.
- Add C++ unit tests for scene math and parameter smoothing.
- Add FL Studio manual smoke-test notes after the first real UI/renderer milestone.
