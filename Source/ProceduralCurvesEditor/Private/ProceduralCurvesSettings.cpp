// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurvesSettings.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(ProceduralCurvesSettings)
#endif

#define LOCTEXT_NAMESPACE "ProceduralCurvesSettings"

FName UProceduralCurvesSettings::GetCategoryName() const
{
	return FName(TEXT("Plugins"));
}

FText UProceduralCurvesSettings::GetSectionText() const
{
	return LOCTEXT("SettingsSection", "Procedural Curve Presets");
}

FText UProceduralCurvesSettings::GetSectionDescription() const
{
	return LOCTEXT("SettingsDescription", "User-created procedural curve presets");
}

#undef LOCTEXT_NAMESPACE
