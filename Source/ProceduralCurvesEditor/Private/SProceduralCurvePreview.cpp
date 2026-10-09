// Copyright PsinaDev. All Rights Reserved.

#include "SProceduralCurvePreview.h"
#include "Fonts/SlateFontInfo.h"
#include "Math/UnrealMathUtility.h"
#include "Rendering/DrawElements.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Styling/CoreStyle.h"

namespace
{
	// Slate drawing moved from FVector2D to FVector2f in 5.2; earlier 5.x overloads take FVector2D only.
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2)
	using FPlotVector = FVector2f;
#else
	using FPlotVector = FVector2D;
#endif

	FPlotVector MakePoint(float X, float Y)
	{
		return FPlotVector(X, Y);
	}

	void DrawLine(FSlateWindowElementList& OutDrawElements, int32 Layer, const FPaintGeometry& Geometry, float X0, float Y0, float X1, float Y1,
		const FLinearColor& Color, float Thickness)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, Geometry, TArray<FPlotVector>{ MakePoint(X0, Y0), MakePoint(X1, Y1) },
			ESlateDrawEffect::None, Color, false, Thickness);
	}

	void DrawLabel(FSlateWindowElementList& OutDrawElements, int32 Layer, const FGeometry& AllottedGeometry, float Width, float Height,
		float X, float Y, const FString& Text, const FSlateFontInfo& Font, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeText(OutDrawElements, Layer,
			AllottedGeometry.ToPaintGeometry(MakePoint(Width, Height), FSlateLayoutTransform(MakePoint(X, Y))), Text, Font, ESlateDrawEffect::None, Color);
	}
}

void SProceduralCurvePreview::Construct(const FArguments& InArgs)
{
	OnGetSeries = InArgs._OnGetSeries;
	Cycles = FMath::Max(InArgs._Cycles, 0.01f);
	Height = InArgs._Height;
	bShowLabels = InArgs._bShowLabels;

	// The plotted structs change through the details panel without invalidating this widget.
	ForceVolatile(true);
}

FVector2D SProceduralCurvePreview::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(120.0, Height);
}

int32 SProceduralCurvePreview::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const float Width = static_cast<float>(AllottedGeometry.GetLocalSize().X);
	const float PlotHeight = static_cast<float>(AllottedGeometry.GetLocalSize().Y);
	if (Width < 2.0f || PlotHeight < 2.0f)
	{
		return LayerId;
	}

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), FCoreStyle::Get().GetBrush("GenericWhiteBox"),
		ESlateDrawEffect::None, FLinearColor(0.012f, 0.012f, 0.012f, 1.0f));

	TArray<FProceduralCurvePreviewSeries> Series;
	if (OnGetSeries)
	{
		OnGetSeries(Series);
	}

	const int32 NumSamples = FMath::Clamp(FMath::CeilToInt(Width), 16, 1024);
	TArray<TArray<float>> Values;
	Values.SetNum(Series.Num());

	float Min = 0.0f;
	float Max = 0.0f;
	for (int32 SeriesIndex = 0; SeriesIndex < Series.Num(); ++SeriesIndex)
	{
		TArray<float>& SeriesValues = Values[SeriesIndex];
		SeriesValues.SetNumUninitialized(NumSamples + 1);
		for (int32 Sample = 0; Sample <= NumSamples; ++Sample)
		{
			float Value = Series[SeriesIndex].Sample ? Series[SeriesIndex].Sample(Cycles * Sample / NumSamples) : 0.0f;
			Value = FMath::IsFinite(Value) ? Value : 0.0f;
			SeriesValues[Sample] = Value;
			Min = FMath::Min(Min, Value);
			Max = FMath::Max(Max, Value);
		}
	}

	if (Max - Min < 1.0e-4f)
	{
		Min -= 0.5f;
		Max += 0.5f;
	}
	const float Padding = 0.08f * (Max - Min);
	const float PlotMin = Min - Padding;
	const float PlotRange = Max - Min + 2.0f * Padding;
	auto ToY = [PlotHeight, PlotMin, PlotRange](float Value)
	{
		return PlotHeight * (1.0f - (Value - PlotMin) / PlotRange);
	};

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const int32 NumBoundaries = FMath::CeilToInt(Cycles);

	const float ZeroY = ToY(0.0f);
	DrawLine(OutDrawElements, LayerId + 1, PaintGeometry, 0.0f, ZeroY, Width, ZeroY, FLinearColor(1.0f, 1.0f, 1.0f, 0.22f), 1.0f);
	for (int32 Boundary = 1; Boundary < NumBoundaries; ++Boundary)
	{
		const float BoundaryX = Width * Boundary / Cycles;
		DrawLine(OutDrawElements, LayerId + 1, PaintGeometry, BoundaryX, 0.0f, BoundaryX, PlotHeight, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 1.0f);
	}

	TArray<FPlotVector> Points;
	Points.Reserve(NumSamples + 1);
	for (int32 SeriesIndex = 0; SeriesIndex < Series.Num(); ++SeriesIndex)
	{
		Points.Reset();
		for (int32 Sample = 0; Sample <= NumSamples; ++Sample)
		{
			Points.Add(MakePoint(Width * Sample / NumSamples, ToY(Values[SeriesIndex][Sample])));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, PaintGeometry, Points, ESlateDrawEffect::None, Series[SeriesIndex].Color, true, 1.5f);
	}

	if (bShowLabels)
	{
		const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 8);
		const FLinearColor LabelColor(1.0f, 1.0f, 1.0f, 0.55f);
		DrawLabel(OutDrawElements, LayerId + 3, AllottedGeometry, Width, PlotHeight, 4.0f, 2.0f, FString::Printf(TEXT("%.3g"), Max), Font, LabelColor);
		DrawLabel(OutDrawElements, LayerId + 3, AllottedGeometry, Width, PlotHeight, 4.0f, PlotHeight - 14.0f, FString::Printf(TEXT("%.3g"), Min), Font, LabelColor);

		// A loop seam is a value jump from the end of one cycle to the start of the next.
		bool bSeam = false;
		for (const FProceduralCurvePreviewSeries& Entry : Series)
		{
			if (Entry.Sample && FMath::Abs(Entry.Sample(1.0f - KINDA_SMALL_NUMBER) - Entry.Sample(0.0f)) > 0.02f * (Max - Min))
			{
				bSeam = true;
				break;
			}
		}
		if (bSeam)
		{
			const FLinearColor SeamColor(1.0f, 0.25f, 0.2f, 0.9f);
			for (int32 Boundary = 1; Boundary < NumBoundaries; ++Boundary)
			{
				const float BoundaryX = Width * Boundary / Cycles;
				DrawLine(OutDrawElements, LayerId + 3, PaintGeometry, BoundaryX, 0.0f, BoundaryX, PlotHeight, SeamColor, 2.0f);
			}
			DrawLabel(OutDrawElements, LayerId + 3, AllottedGeometry, Width, PlotHeight, FMath::Max(Width - 120.0f, 4.0f), 2.0f,
				TEXT("Jumps at the loop seam"), Font, SeamColor);
		}
	}

	return LayerId + 4;
}
