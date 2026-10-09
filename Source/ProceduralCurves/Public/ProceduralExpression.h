// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "Internationalization/Text.h"
#include "Templates/SharedPointer.h"

/**
 * Math expressions for procedural curves, compiled to a stack program.
 *
 * Variables: t (curve time), x (the term's argument in cycles, Frequency * t + Phase / 360), p (frac(x)).
 * Constants: pi, tau, e. Operators: + - * / % ^ (power, right-associative), unary - and !, < <= > >= == !=, && ||, c ? a : b.
 * Comparisons and logic yield 1 or 0; any non-zero value is true. Names are case-insensitive.
 * Division, modulo, sqrt and log by an invalid argument return 0 instead of NaN.
 */
namespace ProceduralExpression
{
	struct FFunctionInfo
	{
		const TCHAR* Name;
		const TCHAR* Signature;
		const TCHAR* Description;
	};

	struct FInputs
	{
		float T = 0.0f;
		float X = 0.0f;
	};

	class FProgram;

	/**
	 * Compiled program of Source, shared through a process-wide cache keyed by the source text, case-insensitively.
	 * @return Never null; an invalid program evaluates to 0 and logs its error once.
	 * @note Thread-safe. The cache never frees a program, so the referenced program outlives every caller; it grows by one small
	 *       entry per distinct formula ever evaluated.
	 */
	PROCEDURALCURVES_API TSharedRef<const FProgram> FindOrCompile(const FString& Source);

	/** Whether Program was compiled from Source, compared case-insensitively like the cache key. Thread-safe. */
	PROCEDURALCURVES_API bool MatchesSource(const FProgram& Program, const FString& Source);

	/** Compiles without touching the cache. @return false with a message in OutError. */
	PROCEDURALCURVES_API bool Validate(const FString& Source, FText& OutError);

	/** Thread-safe. */
	PROCEDURALCURVES_API float Evaluate(const FProgram& Program, const FInputs& Inputs);

	PROCEDURALCURVES_API bool IsValid(const FProgram& Program);

	PROCEDURALCURVES_API TArrayView<const FFunctionInfo> GetFunctions();
}
