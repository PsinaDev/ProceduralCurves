// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

/**
 * Waveforms over X in cycles. Every periodic one repeats each whole X, lies in [-1, 1] and, unless stated, rises through 0 at X = 0 like a sine.
 * Smoothing is the width of a smoothed jump as a fraction of a cycle (or of a knot spacing for noise); 0 keeps the jump sharp.
 * Thread-safe.
 */
namespace ProceduralWaves
{
	PROCEDURALCURVES_API float Sine(float X);

	/** Peaks at X = Skew / 2. Skew 0.5 is symmetric; 1 is a rising sawtooth, 0 a falling one. */
	PROCEDURALCURVES_API float Triangle(float X, float Skew);

	/** +1 over the first Duty of each cycle, -1 over the rest; starts at the rising edge, so X = 0 sits mid-jump when smoothed. */
	PROCEDURALCURVES_API float Square(float X, float Duty, float Smoothing);

	/** Rises from -1 to +1 and drops back once per cycle, at X = 0.5. */
	PROCEDURALCURVES_API float Sawtooth(float X, float Smoothing);

	/** Rectified sine: touches -1 at every whole X, peaks at +1 halfway. */
	PROCEDURALCURVES_API float Bounce(float X);

	/** Uniform value in [-1, 1] per whole Index. */
	PROCEDURALCURVES_API float Knot(int32 Index, int32 Seed);

	/**
	 * Catmull-Rom through Knot values at whole X. Overshoots [-1, 1] slightly.
	 * @param Period  Knot count after which the noise repeats; 0 or less never repeats.
	 */
	PROCEDURALCURVES_API float Noise(float X, int32 Seed, int32 Period);

	/** Holds each Knot value from one whole X to the next. */
	PROCEDURALCURVES_API float SteppedNoise(float X, int32 Seed, int32 Period, float Smoothing);
}
