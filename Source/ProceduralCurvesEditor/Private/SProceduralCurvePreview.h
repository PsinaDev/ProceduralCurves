// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "CoreTypes.h"
#include "Math/Color.h"
#include "Templates/Function.h"
#include "Widgets/SLeafWidget.h"

struct FProceduralCurvePreviewSeries
{
	TFunction<float(float)> Sample;
	FLinearColor Color = FLinearColor::White;
};

/**
 * Plots one or more series over [0, Cycles] with an auto-fitted vertical range that always includes 0.
 * Repaints every frame; the series are fetched anew on each paint, so edits show at once.
 */
class SProceduralCurvePreview : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SProceduralCurvePreview)
		: _Cycles(1.0f)
		, _Height(40.0f)
		, _bShowLabels(false)
	{}
		/** Fills the series to plot; called on every paint. */
		SLATE_ARGUMENT(TFunction<void(TArray<FProceduralCurvePreviewSeries>&)>, OnGetSeries)
		SLATE_ARGUMENT(float, Cycles)
		SLATE_ARGUMENT(float, Height)
		/** Min and max values and a seam marker where the value jumps across a whole time. */
		SLATE_ARGUMENT(bool, bShowLabels)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

#pragma region SWidget
public:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
#pragma endregion SWidget

private:
	TFunction<void(TArray<FProceduralCurvePreviewSeries>&)> OnGetSeries;
	float Cycles = 1.0f;
	float Height = 40.0f;
	bool bShowLabels = false;
};
