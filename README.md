# Multichannel Panorama VST Plugin

Spatial-audio panning plugin for FL Studio (Dear Reality PRO VR–style), implemented with JUCE.

- Case study: `docs/case-studies/issue-1`
- Build system: CMake + JUCE 8.0.13
- Plugin formats: **VST2 (`.dll`)** and **VST3 (`.vst3`)**
- CI artifacts: Windows `Win32` and `x64`, each containing a `VST2/` (`.dll`) and a `VST3/` folder

## Installing in FL Studio

Download the matching artifact from the GitHub Actions run (`MultichannelPanorama-win64`
for 64-bit FL Studio, `MultichannelPanorama-win32` for 32-bit) and unzip it. You get a
`VST2` folder (with `Multichannel Panorama.dll`) and a `VST3` folder.

- **VST2 (`.dll`) – use this if FL Studio does not detect the VST3.**
  Copy `Multichannel Panorama.dll` into your VST2 search folder, e.g.
  `C:\Program Files\Common Files\VST2` (or any folder you add under
  *Options → Manage plugins → Plugin search paths*), then click **Find plugins**.
- **VST3 (`.vst3`).** Copy the `Multichannel Panorama.vst3` package into
  `C:\Program Files\Common Files\VST3`, then click **Find plugins**.

In FL Studio, open **Options → Manage plugins**, add the folder if needed, run a
**Find plugins** scan, then add the effect to a mixer/channel insert slot.

## Local Validation

Check that the preparation files are present:

```bash
bash scripts/validate-preparation.sh
```

Run a configure-only smoke test without fetching JUCE:

```bash
cmake -S . -B build/prepare-smoke -DMULTICHANNEL_PANORAMA_BUILD_PLUGIN=OFF
```

## Local Plugin Build

On a machine with CMake and a supported compiler:

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --config Release --target MultichannelPanorama_VST3
```

The copied VST3 artifact is written under `build/release/artifacts/vst3`.

### Building the VST2 `.dll`

The VST2 SDK is **not** bundled in this repository (Steinberg's licence forbids
redistributing it). Point CMake at a local VST2 SDK — any folder that contains
`pluginterfaces/vst2.x/aeffect.h` — to enable the `.dll` target:

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DMULTICHANNEL_PANORAMA_VST2_SDK_PATH=/path/to/vst2_sdk
cmake --build build/release --config Release --target MultichannelPanorama_VST
```

The copied VST2 `.dll` is written under `build/release/artifacts/vst2`. The CI workflow
fetches these headers automatically from a public mirror, so the uploaded artifacts
always include both the `.dll` and the `.vst3`.

On Windows with Visual Studio 2022, select the target architecture:

```powershell
cmake -S . -B build/win64 -G "Visual Studio 17 2022" -A x64 `
  -DMULTICHANNEL_PANORAMA_VST2_SDK_PATH=C:/path/to/vst2_sdk
cmake --build build/win64 --config Release --target MultichannelPanorama_VST3
cmake --build build/win64 --config Release --target MultichannelPanorama_VST

cmake -S . -B build/win32 -G "Visual Studio 17 2022" -A Win32 `
  -DMULTICHANNEL_PANORAMA_VST2_SDK_PATH=C:/path/to/vst2_sdk
cmake --build build/win32 --config Release --target MultichannelPanorama_VST3
cmake --build build/win32 --config Release --target MultichannelPanorama_VST
```

## Notes

The plugin exposes up to 8 positionable sources plus a movable listener (X/Y/Z + gain),
a draggable 2-D spatial pad (top X/Z and front X/Y views), constant-power panning with
inverse-distance attenuation, parameter smoothing, and state persistence. See
`docs/case-studies/issue-1` for the architecture and roadmap.
