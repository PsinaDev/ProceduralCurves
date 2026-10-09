// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProceduralCurve.h"
#include "ProceduralCurvesSettings.generated.h"

/** A whole FProceduralCurve under a name; applying it replaces the edited curve. */
USTRUCT()
struct FProceduralCurvePresetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Config, Category = "Preset")
	FString PresetName;

	/** Submenu the preset is listed under; empty lists it directly in its section. */
	UPROPERTY(EditAnywhere, Config, Category = "Preset")
	FString Category;

	UPROPERTY(EditAnywhere, Config, Category = "Preset")
	FString Description;

	UPROPERTY(EditAnywhere, Config, Category = "Preset")
	FProceduralCurve Curve;
};

/** User presets, shared through Config/DefaultEditor.ini. */
UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Procedural Curve Presets"))
class UProceduralCurvesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category = "User Presets", meta = (TitleProperty = "PresetName"))
	TArray<FProceduralCurvePresetData> UserPresets;

	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
};
