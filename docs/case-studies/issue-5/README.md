# Issue 5 Case Study: Windows CI Configure Failure

Issue: https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin/issues/5

Prepared on: 2026-06-13

## Requested Outcome

The issue reported failed GitHub Actions builds and asked to preserve related logs and data under
`docs/case-studies/issue-5`, reconstruct the timeline, identify root causes, and propose fixes.

## Evidence Preserved

- `ci-logs/build-vst-plugin-27344514208.log` - original failing `main` run from 2026-06-11.
- `ci-logs/build-vst-plugin-27462273583.log` - confirming failing PR run from 2026-06-13.
- `research.md` - extracted timeline, exact failure lines, root cause, and external references.

## Main Finding

Both Windows matrix jobs failed before compiling the plugin. The VST2 SDK checkout succeeded, but
CMake configure failed with:

```text
Generator

  Visual Studio 17 2022

could not find any instance of Visual Studio.
```

The root cause was the workflow hard-coding the Visual Studio generator. On the observed GitHub
Windows runner image, CMake could not resolve that generator to an installed Visual Studio instance.
Because GitHub-hosted runners already set up a default CMake generator environment, the most robust
CI fix is to let CMake select the runner-provided generator while still passing the matrix
architecture with `-A Win32` or `-A x64`.

## Implemented Fix

`.github/workflows/build-vst.yml` no longer passes `-G "Visual Studio 17 2022"` in the Windows
configure step. The build still targets both architectures through the existing matrix and still
passes the VST2 SDK path so `.dll` artifacts remain enabled.

## Local Validation

The Linux-accessible checks pass locally:

```bash
bash scripts/validate-preparation.sh
cmake -S . -B build/prepare-smoke -DMULTICHANNEL_PANORAMA_BUILD_PLUGIN=OFF
```

The full Windows plugin build must be verified in GitHub Actions because it depends on the
Windows-hosted runner image and Visual Studio toolchain discovery.
