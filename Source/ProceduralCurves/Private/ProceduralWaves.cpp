// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralWaves.h"
#include "Math/UnrealMathUtility.h"
#include "ProceduralCurvesCompat.h"

namespace ProceduralWaves
{
	namespace
	{
		int32 WrapIndex(int32 Index, int32 Period)
		{
			return Period > 0 ? ((Index % Period) + Period) % Period : Index;
		}

		/** Smoothed unit step centred on Edge, Width wide; a hard step for zero width. */
		float Edge(float Edge, float Width, float X)
		{
			if (Width <= 0.0f)
			{
				return X >= Edge ? 1.0f : 0.0f;
			}
			return FMath::SmoothStep(Edge - 0.5f * Width, Edge + 0.5f * Width, X);
		}
	}

	float Sine(float X)
	{
		return FMath::Sin(UE_TWO_PI * X);
	}

	float Triangle(float X, float Skew)
	{
		Skew = FMath::Clamp(Skew, 0.0f, 1.0f);
		const float U = FMath::Frac(X + 0.5f * Skew);

		if (Skew <= UE_KINDA_SMALL_NUMBER)
		{
			return 1.0f - 2.0f * U;
		}
		if (Skew >= 1.0f - UE_KINDA_SMALL_NUMBER)
		{
			return 2.0f * U - 1.0f;
		}
		return U < Skew ? -1.0f + 2.0f * U / Skew : 1.0f - 2.0f * (U - Skew) / (1.0f - Skew);
	}

	float Square(float X, float Duty, float Smoothing)
	{
		Duty = FMath::Clamp(Duty, 0.0f, 1.0f);
		const float Width = FMath::Clamp(Smoothing, 0.0f, FMath::Min(Duty, 1.0f - Duty));
		const float U = FMath::Frac(X);

		// Rising edges at 0 and 1 and the falling edge at Duty; with Width at most min(Duty, 1 - Duty) they never overlap.
		const float High = Edge(0.0f, Width, U) - Edge(Duty, Width, U) + Edge(1.0f, Width, U);
		return 2.0f * FMath::Clamp(High, 0.0f, 1.0f) - 1.0f;
	}

	float Sawtooth(float X, float Smoothing)
	{
		const float Width = FMath::Clamp(Smoothing, 0.0f, 1.0f);
		const float U = FMath::Frac(X + 0.5f);
		if (Width <= 0.0f)
		{
			return 2.0f * U - 1.0f;
		}

		// The drop of 2 at U = 1 is spread over Width, half before the wrap and half after it.
		return 2.0f * U - 1.0f - 2.0f * Edge(1.0f, Width, U) + 2.0f * (1.0f - Edge(0.0f, Width, U));
	}

	float Bounce(float X)
	{
		return 2.0f * FMath::Abs(FMath::Sin(UE_PI * X)) - 1.0f;
	}

	float Knot(int32 Index, int32 Seed)
	{
		// lowbias32 integer hash.
		uint32 Hash = static_cast<uint32>(Index) * 0x9E3779B9u ^ static_cast<uint32>(Seed) * 0x85EBCA6Bu;
		Hash ^= Hash >> 16;
		Hash *= 0x7FEB352Du;
		Hash ^= Hash >> 15;
		Hash *= 0x846CA68Bu;
		Hash ^= Hash >> 16;
		return static_cast<float>(Hash & 0x00FFFFFFu) / static_cast<float>(0x00FFFFFFu) * 2.0f - 1.0f;
	}

	float Noise(float X, int32 Seed, int32 Period)
	{
		const int32 Index = FMath::FloorToInt32(X);
		const float T = X - Index;

		const float P0 = Knot(WrapIndex(Index - 1, Period), Seed);
		const float P1 = Knot(WrapIndex(Index, Period), Seed);
		const float P2 = Knot(WrapIndex(Index + 1, Period), Seed);
		const float P3 = Knot(WrapIndex(Index + 2, Period), Seed);

		const float T2 = T * T;
		const float T3 = T2 * T;
		return 0.5f * (2.0f * P1 + (P2 - P0) * T + (2.0f * P0 - 5.0f * P1 + 4.0f * P2 - P3) * T2 + (3.0f * P1 - P0 - 3.0f * P2 + P3) * T3);
	}

	float SteppedNoise(float X, int32 Seed, int32 Period, float Smoothing)
	{
		const int32 Index = FMath::FloorToInt32(X);
		const float T = X - Index;
		const float Width = FMath::Clamp(Smoothing, 0.0f, 1.0f);

		const float Current = Knot(WrapIndex(Index, Period), Seed);
		if (Width <= 0.0f)
		{
			return Current;
		}

		const float Previous = Knot(WrapIndex(Index - 1, Period), Seed);
		const float Next = Knot(WrapIndex(Index + 1, Period), Seed);

		// Each step is spread over Width around its whole X: the tail of the step into Current, then the head of the step into Next.
		return Current + (Current - Previous) * (Edge(0.0f, Width, T) - 1.0f) + (Next - Current) * Edge(1.0f, Width, T);
	}
}
