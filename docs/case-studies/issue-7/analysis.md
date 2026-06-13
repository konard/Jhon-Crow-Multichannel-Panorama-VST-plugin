# Case Study: Issue #7 — Per-Channel Auto-Load and Bypass

## Issue Summary

**Title (Russian):** "fix сейчас работает управление панорамой только для всего звука сразу"
**Translation:** "fix — right now panorama control only works for all audio at once"

**Request:** Make all incoming channels automatically load on screen at a fixed position, with the ability to individually bypass each channel.

## Timeline / Sequence of Events

1. **Plugin initial state**: The plugin was built with 8 fixed source "slots". Only source 0 is activated by default (in `PluginProcessor.cpp` constructor).
2. **User observation**: In the screenshot, source 5 is selected in the control panel (X=30, Y=30, Z=30) but is not visible on the spatial pad because it's not `active`. The spatial pad only renders sources where `active == true`.
3. **Problem 1 — Auto-load**: Channels do not automatically appear. The user must manually click each source button and activate them. There is no mechanism that looks at how many input channels the DAW is providing and automatically activates the corresponding sources.
4. **Problem 2 — Global-only bypass**: There is a single `bypass` parameter in `PluginProcessor.h` that bypasses the entire plugin. There is no per-source bypass.

## Root Cause Analysis

### Root Cause 1: No Auto-Activation of Sources

In `PluginProcessor.cpp`:
- `isBusesLayoutSupported` accepts 1–64 input channels
- `processBlock` iterates sources 0..7, using `inputCh = i < numInputChannels ? i : 0`
- But the UI only shows sources that have `active == true` via the `sourceActive` parameter
- **The `active` flag is never set based on the actual number of input channels**

The constructor only activates source 0:
```cpp
if (auto* p = parameters.getParameter (ParameterIDs::sourceActive (0)))
    p->setValueNotifyingHost (1.0f);
```

So when a DAW sends 4 channels, sources 1–3 are present in audio but invisible in the UI.

### Root Cause 2: No Per-Channel Bypass

In `PluginProcessor.h`, only a global bypass exists:
```cpp
constexpr auto bypass = "bypass";
```

In `processBlock`, the global bypass returns early for ALL sources:
```cpp
if (bypassParam != nullptr && bypassParam->load() >= 0.5f)
    return;
```

There is no per-source bypass parameter, no per-source bypass UI button, and no per-source bypass logic in `processBlock`.

### Root Cause 3: Fixed Default Positions

All sources default to position (0, 0, 0) except source 0 which defaults to Z=2. When channels auto-load, they stack on top of each other, making them hard to distinguish and interact with.

## Proposed Solution

### 1. Auto-activate sources based on input channel count in `prepareToPlay`

In `prepareToPlay`, check `getTotalNumInputChannels()` and activate the corresponding sources (up to `maxSources`). This ensures the UI matches the actual audio routing.

### 2. Add per-source bypass parameter

Add `sourceBypass(i)` parameters (bool) to the parameter layout. In `processBlock`, check each source's bypass flag before processing it. In the UI, add a bypass toggle button per source in the source selector strip.

### 3. Spread auto-loaded sources at fixed positions

When auto-activating, place sources in a circle or arc around the listener at a reasonable distance (e.g. 5m), so they are immediately visible and spatially distributed.

## Implementation Plan

1. Add `sourceBypass(i)` parameters to `ParameterIDs` and `createParameterLayout()`
2. Add `SceneAtomics` field for per-source bypass (or read directly from parameters in `processBlock`)
3. Update `processBlock` to skip bypassed sources
4. Update `prepareToPlay` to auto-activate sources based on input channel count
5. Add per-source bypass toggle button to the UI (in the source selector row)
6. Add per-source default positions (spread in a semicircle)
