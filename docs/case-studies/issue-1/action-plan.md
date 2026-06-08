# Future Action Plan and Risks

## Completed in This Preparation Pass

- Created the issue case-study folder.
- Captured product, host, SDK, build, and component research.
- Added a JUCE/CMake VST3 scaffold.
- Added a Windows GitHub Actions build matrix for 32-bit and 64-bit VST3 artifacts.
- Added a validation script for required preparation assets.

## Phase 1: Validate Toolchain

- Run GitHub Actions on the PR branch and inspect artifact output paths.
- Confirm the `Win32` and `x64` VST3 artifacts contain loadable Windows plugin packages.
- Add Steinberg VST3 validator or pluginval once the CI artifact path is stable.
- Decide whether the project needs only VST3 or also a legally supplied VST2 build.

## Phase 2: Scene and Math Core

- Implement `ListenerPose`, `SourcePose`, `SceneState`, and coordinate conversions.
- Add unit tests for Cartesian-to-polar conversion, distance calculations, clamping, and smoothing.
- Define exact axis conventions for UI, host parameters, and renderer coordinates.
- Add serialization tests for project/session recall.

## Phase 3: MVP Plugin Behavior

- Support one plugin instance as one spatial source.
- Expose source/listener XYZ controls as automatable parameters.
- Add smoothed gain/distance attenuation.
- Verify mono, stereo, and multichannel layouts in pluginval and at least one DAW host.

## Phase 4: Custom UI

- Replace the generic parameter editor with a custom JUCE editor.
- Build a draggable XYZ pad with source/listener dots and labels.
- Add X/Z, Y/Z, and X/Y display modes.
- Add numeric controls for the selected object.
- Add keyboard/mouse reset behavior and zoom levels.

## Phase 5: Spatial Renderer

- Prototype binaural rendering with a candidate HRTF engine.
- Compare Steam Audio, libspatialaudio, and a custom Ambisonics path.
- Add resource lifecycle tests for HRTF loading and renderer reconfiguration.
- Add CPU and denormal tests for moving-source automation.

## Phase 6: FL Studio-Specific Validation

- Test 64-bit FL Studio with the 64-bit VST3 artifact.
- Test 32-bit artifact only if a supported FL Studio/bridge scenario is required.
- Verify plugin scanning, names, state recall, automation recording/playback, and bridged behavior.
- Investigate whether FL Studio exposes enough VST3 sidechain buses for a central multi-source renderer.

## Phase 7: Packaging and Release

- Decide installer format for Windows.
- Add artifact signing if distribution requires it.
- Add release workflow once builds and validation are stable.
- Document install paths and FL Studio scan steps.

## Known Risks

| Risk | Impact | Mitigation |
| --- | --- | --- |
| Strict `.dll` requirement implies VST2 | VST2 SDK access is legally constrained and deprecated. | Default to VST3; only add VST2 if legal SDK access is supplied. |
| 32-bit support | Modern DAWs and dependencies may prioritize 64-bit. | Keep Win32 CI for now; drop only after explicit project decision. |
| FL Studio routing limits | A single plugin may not automatically receive arbitrary mixer channels. | Validate sidechain/multibus behavior early; prefer one-source-per-instance MVP. |
| Inter-instance communication | Shared listener/source scene can race or break recall. | Keep scene state explicit and host-automatable; avoid audio transport between plugin instances. |
| HRTF licensing | HRTF datasets and engines can have restrictive licenses. | Review dependency and dataset licenses before embedding. |
| Real-time safety | Spatial renderers can allocate, lock, or reload filters unsafely. | Enforce no allocation/locks in process block; test with automation stress cases. |
| UI complexity | DearVR-like UI requires careful 3D interaction and scaling. | Build small reusable UI components and add visual/manual verification. |
| CPU usage | Multi-source HRTF/reflection rendering can be expensive. | Add quality modes, static-source optimization, and profiling from the prototype phase. |
