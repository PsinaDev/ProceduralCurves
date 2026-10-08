// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurve.h"
#include "Math/UnrealMathUtility.h"
#include "ProceduralExpression.h"
#include "ProceduralWaves.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(ProceduralCurve)
#endif

const ProceduralExpression::FProgram* FProceduralCurveTerm::ResolveExpression() const
{
	if (Expression.IsEmpty())
	{
		return nullptr;
	}

	// Fast path without hashing or locking; a race only makes two threads resolve the same program.
	const ProceduralExpression::FProgram* Program = ExpressionCache.Program.load(std::memory_order_acquire);
	if (!Program || !ProceduralExpression::MatchesSource(*Program, Expression))
	{
		Program = &ProceduralExpression::FindOrCompile(Expression).Get();
		ExpressionCache.Program.store(Program, std::memory_order_release);
	}
	return Program;
}

float FProceduralCurveTerm::Evaluate(float Time) const
{
	const float X = Frequency * Time + Phase / 360.0f;
	const int32 KnotPeriod = FMath::RoundToInt32(Frequency);
	float Wave = 0.0f;

	switch (Waveform)
	{
	case EProceduralWaveform::Sine:
		Wave = ProceduralWaves::Sine(X);
		break;
	case EProceduralWaveform::Triangle:
		Wave = ProceduralWaves::Triangle(X, Skew);
		break;
	case EProceduralWaveform::Square:
		Wave = ProceduralWaves::Square(X, Duty, Smoothing);
		break;
	case EProceduralWaveform::Sawtooth:
		Wave = ProceduralWaves::Sawtooth(X, Smoothing);
		break;
	case EProceduralWaveform::Bounce:
		Wave = ProceduralWaves::Bounce(X);
		break;
	case EProceduralWaveform::Noise:
		Wave = ProceduralWaves::Noise(X, Seed, KnotPeriod);
		break;
	case EProceduralWaveform::SteppedNoise:
		Wave = ProceduralWaves::SteppedNoise(X, Seed, KnotPeriod, Smoothing);
		break;
	case EProceduralWaveform::Expression:
		if (const ProceduralExpression::FProgram* Program = ResolveExpression())
		{
			ProceduralExpression::FInputs Inputs;
			Inputs.T = Time;
			Inputs.X = X;
			Wave = ProceduralExpression::Evaluate(*Program, Inputs);
		}
		break;
	}

	if (Exponent != 1.0f)
	{
		Wave = FMath::Sign(Wave) * FMath::Pow(FMath::Abs(Wave), FMath::Max(Exponent, 0.01f));
	}
	if (Range == EProceduralTermRange::Unipolar)
	{
		Wave = 0.5f * Wave + 0.5f;
	}

	return Bias + Amplitude * Wave;
}

float FProceduralCurve::Evaluate(float Time) const
{
	float Value = 0.0f;
	for (const FProceduralCurveTerm& Term : Terms)
	{
		if (!Term.bEnabled)
		{
			continue;
		}

		const float TermValue = Term.Evaluate(Time);
		if (Term.Blend == EProceduralTermBlend::Multiply)
		{
			Value *= TermValue;
		}
		else
		{
			Value += TermValue;
		}
	}

	Value += Offset;
	return bClampOutput ? FMath::Clamp(Value, FMath::Min(OutputMin, OutputMax), FMath::Max(OutputMin, OutputMax)) : Value;
}

bool FProceduralCurve::IsZero() const
{
	if (Offset != 0.0f)
	{
		return false;
	}
	for (const FProceduralCurveTerm& Term : Terms)
	{
		if (Term.bEnabled && Term.Blend == EProceduralTermBlend::Add && (Term.Amplitude != 0.0f || Term.Bias != 0.0f))
		{
			return false;
		}
	}
	return true;
}

FProceduralCurve& FProceduralCurve::Add(EProceduralWaveform Waveform, float Amplitude, float Frequency, float Phase)
{
	Terms.Emplace(Waveform, Amplitude, Frequency, Phase);
	return *this;
}

FProceduralCurve& FProceduralCurve::Scale(float Factor)
{
	Offset *= Factor;
	for (FProceduralCurveTerm& Term : Terms)
	{
		if (Term.Blend == EProceduralTermBlend::Add)
		{
			Term.Amplitude *= Factor;
			Term.Bias *= Factor;
		}
	}
	OutputMin *= Factor;
	OutputMax *= Factor;
	return *this;
}
