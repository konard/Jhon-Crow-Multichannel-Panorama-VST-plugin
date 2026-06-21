# Case Study: Issue #7 - Per-Channel Auto-Load and Bypass

## Issue Summary

**Title (Russian):** "fix сейчас работает управление панорамой только для всего звука сразу"
**Translation:** "fix - right now panorama control only works for all audio at once"

**Request:** Make all incoming channels automatically load on screen at fixed positions, with the ability to bypass each channel individually.

## Evidence Saved in This Folder

- `screenshot.png`: original issue screenshot from GitHub user attachment.
- `issue.json`: issue #7 metadata and body.
- `pr-8-comments.json`: PR #8 conversation comments, including the owner follow-up.
- `ci-runs.json`: recent CI runs for branch `issue-7-1b0527107289`.
- `ci-run-27465725812.log`: latest passing CI log for the first implementation.

## Online References

- JUCE `AudioProcessor::processBlock` documentation: the buffer starts with the processor input data and must be replaced with output data; when a processor has more inputs than outputs, the input channels remain in the buffer and only output channels should be overwritten. Source: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html
- JUCE bus layout tutorial: a DAW may request different channel sets at runtime, and the plugin can accept or reject layouts through `isBusesLayoutSupported()`. Source: https://juce.com/tutorials/tutorial_audio_bus_layouts/
- FL Studio mixer manual: sidechain sends allow plugins with more than one audio input to receive multiple independent streams in addition to the destination mixer track. Source: https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/mixer.htm

## Timeline / Sequence of Events

1. **2026-06-13 10:12 UTC - issue opened.** The user reported that panorama control affected the whole sound at once and asked for all incoming channels to appear automatically with per-channel bypass.
2. **Initial state before PR #8.** The plugin had eight source slots in parameters/UI, but only source 1 was active by default. The UI rendered only active sources.
3. **First implementation in PR #8.** The code added auto-activation from `getTotalNumInputChannels()`, per-source bypass parameters, bypass UI controls, and spread default source positions.
4. **2026-06-13 11:40 UTC - CI passed.** Run `27465725812` passed for repository preparation and both Windows builds.
5. **2026-06-13 15:32 UTC - owner follow-up.** The owner commented: "всё ещё все каналы звучат из 1" ("all channels still sound from 1").
6. **Second investigation.** The audio code still used the old fallback mapping: when a source index had no matching host input channel, it read input channel 0.

## Root Cause Analysis

### Root Cause 1: No Auto-Activation of Sources

Before PR #8, `sourceActive(0)` was set in the constructor, but no code activated sources based on the actual input channel count. The spatial pad skipped inactive sources, so channels other than source 1 could exist in processing but remain invisible.

### Root Cause 2: Global-Only Bypass

Before PR #8, `ParameterIDs::bypass` bypassed the entire plugin. There was no per-source bypass parameter, no per-source bypass UI control, and no per-source branch in `processBlock()`.

### Root Cause 3: Missing Inputs Fell Back to Channel 1

The first PR #8 implementation kept this mapping pattern in both the normal and bypass paths:

```cpp
const int inputCh = i < numInputChannels ? i : 0;
```

That meant an active source without a matching host input channel reused input channel 0. In user-visible terms, source slots beyond the real host input count could all sound as if they came from channel/source 1. This matched the owner's follow-up.

### Root Cause 4: Host Routing Still Matters

The plugin can only separate audio streams that the host actually provides as separate input channels or input buses. If FL Studio has already summed several mixer tracks into one stereo signal before the plugin, the plugin cannot reconstruct the original tracks. FL Studio's own documentation describes sidechain sends as the path for effects with multiple audio inputs to receive independent streams.

## Implemented Solution

### 1. Keep the first PR #8 behavior that was correct

- Auto-activate sources according to the host input channel count.
- Add `sourceBypass(i)` parameters.
- Add per-source bypass processing and UI controls.
- Spread newly activated sources on a visible arc.

### 2. Fix the channel-1 fallback

Added `source/SourceChannelMapping.h` with an explicit mapping helper:

```cpp
sourceInputChannelFor (sourceIndex, numInputChannels)
```

The helper returns `Panorama::noInputChannel` when a source slot does not have a matching host input. `processBlock()` now skips those slots instead of reading input channel 0.

### 3. Add a reproducing test

Added `tests/SourceChannelMappingTests.cpp`. The test covers the owner feedback case: with four host input channels and eight active source slots, only four sources route to inputs, and sources 5-8 do not fall back to channel 1.

### 4. Run the test in CI

Updated the lightweight repository-preparation CI job to configure with:

```bash
cmake -S . -B build/prepare-smoke -DMULTICHANNEL_PANORAMA_BUILD_PLUGIN=OFF -DMULTICHANNEL_PANORAMA_BUILD_TESTS=ON
```

It then builds `SourceChannelMappingTests` and runs `ctest`.

## Remaining Practical Note

If the owner expects separate FL Studio mixer tracks to appear as separate panorama sources, the tracks must be routed to the plugin as separate host inputs. A future enhancement could expose explicit sidechain input buses for source 2-8, but the current fix prevents the plugin itself from duplicating channel 1 when a source has no matching input channel.
