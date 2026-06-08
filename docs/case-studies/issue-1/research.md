# Research Notes

Research date: 2026-06-08

## Product Reference: Dear Reality dearVR PRO 2

The requested interaction model is closest to Dear Reality's dearVR PRO 2 position module:

- It supports Cartesian and polar positioning modes.
- Cartesian mode exposes X, Y, and Z distances from the listener.
- The source icon can be dragged in an XYZ pad, with numeric entry also available.
- Polar mode exposes azimuth, elevation, and distance sliders.
- The display can switch between X/Z, Y/Z, and X/Y views.
- It models more than static panning: distance, listener-relative angle, elevation, reflections, reverb, output formats, stereo width, and head tracking are part of the full product concept.

Primary source:

- Dear Reality / Plugin Alliance manual, `dearVR PRO 2 User Manual V2.0.0`, January 2024: https://files.plugin-alliance.com/products/dearvr-pro-2/dearvr-pro-2_manual.pdf

## FL Studio Format and Bitness Facts

Image-Line documents that FL Studio on Windows supports 32-bit and 64-bit VST 1/2, VST3, CLAP, and its native plugin format. It also documents plugin bridging for bitness mismatches and recommends using 64-bit plugins in recent FL Studio versions.

Important packaging details for this project:

- VST2 plugins on Windows are `.dll` files.
- VST3 plugins use `.vst3` packages and must be installed in VST3-specific folders.
- On 64-bit Windows, 32-bit VST3 plugins go under `Program Files (x86)` VST3 locations.
- Modern FL Studio can bridge mismatched plugin bitness on Windows, but bridging adds overhead.

Primary sources:

- FL Studio plugin standards: https://www.image-line.com/fl-studio-learning/fl-studio-beta-online-manual/html/plugins_supported.htm
- FL Studio external plugin installation and locations: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm

## VST SDK and Licensing Facts

Steinberg's current VST3 SDK is the recommended target:

- VST3 is available under the MIT license starting with VST3 SDK 3.8.
- VST2 is discontinued.
- VST3 supports dynamic I/O, sample-accurate automation, resizeable editors, channel context information, and 3D speaker configurations such as Ambisonics and Atmos.
- Steinberg describes Windows VST plugins as DLL-based components, with modern VST3 delivery packaged under the `.vst3` convention.

Primary sources:

- Steinberg VST3 licensing page: https://steinbergmedia.github.io/vst3_dev_portal/pages/VST%2B3%2BLicensing/Index.html
- Steinberg VST3 SDK repository: https://github.com/steinbergmedia/vst3sdk
- Steinberg VST2 discontinuation notice: https://helpcenter.steinberg.de/hc/en-us/articles/4409561018258-VST-2-Discontinued

## Build Framework

JUCE is the most pragmatic scaffold for this repository:

- It supports building VST, VST3, AU, AUv3, AAX, and LV2 plugins from one codebase.
- It includes a CMake API, including `juce_add_plugin`.
- Recent JUCE versions include the files needed to create VST3 plugins.
- JUCE packaging documentation lists the Windows VST3 common-files installation path and notes the `Program Files (x86)` suffix for 32-bit plugins on 64-bit Windows.

Primary sources:

- JUCE features: https://juce.com/juce/features/
- JUCE CMake API: https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
- JUCE basic plugin tutorial: https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/
- JUCE plugin packaging tutorial: https://juce.com/tutorials/tutorial_app_plugin_packaging/

Dependency version selected:

- JUCE 8.0.13, latest GitHub release observed on 2026-06-08.

## GitHub Actions Artifacts

GitHub Actions artifacts are appropriate for publishing CI-built plugin packages. GitHub's docs describe artifacts as files or file collections produced during workflow runs, and the upload-artifact action provides the upload mechanism. The prepared workflow uses `actions/upload-artifact@v7`, the latest major release observed on 2026-06-08.

Primary sources:

- GitHub workflow artifacts concept: https://docs.github.com/en/actions/concepts/workflows-and-actions/workflow-artifacts
- GitHub artifact storage tutorial: https://docs.github.com/en/actions/tutorials/store-and-share-data
- upload-artifact action repository: https://github.com/actions/upload-artifact

## Candidate Spatial Audio Components

These are useful references or possible dependencies for later implementation. Each needs license and maintenance review before integration.

| Component | Use | Notes |
| --- | --- | --- |
| JUCE | Plugin wrapper, UI, parameters, CMake | Best default foundation for this repo. |
| Steinberg VST3 SDK | Format standard, validator, host/test tools | Required ecosystem knowledge even when using JUCE wrapper. |
| Steam Audio | HRTF, binaural rendering, occlusion, reflections, C/C++ SDK | Strong feature match; license and binary distribution details need review. Source: https://partner.steamgames.com/doc/features/steam_audio |
| libspatialaudio | Cross-platform C++ spatial renderer for HOA, objects, speaker layouts, binaural output | Good candidate for object/HOA renderer research. Source: https://github.com/videolan/libspatialaudio |
| IEM Plug-in Suite | Ambisonic reference implementation and workflow ideas | GPLv3; likely reference-only unless project adopts compatible licensing. Source: https://plugins.iem.at/ |
| SPARTA / SAF | Open-source spatial audio plugins and Spatial Audio Framework | Useful for high-order Ambisonics and SOFA/head-tracking ideas. Source: https://leomccormack.github.io/sparta-site/ |
| Resonance Audio | Open-source spatial audio SDK with DAW tools | Apache-2.0, but appears older/less active; use as reference, not first dependency. Source: https://github.com/resonance-audio/resonance-audio |
| SOFA | Standard file format for HRTFs/BRIRs/SRIRs | Important for personalized HRTF support. Source: https://sofacoustics.org/ |
| pluginval | Plugin compatibility and stress validation | Recommended future CI addition after producing stable plugin artifacts. Source: https://github.com/Tracktion/pluginval |

## Practical Implications

- A VST3-first route gives the cleanest legal and build path.
- The issue's request for 32/64-bit `.dll` files should be treated as "Windows plugin binaries for both bitnesses" unless legacy VST2 is explicitly required and legally supplied.
- The full Dear Reality-like experience is not only UI. It requires robust position automation, smoothing, spatial rendering, output format management, and DAW-host validation.
- Multi-source behavior in FL Studio must be designed around host routing constraints. A plugin cannot automatically receive every mixer channel unless FL Studio routes those channels into the plugin instance through supported buses/sidechains.
