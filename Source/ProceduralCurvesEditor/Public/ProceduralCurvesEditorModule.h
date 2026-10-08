// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Modules/ModuleManager.h"

/** Registers the details customizations of FProceduralCurve, FProceduralVectorCurve, FProceduralRotatorCurve and FProceduralCurveTerm. */
class FProceduralCurvesEditorModule : public IModuleInterface
{
#pragma region Lifecycle
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
#pragma endregion Lifecycle
};
