// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "CoreTypes.h"
#include "ProceduralCurvesSettings.h"

class IPropertyHandle;

namespace ProceduralCurvePresets
{
	/** Single Expression-term curves, built once. */
	const TArray<FProceduralCurvePresetData>& GetBuiltIn();

	TArray<FProceduralCurvePresetData> GetUser();

	/** Name of the user, then built-in, preset whose curve equals Curve in every property; empty when none does. */
	FString FindMatchingName(const FProceduralCurve& Curve);

	/** User presets shadow built-in presets of the same name. */
	bool Find(const FString& Name, FProceduralCurvePresetData& OutPreset);

	/** Replaces every curve the handle edits with the preset's curve, as one undoable property change. */
	void Apply(IPropertyHandle& CurveHandle, const FProceduralCurvePresetData& Preset);

	/**
	 * Applies the preset Direction steps away from the one the first edited curve matches, wrapping around built-in then user presets.
	 * A curve matching no preset steps to the first preset forwards and the last one backwards.
	 */
	void Step(IPropertyHandle& CurveHandle, int32 Direction);

	/** Stores Curve under Name, replacing a user preset of the same name. */
	void SaveUser(const FString& Name, const FString& Category, const FString& Description, const FProceduralCurve& Curve);

	bool DeleteUser(const FString& Name);
}
