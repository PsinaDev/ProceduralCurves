// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "UObject/ObjectMacros.h"
#include <atomic>
#include "ProceduralCurve.generated.h"

namespace ProceduralExpression
{
	class FProgram;
}

/**
 * Program a term last resolved for its Expression; copies carry it, and a term re-resolves when its text no longer matches.
 * @note Points into ProceduralExpression's process-wide cache, which never frees a program.
 */
struct FProceduralExpressionCache
{
	FProceduralExpressionCache() = default;

	FProceduralExpressionCache(const FProceduralExpressionCache& Other)
		: Program(Other.Program.load(std::memory_order_relaxed))
	{
	}

	FProceduralExpressionCache& operator=(const FProceduralExpressionCache& Other)
	{
		Program.store(Other.Program.load(std::memory_order_relaxed), std::memory_order_relaxed);
		return *this;
	}

	std::atomic<const ProceduralExpression::FProgram*> Program { nullptr };
};

UENUM(BlueprintType)
enum class EProceduralWaveform : uint8
{
	Sine,
	/** Peak position set by Skew; Skew 1 or 0 gives a sawtooth. */
	Triangle,
	/** High for Duty of each cycle; Smoothing rounds the jumps. */
	Square,
	/** Rising ramp; Smoothing rounds the drop. */
	Sawtooth,
	/** Rectified sine, touching the bottom once per cycle. */
	Bounce,
	/** Smooth random values on Frequency knots per unit of time. */
	Noise,
	/** Random values held between Frequency knots per unit of time; Smoothing rounds the steps. */
	SteppedNoise,
	/** Formula typed into Expression. */
	Expression
};

UENUM(BlueprintType)
enum class EProceduralTermBlend : uint8
{
	/** Adds the term to the terms above it. */
	Add,
	/** Multiplies everything above it by the term. */
	Multiply
};

UENUM(BlueprintType)
enum class EProceduralTermRange : uint8
{
	/** The waveform spans [-1, 1]. */
	Bipolar,
	/** The waveform is remapped to [0, 1]. */
	Unipolar
};

/**
 * One wave of an FProceduralCurve: Bias + Amplitude * waveform(x), x = Frequency * Time + Phase / 360 in cycles.
 * Noise knots sit at whole x and repeat every round(Frequency) knots, so whole-number frequencies loop over a unit of time like the periodic shapes.
 */
USTRUCT(BlueprintType)
struct PROCEDURALCURVES_API FProceduralCurveTerm
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	EProceduralWaveform Waveform = EProceduralWaveform::Sine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	EProceduralTermBlend Blend = EProceduralTermBlend::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	float Amplitude = 1.0f;

	/** Cycles per unit of time. Whole numbers keep the curve seamless across a unit of time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	float Frequency = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term", meta = (Units = "deg"))
	float Phase = 0.0f;

	/** Added after Amplitude. A Multiply term of Bias 1 and Amplitude 0.2 scales by 0.8..1.2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term")
	float Bias = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape")
	EProceduralTermRange Range = EProceduralTermRange::Bipolar;

	/** Signed power on the bipolar waveform before Range: above 1 sharpens the peaks, below 1 flattens them toward a square. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "8"))
	float Exponent = 1.0f;

	/** Peak position as a fraction of the cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape",
		meta = (EditCondition = "Waveform == EProceduralWaveform::Triangle", EditConditionHides, ClampMin = "0", ClampMax = "1"))
	float Skew = 0.5f;

	/** High fraction of the cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape",
		meta = (EditCondition = "Waveform == EProceduralWaveform::Square", EditConditionHides, ClampMin = "0", ClampMax = "1"))
	float Duty = 0.5f;

	/** Width of each rounded jump as a fraction of a cycle or of a knot spacing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape",
		meta = (EditCondition = "Waveform == EProceduralWaveform::Square || Waveform == EProceduralWaveform::Sawtooth || Waveform == EProceduralWaveform::SteppedNoise",
			EditConditionHides, ClampMin = "0", ClampMax = "1"))
	float Smoothing = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape",
		meta = (EditCondition = "Waveform == EProceduralWaveform::Noise || Waveform == EProceduralWaveform::SteppedNoise", EditConditionHides))
	int32 Seed = 0;

	/**
	 * Waveform value as a formula of t, x and p; see ProceduralExpression.h for the grammar and functions.
	 * Compiled once per distinct text and cached for the process; an invalid formula evaluates to 0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Term|Shape",
		meta = (EditCondition = "Waveform == EProceduralWaveform::Expression", EditConditionHides, MultiLine = true))
	FString Expression;

	FProceduralCurveTerm() = default;

	FProceduralCurveTerm(EProceduralWaveform InWaveform, float InAmplitude, float InFrequency, float InPhase)
		: Waveform(InWaveform), Amplitude(InAmplitude), Frequency(InFrequency), Phase(InPhase)
	{
	}

	/** Bias + Amplitude * shaped waveform at Time. Thread-safe. */
	float Evaluate(float Time) const;

