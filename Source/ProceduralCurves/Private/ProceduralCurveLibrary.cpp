// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurveLibrary.h"
#include "Math/UnrealMathUtility.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(ProceduralCurveLibrary)
#endif

float UProceduralCurveLibrary::EvaluateProceduralCurve(const FProceduralCurve& Curve, float Time)
{
	return Curve.Evaluate(Time);
}

FVector UProceduralCurveLibrary::EvaluateProceduralVectorCurve(const FProceduralVectorCurve& Curve, float Time)
{
	return Curve.Evaluate(Time);
}

FRotator UProceduralCurveLibrary::EvaluateProceduralRotatorCurve(const FProceduralRotatorCurve& Curve, float Time)
{
	return Curve.Evaluate(Time);
}

float UProceduralCurveLibrary::AdvanceProceduralPhase(float DeltaTime, float CycleDuration, float& Phase)
{
	// Wrapping every call keeps Phase small, so precision never degrades however long it runs.
	Phase = CycleDuration > 0.0f ? FMath::Frac(Phase + DeltaTime / CycleDuration) : FMath::Frac(Phase);
	return Phase;
}

float UProceduralCurveLibrary::EvaluateProceduralCurveOnTime(const FProceduralCurve& Curve, float DeltaTime, float CycleDuration, float& Phase)
{
	return Curve.Evaluate(AdvanceProceduralPhase(DeltaTime, CycleDuration, Phase));
}

FVector UProceduralCurveLibrary::EvaluateProceduralVectorCurveOnTime(const FProceduralVectorCurve& Curve, float DeltaTime, float CycleDuration, float& Phase)
{
	return Curve.Evaluate(AdvanceProceduralPhase(DeltaTime, CycleDuration, Phase));
}

FRotator UProceduralCurveLibrary::EvaluateProceduralRotatorCurveOnTime(const FProceduralRotatorCurve& Curve, float DeltaTime, float CycleDuration, float& Phase)
{
	return Curve.Evaluate(AdvanceProceduralPhase(DeltaTime, CycleDuration, Phase));
}
