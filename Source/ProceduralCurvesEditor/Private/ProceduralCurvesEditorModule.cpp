// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurvesEditorModule.h"
#include "ProceduralCurve.h"
#include "ProceduralCurveCustomization.h"
#include "PropertyEditorModule.h"

void FProceduralCurvesEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyTypeLayout(FProceduralCurve::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FProceduralCurveCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FProceduralVectorCurve::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FProceduralCurveGroupCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FProceduralRotatorCurve::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FProceduralCurveGroupCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FProceduralCurveTerm::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FProceduralCurveTermCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FProceduralCurvesEditorModule::ShutdownModule()
{
	if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyModule->UnregisterCustomPropertyTypeLayout(FProceduralCurve::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomPropertyTypeLayout(FProceduralVectorCurve::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomPropertyTypeLayout(FProceduralRotatorCurve::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomPropertyTypeLayout(FProceduralCurveTerm::StaticStruct()->GetFName());
		PropertyModule->NotifyCustomizationModuleChanged();
	}
}

IMPLEMENT_MODULE(FProceduralCurvesEditorModule, ProceduralCurvesEditor)