private:
	const ProceduralExpression::FProgram* ResolveExpression() const;

	mutable FProceduralExpressionCache ExpressionCache;
};

/**
 * Formula-defined curve: terms applied top to bottom (Add sums, Multiply scales the running result), then Offset, then the optional clamp.
 * Continuous in time and thread-safe to evaluate. A unit of time is one cycle for whole-number frequencies.
 */
USTRUCT(BlueprintType)
struct PROCEDURALCURVES_API FProceduralCurve
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	TArray<FProceduralCurveTerm> Terms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	float Offset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	bool bClampOutput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve", meta = (EditCondition = "bClampOutput", EditConditionHides))
	float OutputMin = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve", meta = (EditCondition = "bClampOutput", EditConditionHides))
	float OutputMax = 1.0f;

	float Evaluate(float Time) const;

	/** True when every enabled Add term has zero amplitude and bias and Offset is zero. */
	bool IsZero() const;

	FProceduralCurve& Add(EProceduralWaveform Waveform, float Amplitude, float Frequency, float Phase);

	FProceduralCurve& AddSine(float Amplitude, float Frequency, float Phase)
	{
		return Add(EProceduralWaveform::Sine, Amplitude, Frequency, Phase);
	}

	/** Scales the output linearly: Offset and the amplitude and bias of every Add term. Multiply terms are factors and stay. */
	FProceduralCurve& Scale(float Factor);
};

USTRUCT(BlueprintType)
struct PROCEDURALCURVES_API FProceduralVectorCurve
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve Y;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve Z;

	FVector Evaluate(float Time) const
	{
		return FVector(X.Evaluate(Time), Y.Evaluate(Time), Z.Evaluate(Time));
	}

	bool IsZero() const
	{
		return X.IsZero() && Y.IsZero() && Z.IsZero();
	}

	FProceduralVectorCurve& Scale(float Factor)
	{
		X.Scale(Factor);
		Y.Scale(Factor);
		Z.Scale(Factor);
		return *this;
	}
};

/** Rotation in degrees, one curve per FRotator component. */
USTRUCT(BlueprintType)
struct PROCEDURALCURVES_API FProceduralRotatorCurve
{
	GENERATED_BODY()

	/** Around the forward (X) axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve Roll;

	/** Around the right (Y) axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve Pitch;

	/** Around the up (Z) axis. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curve")
	FProceduralCurve Yaw;

	FRotator Evaluate(float Time) const
	{
		return FRotator(Pitch.Evaluate(Time), Yaw.Evaluate(Time), Roll.Evaluate(Time));
	}

	bool IsZero() const
	{
		return Roll.IsZero() && Pitch.IsZero() && Yaw.IsZero();
	}

	FProceduralRotatorCurve& Scale(float Factor)
	{
		Roll.Scale(Factor);
		Pitch.Scale(Factor);
		Yaw.Scale(Factor);
		return *this;
	}
};
