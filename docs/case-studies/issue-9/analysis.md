# Case Study: Issue #9 - Distance Change Is Not Noticeable

## Issue Summary

Issue: https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin/issues/9

Reporter text:

> изменение дальности не заметно

Translation:

> Change in distance is not noticeable.

The reporter clarified that the difference is noticeable only when the sound source is moved very close to the listener, and asked for distance and each individual source to be clearly distinguishable.

## Collected Evidence

This folder preserves the data used for the reconstruction:

- `issue.json` - issue title, body, author, timestamps, and comments.
- `issue-comments.json` - paginated issue comments.
- `pr-10.json` - PR title, body, draft state, comments, reviews, and timestamps.
- `pr-10.diff` - full PR diff at the time of analysis.
- `branch-commits.tsv` - branch commits with ISO timestamps.
- `ci-runs.json` - five most recent workflow runs on `issue-9-45bb00f49c9a`.
- `ci-run-27894856634.log` - failed CI run for commit `608b232`.
- `ci-run-27894879036.log` - passing CI run for commit `d0affd2`.
- `ci-run-27895043747.log` - passing CI run for commit `7756f09`.
- `pr-comment-release-type-missing.png` - reviewer screenshot showing the missing manual release-type selector.

The reviewer screenshot was downloaded from GitHub user attachments and verified as a PNG by checking its file signature.

## Timeline

- 2026-06-21 05:28 UTC - Initial preparation commit ran CI successfully.
- 2026-06-21 05:35 UTC - Commit `608b232` added the distance-gain fix and test, but CI failed because the workflow built only `SourceChannelMappingTests` before running all registered CTest tests.
- 2026-06-21 05:36 UTC - Commit `d0affd2` changed the smoke-test job to build all lightweight test targets before `ctest`; CI passed.
- 2026-06-21 05:44 UTC - Commit `7756f09` reverted the temporary task-details commit; CI passed again.
- 2026-06-25 15:37 UTC - Owner review comment reported that after reset there was no release-type choice in the newly added workflow stage and requested a complete issue case study with logs/data.
- 2026-06-25 - The workflow was updated to declare a `workflow_dispatch` `release_type` choice input and this case study was expanded with issue, PR, CI, screenshot, and diff artifacts.

## Audio Root Cause

The original distance model in `source/SceneState.h` was:

```cpp
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    const float dist = std::max (distanceMetres, 0.01f);
    return 1.0f / dist;
}
```

This is an inverse-distance pressure law, but it was not normalized to a reference distance and it allowed gain above unity for sources closer than 1 m. Close-range values were extreme:

| Distance | Old gain | Old dB |
|---:|---:|---:|
| 0.01 m | 100.0 | +40.0 dB |
| 0.5 m | 2.0 | +6.0 dB |
| 1.0 m | 1.0 | 0.0 dB |
| 2.0 m | 0.5 | -6.0 dB |
| 30.0 m | 0.033 | -29.5 dB |

The issue was not that inverse distance is inherently wrong. The issue was that the model treated 1 m as an accidental unity point and allowed distances below that point to amplify above 0 dB. In a summed multichannel mix, near sources could clip or dominate the mix, masking practical distance changes between ordinary positions.

The implemented model extracts the function to `source/DistanceModel.h` and clamps at a 1 m reference distance:

```cpp
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    constexpr float refDist = 1.0f;
    const float dist = std::max (distanceMetres, refDist);
    return refDist / dist;
}
```

This keeps gain in `[0, 1]`, preserves the expected -6 dB per distance doubling, and provides about 30 dB of usable distance contrast between 1 m and 30 m without close-range overload.

## CI Root Cause

The failed CI run `27894856634` shows:

- `SourceChannelMappingTests` was built successfully.
- CTest then tried to run `DistanceGainTests`.
- The executable did not exist in `build/prepare-smoke`.
- CTest failed with `50% tests passed, 1 tests failed out of 2`.

The workflow step had built a single explicit target while CMake had registered multiple tests. The fix was to run:

```bash
cmake --build build/prepare-smoke
ctest --test-dir build/prepare-smoke --output-on-failure
```

The next two CI runs, `27894879036` and `27895043747`, both passed and show `SourceChannelMappingTests` and `DistanceGainTests` passing.

## Workflow UI Root Cause

The reviewer screenshot showed that the manual workflow run UI did not offer a release-type selector. The workflow had:

```yaml
workflow_dispatch:
```

with no inputs. GitHub Actions only renders manual-run controls when `workflow_dispatch.inputs` are declared. GitHub supports typed manual inputs including `choice`, so the workflow now declares:

```yaml
workflow_dispatch:
  inputs:
    release_type:
      description: Release type for manual builds
      required: true
      default: reset
      type: choice
      options:
        - reset
        - patch
        - minor
        - major
```

This restores the visible release-type choice after reset while leaving push and pull-request builds unchanged.

## Online Research

- GitHub Actions supports `workflow_dispatch` inputs, and typed manual inputs include `choice`, `boolean`, and `environment`: https://github.blog/changelog/2021-11-10-github-actions-input-types-for-manual-workflows/
- GitHub workflow syntax documents that `workflow_dispatch` can specify inputs passed to manually triggered workflows: https://docs.github.com/en/enterprise-cloud@latest/actions/reference/workflows-and-actions/workflow-syntax#onworkflow_dispatch
- OpenAL 1.1 defines `AL_INVERSE_DISTANCE_CLAMPED`, where `AL_REFERENCE_DISTANCE` indicates both the reference distance and the distance below which gain is clamped: https://www.openal.org/documentation/openal-1.1-specification.pdf
- FMOD documents distance attenuation using min/max distance ranges and inverse rolloff behavior: https://www.fmod.com/docs/2.03/studio/effect-reference.html
- Kolarik et al. review auditory distance cues and describe level as a major distance cue in free-field listening: https://pmc.ncbi.nlm.nih.gov/articles/PMC4744263/

## Solution Options

1. Clamped inverse pressure law, normalized at 1 m.
   This is the implemented fix. It preserves familiar physical behavior, prevents gain above unity, and makes the distance range clearly audible.

2. Inverse-square gain law.
   This would create a much stronger rolloff, about -60 dB at 30 m. It may be useful as a future optional mode, but it is aggressive as a default.

3. Linear rolloff to a configured maximum distance.
   This is predictable for UI control and game-style sound design, but less physically natural than inverse pressure attenuation.

4. User-selectable distance model.
   A future enhancement could expose inverse, inverse-square, and linear models in the UI. That would solve multiple creative-use cases but is broader than the bug report.

## Verification

- `tests/DistanceGainTests.cpp` verifies that distance gain is clamped to unity below the reference distance, follows inverse-distance ratios above it, and never exceeds `1.0`.
- Local CMake/CTest verifies `SourceChannelMappingTests` and `DistanceGainTests`.
- CI runs `27894879036` and `27895043747` verify the corrected workflow builds all lightweight test targets before running CTest.
- The manual workflow declaration now includes a typed release selector, addressing the reviewer screenshot.
