# ProceduralCurves

**English** | [Русский](README.ru.md)

Formula-defined curves for Unreal Engine 5. Instead of keys, a curve is a stack of waveforms and math expressions, evaluated continuously at any time, from any thread, with live plots in the Details panel.

![Damped spring expression](screenshots/ProceduralCurve_Spring.png)

## Overview

**ProceduralCurves** adds `FProceduralCurve`: an offset plus an ordered list of terms. Each term is a waveform (sine, triangle, square, sawtooth, bounce, noise, stepped noise or a typed formula) with amplitude, frequency, phase, bias and shaping. Terms either add to the result or multiply it, so envelopes, modulation and layered motion take a few rows instead of hundreds of keys.

Typical uses: weapon sway and head bob, breathing idles, light flicker, UI pulses, camera shake, procedural recoil, anything periodic or noisy that an animator wants to tune by numbers rather than by keys.

**Key features:**
- `FProceduralCurve`, `FProceduralVectorCurve` and `FProceduralRotatorCurve` structs. Plain data: work in data assets, Blueprints, SaveGame and replication
- Eight waveforms with skew, duty, smoothing, exponent and unipolar range
- Add and Multiply blending between terms
- Math expressions compiled to a stack program, with 40 functions, cached per distinct formula
- Frame-rate independent playback: one full cycle every N seconds
- Details panel customization: inline plots, a two-cycle preview with range labels and a loop-seam warning, live formula validation
- Thread-safe evaluation, no editor dependencies at runtime

## Installation

Clone or download the source into your project's `Plugins/` directory, regenerate project files, and enable the plugin in `.uproject` or via Edit > Plugins.

**Modules:**

| Module | Type | Purpose |
|--------|------|---------|
| `ProceduralCurves` | Runtime | Curve structs, waveforms, expression compiler, Blueprint library |
| `ProceduralCurvesEditor` | Editor | Details panel customization and plots |

## Quick Start

### Details panel

1. Add a `FProceduralCurve` (or `FProceduralVectorCurve`, `FProceduralRotatorCurve`) property to any class, struct or data asset
2. Add terms, pick a waveform for each, set Amplitude, Frequency and Phase
3. Watch the plot update as you edit

```cpp
UPROPERTY(EditAnywhere, Category = "Sway")
FProceduralVectorCurve Sway;
```

### Blueprint: playing a curve over time

**Evaluate Procedural Curve On Time** advances a phase by `DeltaTime / CycleDuration` and evaluates the curve at it. With `CycleDuration = 3` the curve completes one cycle every 3 seconds at any frame rate. Keep `Phase` in a float variable.

| Function | Purpose |
|----------|---------|
| `EvaluateProceduralCurve` | Value at a given time |
| `EvaluateProceduralVectorCurve` | Vector value at a given time |
| `EvaluateProceduralRotatorCurve` | Rotator value at a given time |
| `EvaluateProceduralCurveOnTime` | Advance the phase, then evaluate |
| `EvaluateProceduralVectorCurveOnTime` | Same for a vector curve |
| `EvaluateProceduralRotatorCurveOnTime` | Same for a rotator curve |
| `AdvanceProceduralPhase` | Advance and wrap a phase only |

All of them are thread-safe and callable from animation Blueprint thread-safe functions.

![Blueprint nodes](screenshots/ProceduralCurveNodes.png)

### C++

```cpp
#include "ProceduralCurve.h"
#include "ProceduralCurveLibrary.h"

// Bob: a 2x sine with a sharper 4x contact, written as data.
FProceduralCurve Bob;
Bob.AddSine(0.6f, 2.0f, 270.0f)
   .AddSine(0.07f, 4.0f, 270.0f);

// Breathing envelope multiplying everything above it.
FProceduralCurveTerm& Breath = Bob.Terms.AddDefaulted_GetRef();
Breath.Blend = EProceduralTermBlend::Multiply;
Breath.Bias = 1.0f;
Breath.Amplitude = 0.2f;

// One cycle every 1.2 seconds, frame-rate independent.
float Phase = 0.0f;
const float Value = UProceduralCurveLibrary::EvaluateProceduralCurveOnTime(Bob, DeltaTime, 1.2f, Phase);
```

## Terms

Each term computes `Bias + Amplitude * shape(waveform(x))`, where `x = Frequency * Time + Phase / 360` is measured in cycles. Terms are applied top to bottom: **Add** sums the term into the result, **Multiply** scales everything above it. `Offset` is added at the end, then the optional output clamp applies.

| Waveform | Shape | Parameters |
|----------|-------|------------|
| Sine | Classic sine | |
| Triangle | Linear up and down | `Skew`: peak position; 0 or 1 gives a sawtooth |
| Square | High for part of the cycle | `Duty`: high fraction; `Smoothing`: rounded jumps |
| Sawtooth | Rising ramp with a drop | `Smoothing`: rounded drop |
| Bounce | Rectified sine | |
| Noise | Smooth random values | `Seed`; knots per unit = `Frequency` |
| SteppedNoise | Random values held between knots | `Seed`, `Smoothing` |
| Expression | Typed formula | `Expression` |

