// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Math/Vector.h"
#include "ProceduralCurve.h"
#include "UObject/ObjectMacros.h"
#include "ProceduralCurveLibrary.generated.h"

UCLASS()
class PROCEDURALCURVES_API UProceduralCurveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#pragma region Evaluation
public:
	UFUNCTION(BlueprintPure, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static float EvaluateProceduralCurve(const FProceduralCurve& Curve, float Time);

	UFUNCTION(BlueprintPure, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static FVector EvaluateProceduralVectorCurve(const FProceduralVectorCurve& Curve, float Time);

	UFUNCTION(BlueprintPure, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static FRotator EvaluateProceduralRotatorCurve(const FProceduralRotatorCurve& Curve, float Time);
#pragma endregion Evaluation

#pragma region Playback
public:
	/**
	 * Advances Phase by DeltaTime / CycleDuration, wrapped to [0, 1), so one cycle takes CycleDuration seconds whatever the frame rate.
	 *
	 * @param CycleDuration  Seconds per cycle; 0 or less holds Phase.
	 * @param Phase          Persistent state in [0, 1); seed it with the starting phase.
	 * @return The new Phase.
	 */
	UFUNCTION(BlueprintCallable, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static float AdvanceProceduralPhase(float DeltaTime, float CycleDuration, UPARAM(ref) float& Phase);

	/** Advances Phase like AdvanceProceduralPhase and evaluates Curve at it, so the curve runs one full cycle every CycleDuration seconds. */
	UFUNCTION(BlueprintCallable, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static float EvaluateProceduralCurveOnTime(const FProceduralCurve& Curve, float DeltaTime, float CycleDuration, UPARAM(ref) float& Phase);

	/** Vector counterpart of EvaluateProceduralCurveOnTime. */
	UFUNCTION(BlueprintCallable, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static FVector EvaluateProceduralVectorCurveOnTime(const FProceduralVectorCurve& Curve, float DeltaTime, float CycleDuration, UPARAM(ref) float& Phase);

	/** Rotator counterpart of EvaluateProceduralCurveOnTime. */
	UFUNCTION(BlueprintCallable, Category = "Math|Procedural Curve", meta = (BlueprintThreadSafe))
	static FRotator EvaluateProceduralRotatorCurveOnTime(const FProceduralRotatorCurve& Curve, float DeltaTime, float CycleDuration, UPARAM(ref) float& Phase);
#pragma endregion Playback
};
