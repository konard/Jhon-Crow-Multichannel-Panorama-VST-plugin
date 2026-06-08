# Issue 1 Case Study: Preparation Environment

Issue: https://github.com/Jhon-Crow/Multichannel-Panorama-VST-plugin/issues/1

Prepared on: 2026-06-08

## Requested Outcome

The issue asks for preparation before full implementation of a VST plugin for FL Studio:

- Collect research and technical data for the requested plugin.
- Store the case study under `docs/case-studies/issue-1`.
- Propose architecture and possible implementation options.
- Prepare GitHub Actions so the repository can build Windows plugin artifacts for 32-bit and 64-bit targets.
- Record a forward plan and known risks.

## Repository Baseline

Before this preparation pass, the repository contained only `README.md` and a temporary `.gitkeep` generated for PR creation. There was no plugin source tree, no CMake build, no documentation folder, and no GitHub Actions workflow.

## Prepared Assets

- `CMakeLists.txt` creates a pinned JUCE 8.0.13 VST3 plugin scaffold.
- `source/PluginProcessor.*` defines a minimal pass-through processor with listener/source position parameters and state persistence.
- `.github/workflows/build-vst.yml` validates preparation files and builds Windows VST3 artifacts for `Win32` and `x64`.
- `scripts/validate-preparation.sh` checks that the requested docs, source scaffold, and workflow markers are present.
- This case-study folder records the research, architecture proposal, plan, and risks.

## Main Conclusion

The most practical first implementation target is a JUCE-based VST3 plugin. FL Studio supports VST3 and 32/64-bit Windows plugins, while a legacy VST2 `.dll` deliverable needs a separate legal decision because VST2 is discontinued and no longer a normal public SDK path. The prepared workflow therefore builds VST3 Windows artifacts for both architectures. A true VST2 `.dll` can be added later only if the project supplies valid legacy VST2 SDK access and accepts the compatibility/legal tradeoff.

## Case Study Files

- [research.md](research.md) - external facts, source links, and candidate libraries/components.
- [architecture.md](architecture.md) - proposed architecture and implementation approach.
- [action-plan.md](action-plan.md) - phased future work and known risks.
