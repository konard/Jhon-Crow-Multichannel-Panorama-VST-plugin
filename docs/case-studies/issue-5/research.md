# Research Notes

Research date: 2026-06-13

## Source Issue

Issue 5 reported that the CI builds failed and linked directly to the failing Windows configure
step:

- Issue: https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin/issues/5
- Referenced job: https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin/actions/runs/27344514208/job/80789560840#step:4:18

## Preserved CI Runs

### Run 27344514208

- Workflow: `Build VST Plugin`
- Event: `push`
- Branch: `main`
- SHA: `118b3055310eb27220d1496816a3eeb2dfac9781`
- Created: `2026-06-11T11:46:32Z`
- Result: failure
- Preserved log: `ci-logs/build-vst-plugin-27344514208.log`

Jobs:

| Job | Result | Key observation |
| --- | --- | --- |
| Validate repository preparation | success | Preparation script and configure-only smoke test passed. |
| Build Windows (win64) | failure | Failed in Configure CMake before build steps. |
| Build Windows (win32) | failure | Failed in Configure CMake before build steps. |

Extracted failure lines:

- `ci-logs/build-vst-plugin-27344514208.log:304` - `CMake Error at CMakeLists.txt:3 (project):`
- `ci-logs/build-vst-plugin-27344514208.log:310` - `could not find any instance of Visual Studio.`
- `ci-logs/build-vst-plugin-27344514208.log:473` - same CMake project error for Win32.
- `ci-logs/build-vst-plugin-27344514208.log:479` - same Visual Studio instance discovery failure for Win32.

### Run 27462273583

- Workflow: `Build VST Plugin`
- Event: `pull_request`
- Branch: `issue-5-ee7bc7726a77`
- SHA: `97bdc6b013da7ceb28f13a8bee93f17d159bbd68`
- Created: `2026-06-13T08:58:17Z`
- Result: failure
- Preserved log: `ci-logs/build-vst-plugin-27462273583.log`

This later PR run reproduced the same configure-stage failure pattern, confirming that the original
failure was still active on the issue branch before the workflow change.

## Sequence of Events

1. Issue 3 added CI support for Windows VST2 `.dll` artifacts in addition to VST3.
2. The workflow matrix used `windows-latest` and passed `-G "Visual Studio 17 2022"` plus
   `-A Win32` or `-A x64` to CMake.
3. On 2026-06-11, `main` run `27344514208` started after merge commit
   `118b3055310eb27220d1496816a3eeb2dfac9781`.
4. The Linux validation job passed, proving that the repository files and configure-only mode were
   valid.
5. Both Windows jobs cloned the VST2 SDK headers successfully into `D:/vst2sdk`.
6. Both Windows jobs failed immediately when CMake evaluated `project()` and tried to resolve the
   hard-coded Visual Studio generator.
7. On 2026-06-13, PR run `27462273583` reproduced the same failure against the issue branch.
8. The workflow was updated to stop hard-coding the generator and let the hosted runner's CMake
   environment select the available generator while preserving the requested architecture.

## Root Cause

The CI failure was an environment/toolchain discovery problem, not a C++ compile failure and not a
VST2 SDK download failure.

The failing command was:

```powershell
cmake -S . -B "D:/b/win64" -G "Visual Studio 17 2022" -A "x64" -DCMAKE_BUILD_TYPE=Release -DMULTICHANNEL_PANORAMA_VST2_SDK_PATH="$env:VST2_SDK_PATH"
```

CMake then reported that it could not find any Visual Studio instance for the named generator.
Because the workflow pinned a specific generator name, the build had no fallback when that generator
was not discoverable in the current runner image.

## External Facts

- GitHub-hosted `windows-latest` runners are image labels that can move as GitHub updates the
  hosted runner fleet. Workflows should avoid depending on unstated image details when CMake can
  discover the active toolchain environment.
- CMake's Visual Studio generators accept `-A` for platform selection. Passing `-A Win32` and
  `-A x64` is still the part required by this workflow's 32-bit and 64-bit artifact matrix.

Sources:

- GitHub Actions hosted runners reference: https://docs.github.com/en/actions/reference/github-hosted-runners-reference
- CMake Visual Studio generators documentation: https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators

## Solution Options

| Option | Pros | Cons |
| --- | --- | --- |
| Let CMake use the runner default generator and keep `-A` | Minimal change; avoids stale generator-name assumptions; preserves Win32/x64 matrix. | Still depends on the hosted runner having a Visual Studio-compatible CMake environment. |
| Pin a specific runner image such as `windows-2022` | Narrows runner drift. | Still fails if Visual Studio discovery changes or if the requested image is later deprecated. |
| Add an explicit Visual Studio setup step | More control over toolchain selection. | More moving parts and slower CI for a small project. |

## Chosen Fix

The workflow now removes `-G "Visual Studio 17 2022"` from the configure command. This is the
smallest change that addresses the observed root cause without changing the plugin build,
artifact layout, VST2 SDK handling, or branch triggers.
