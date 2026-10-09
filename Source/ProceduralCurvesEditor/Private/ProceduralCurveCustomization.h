// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "IPropertyTypeCustomization.h"
#include "Templates/SharedPointer.h"

class IPropertyHandle;

/** FProceduralCurve: inline plot in the header, a two-cycle plot with range labels above the terms. */
class FProceduralCurveCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

#pragma region IPropertyTypeCustomization
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
#pragma endregion IPropertyTypeCustomization
};

/** FProceduralVectorCurve and FProceduralRotatorCurve: every FProceduralCurve member plotted together in red, green and blue, legend from the member names. */
class FProceduralCurveGroupCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

#pragma region IPropertyTypeCustomization
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
#pragma endregion IPropertyTypeCustomization
};

/** FProceduralCurveTerm: one header line with toggle, waveform, blend, a sparkline and a summary; a validated formula box for Expression. */
class FProceduralCurveTermCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

#pragma region IPropertyTypeCustomization
public:
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
#pragma endregion IPropertyTypeCustomization
};
