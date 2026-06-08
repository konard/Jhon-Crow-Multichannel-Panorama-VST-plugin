# Multichannel Panorama VST Plugin

Preparation scaffold for issue #1:

- Case study: `docs/case-studies/issue-1`
- Build system: CMake + JUCE 8.0.13
- Initial plugin format: VST3
- CI artifacts: Windows VST3 packages for `Win32` and `x64`

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

The copied VST3 artifact is written under:

```text
build/release/artifacts/vst3
```

On Windows with Visual Studio 2022, select the target architecture:

```powershell
cmake -S . -B build/win64 -G "Visual Studio 17 2022" -A x64
cmake --build build/win64 --config Release --target MultichannelPanorama_VST3

cmake -S . -B build/win32 -G "Visual Studio 17 2022" -A Win32
cmake --build build/win32 --config Release --target MultichannelPanorama_VST3
```

## Notes

The scaffold is intentionally minimal. It exposes listener/source position parameters, source gain, bypass, state persistence, and pass-through processing. The full Dear Reality-style spatial UI and renderer are planned in the issue case study.