**Shaping, for every waveform:**
- `Range`: Bipolar keeps `[-1, 1]`, Unipolar remaps to `[0, 1]` (pulses, envelopes)
- `Exponent`: signed power, above 1 sharpens peaks, below 1 flattens toward a square
- `Bias`: added after amplitude; a Multiply term with Bias 1 and Amplitude 0.2 scales by 0.8..1.2
- `bEnabled`: mute a term without deleting it

Every periodic waveform repeats once per whole `x` and, unless stated, rises through zero at `x = 0` like a sine. Noise repeats every `round(Frequency)` knots, so whole-number frequencies loop seamlessly over a unit of time.

## Expressions

Set a term's waveform to **Expression** and type a formula. Its result is the waveform value: Amplitude, Bias, Exponent and Range still apply. Errors show under the field while you type, with the position of the problem.

**Variables:** `t` (curve time), `x` (`Frequency * t + Phase / 360`, in cycles), `p` (`frac(x)`, position within the cycle)
**Constants:** `pi`, `tau`, `e`
**Operators:** `+ - * / % ^` (power, right-associative), unary `-` and `!`, `< <= > >= == !=`, `&& ||`, `c ? a : b`

| Group | Functions |
|-------|-----------|
| Trigonometry | `sin cos tan asin acos atan atan2` |
| Rounding | `abs sign floor ceil round frac` |
| Power | `sqrt exp log log2 pow` |
| Range | `min max mod clamp saturate lerp remap select` |
| Easing | `step smoothstep smootherstep` |
| Waves | `sine tri square saw bounce` (period 1, same parameters as the term waveforms) |
| Noise | `noise pnoise snoise hash` |

Division, modulo, `sqrt` and `log` with an invalid argument return 0 instead of NaN. Names are case-insensitive. Hover the help line under the formula field for every signature.

**Examples:**

Decaying bounces:

![Decaying bounces expression](screenshots/ProceduralCurve_Bounce.png)

A sine with rare random glitches, `sine(x) + (hash(floor(x * 16), 7) > 0.85 ? (hash(floor(x * 16), 3) - 0.5) * 3 : 0)`:

![Glitch expression](screenshots/ProceduralCurve_Glitch.png)

## Details Panel

- **Curve header:** inline plot of one cycle
- **Curve body:** two-cycle plot with min and max labels; a red marker and a warning when the value jumps across the loop seam
- **Vector and rotator curves:** X, Y and Z (or Roll, Pitch and Yaw) plotted together in red, green and blue
- **Term header:** enable toggle, waveform, blend, a sparkline and a one-line summary (`A 0.4   f 2   310°`)
- **Term body:** only the parameters of the selected waveform
- **Formula field:** multi-line, validated as you type, full function reference on hover

A single sine term:

![Sine curve in the Details panel](screenshots/ProceduralCurve_Sine.png)

A vector curve with X, Y and Z phase-shifted:

![Vector curve in the Details panel](screenshots/ProceduralVectorCurve_Sine.png)

A rotator curve, legend taken from the Roll, Pitch and Yaw members:

![Rotator curve in the Details panel](screenshots/ProceduralRotator.png)

## Architecture

```
FProceduralCurve (USTRUCT)
├── Terms: TArray<FProceduralCurveTerm>
├── Offset, bClampOutput, OutputMin, OutputMax
├── Evaluate(Time) -> float
└── AddSine() / Add() / Scale()

FProceduralCurveTerm (USTRUCT)
├── Waveform, Blend, Amplitude, Frequency, Phase, Bias
├── Range, Exponent, Skew, Duty, Smoothing, Seed
├── Expression: FString
└── Evaluate(Time) -> float

FProceduralVectorCurve (USTRUCT)
└── X, Y, Z: FProceduralCurve

FProceduralRotatorCurve (USTRUCT)
└── Roll, Pitch, Yaw: FProceduralCurve      // degrees, Evaluate -> FRotator

ProceduralWaves             // waveform math shared by terms and expressions
ProceduralExpression        // tokenizer, recursive-descent compiler, stack VM
UProceduralCurveLibrary     // Blueprint functions and frame-rate independent playback
```

### How expressions work

1. **Compile:** a recursive-descent parser turns the formula into a flat stack program, checking names, argument counts and stack depth.
2. **Cache:** programs live in a process-wide cache keyed by the formula text, behind a read-write lock. A formula compiles once no matter how many curves, copies or threads use it, and nothing compiled is stored in assets, so copies, undo, replication and Blueprint-made structs always work. Each term also remembers its last program in an atomic pointer, so a steady formula is evaluated without hashing or locking; the cache never frees a program, which keeps that pointer valid.
3. **Run:** the program executes on a fixed-size stack with no allocation; invalid formulas evaluate to 0 and log their error once.

## Notes

- Evaluation is thread-safe. Editing a curve in the Details panel during PIE while an animation worker thread evaluates it is not, the same as with regular curve assets.
- Whole-number frequencies loop seamlessly over a unit of time. The plot marks the seam when they do not.

## Requirements

- Unreal Engine 5.0+ (built and tested on 5.6; older versions are covered by version guards but not compile-tested)

## License

MIT
