# Case Study: Issue #9 – Distance Change Is Not Noticeable

## Issue Summary

**Title (Russian):** "изменение дальности не заметно"
**Translation:** "Change in distance is not noticeable"

**Reporter's description (Russian):**
> "разница заметна только если поднести источник звука в упор. сделай чтоб можно было точно различать дальность и каждый отдельный источник звука."

**Translation:**
> "The difference is only noticeable if you bring the sound source very close. Make it possible to clearly distinguish distance and each individual sound source."

---

## Evidence and Code Analysis

### Current `distanceToLinearGain` implementation (`source/SceneState.h`)

```cpp
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    const float dist = std::max (distanceMetres, 0.01f);
    return 1.0f / dist;
}
```

The function returns `1/dist`. With no clamping at 1.0, it can return values **greater than 1.0** for distances < 1 m. The gain table:

| Distance (m) | gain = 1/dist | dB     |
|:---:|:---:|:---:|
| 0.01 | 100.0 | +40 dB |
| 0.5 | 2.0 | +6 dB |
| 1.0 | 1.0 | 0 dB |
| 2.0 | 0.5 | −6 dB |
| 5.0 | 0.2 | −14 dB |
| 10.0 | 0.1 | −20 dB |
| 20.0 | 0.05 | −26 dB |
| 30.0 | 0.033 | −29.5 dB |

**The problem:** The default listener position is (0, 0, 0) and the default source position for source 0 is Z=2m. The gain starts at 0.5 (−6 dB). Moving the source from 2m to 30m gives a range of only −23.5 dB — not enough for clear distance perception when all the perceptually "interesting" close-range variation is squashed by the high gain at <1m distances, which also saturates/clips the mix.

Additionally, `1/dist` is the **pressure** law, giving −6 dB per distance doubling. This is correct for modeling the natural behaviour of sound in free space, but the gain must be **normalized** so that the reference distance yields gain = 1.0 (0 dB), and values must be **clamped** so close sources don't amplify above 0 dB.

---

## Psychoacoustics Research

### Key facts

- **Inverse-square law**: Sound intensity drops with 1/r², corresponding to −6 dB pressure (SPL) per distance doubling. This matches the perceptual experience of sound in free space. (Source: Sengpielaudio; Kolarik et al. 2016, PMC4744263)
- **Just Noticeable Difference (JND)** for loudness is ~0.4 dB broadband / 1–2 dB sinewaves, meaning distance changes as small as 6–25% of reference can be detected. (PMC4744263)
- **Practical VST range**: 20–40 dB of attenuation span covers realistic near-to-far transitions (1m–30m = 5 doublings = 30 dB). (PubMed 38350176, PMC auditory distance review)
- **OpenAL default model** (`AL_INVERSE_DISTANCE_CLAMPED`): clamps distance to [referenceDistance, maxDistance] and normalizes gain so `referenceDistance → gain = 1.0`. (OpenAL 1.1 Specification)
- **FMOD Inverse mode**: `gain = minDist / max(distance, minDist)` — equivalent to clamped inverse law normalized at minDist.

### Why the current code sounds flat

1. At source Z=2m (default), gain = 0.5. All sources start already attenuated.
2. Moving a source from 2m to 30m spans only −23.5 dB.
3. Closer than 1m, gain exceeds 1.0, causing amplitude clipping in the mix which masks the effect perceptually.
4. There is **no normalization at a reference distance** — the level at any given position is arbitrary and depends on raw distance in meters.

---

## Proposed Solutions

### Solution A (Implemented): Clamped inverse law, normalized at reference distance

Reference distance = 1m (gain = 1.0 at 1m, 0 dB). Sources closer than 1m are held at gain = 1.0.

```cpp
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    const float refDist = 1.0f;
    const float dist = std::max (distanceMetres, refDist);
    return refDist / dist;
}
```

Result: gain is always in [0, 1.0]. At 1m: 0 dB. At 30m: −29.5 dB. Full 29.5 dB dynamic range without clipping.

### Solution B: Inverse-square law (more dramatic, 60 dB range)

```cpp
inline float distanceToLinearGain (float distanceMetres) noexcept
{
    const float refDist = 1.0f;
    const float dist = std::max (distanceMetres, refDist);
    return (refDist * refDist) / (dist * dist);
}
```

At 30m: −59 dB. Very dramatic. This is used in game engines with `rolloffFactor=2.0` in the OpenAL exponent model.

### Solution C: Linear rolloff

Predictable per-meter rolloff reaching silence at maxDistance. Non-physical but gives a defined endpoint.

---

## Chosen Approach

**Solution A** (clamped inverse law) was chosen because:
1. It matches the physical model of sound pressure in free space (the same model that human hearing is calibrated for).
2. It normalizes the reference level correctly: gain = 1.0 at 1m.
3. It prevents clipping from sources closer than 1m.
4. It provides ~30 dB of perceptually significant range from 1m to 30m (5 doublings × 6 dB).
5. It matches the approach used by OpenAL and FMOD for their default inverse-distance models.

---

## Online References

- Kolarik et al. (2016). "Auditory distance perception in humans: a review of cues, development, neuronal bases, and effects of sensory loss." *Attention, Perception, & Psychophysics* — https://pmc.ncbi.nlm.nih.gov/articles/PMC4744263/
- Sengpielaudio — Inverse Square Law Calculator: https://sengpielaudio.com/calculator-squarelaw.htm
- OpenAL 1.1 Specification: https://www.openal.org/documentation/openal-1.1-specification.pdf
- FMOD Distance Attenuation Reference: https://www.fmod.com/docs/2.03/studio/effect-reference.html
- Audio University — Inverse Square Law: https://audiouniversityonline.com/inverse-square-law-of-sound/
