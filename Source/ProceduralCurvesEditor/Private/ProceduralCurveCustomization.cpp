// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurveCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "ProceduralCurve.h"
#include "ProceduralExpression.h"
#include "PropertyHandle.h"
#include "SProceduralCurvePreview.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "ProceduralCurveCustomization"

namespace
{
	const FLinearColor CurveColor(1.0f, 0.62f, 0.2f);
	const FLinearColor AxisColors[3] = { FLinearColor(0.95f, 0.32f, 0.3f), FLinearColor(0.42f, 0.88f, 0.36f), FLinearColor(0.38f, 0.6f, 1.0f) };

	/** First edited instance; the plots show only that one when several objects are selected. */
	template<typename T>
	const T* GetFirstRaw(const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (!Handle.IsValid() || !Handle->IsValidHandle())
		{
			return nullptr;
		}
		TArray<const void*> RawData;
		Handle->AccessRawData(RawData);
		return RawData.Num() > 0 ? static_cast<const T*>(RawData[0]) : nullptr;
	}

	TFunction<void(TArray<FProceduralCurvePreviewSeries>&)> MakeCurveSeries(TSharedPtr<IPropertyHandle> Handle)
	{
		return [Handle](TArray<FProceduralCurvePreviewSeries>& OutSeries)
		{
			if (const FProceduralCurve* Curve = GetFirstRaw<FProceduralCurve>(Handle))
			{
				OutSeries.Add({ [Curve](float Time) { return Curve->Evaluate(Time); }, CurveColor });
			}
		};
	}

	/** FProceduralCurve members of a group struct, in declaration order. */
	TArray<TSharedPtr<IPropertyHandle>> GetComponentCurves(const TSharedRef<IPropertyHandle>& GroupHandle)
	{
		TArray<TSharedPtr<IPropertyHandle>> Components;
		uint32 NumChildren = 0;
		GroupHandle->GetNumChildren(NumChildren);
		for (uint32 Index = 0; Index < NumChildren; ++Index)
		{
			const TSharedPtr<IPropertyHandle> Child = GroupHandle->GetChildHandle(Index);
			const FStructProperty* ChildStruct = Child.IsValid() ? CastField<FStructProperty>(Child->GetProperty()) : nullptr;
			if (ChildStruct && ChildStruct->Struct == FProceduralCurve::StaticStruct())
			{
				Components.Add(Child);
			}
		}
		return Components;
	}

	FLinearColor GetComponentColor(int32 Index)
	{
		return AxisColors[Index % UE_ARRAY_COUNT(AxisColors)];
	}

	TFunction<void(TArray<FProceduralCurvePreviewSeries>&)> MakeGroupSeries(TArray<TSharedPtr<IPropertyHandle>> Components)
	{
		return [Components](TArray<FProceduralCurvePreviewSeries>& OutSeries)
		{
			for (int32 Index = 0; Index < Components.Num(); ++Index)
			{
				if (const FProceduralCurve* Curve = GetFirstRaw<FProceduralCurve>(Components[Index]))
				{
					OutSeries.Add({ [Curve](float Time) { return Curve->Evaluate(Time); }, GetComponentColor(Index) });
				}
			}
		};
	}

	TSharedRef<SWidget> MakePlot(TFunction<void(TArray<FProceduralCurvePreviewSeries>&)> OnGetSeries, float Height, float Cycles, bool bShowLabels)
	{
		return SNew(SBox)
			.Padding(FMargin(0.0f, 2.0f))
			[
				SNew(SProceduralCurvePreview)
				.OnGetSeries(MoveTemp(OnGetSeries))
				.Height(Height)
				.Cycles(Cycles)
				.bShowLabels(bShowLabels)
			];
	}

	bool IsInsideCurveGroup(const TSharedRef<IPropertyHandle>& Handle)
	{
		const TSharedPtr<IPropertyHandle> Parent = Handle->GetParentHandle();
		const FStructProperty* ParentStruct = Parent.IsValid() ? CastField<FStructProperty>(Parent->GetProperty()) : nullptr;
		return ParentStruct && (ParentStruct->Struct == FProceduralVectorCurve::StaticStruct() || ParentStruct->Struct == FProceduralRotatorCurve::StaticStruct());
	}

	FText BuildFunctionHelp()
	{
		FString Help = TEXT("Variables: t (curve time), x (Frequency * t + Phase / 360, in cycles), p (frac(x)).\n")
			TEXT("Constants: pi, tau, e.\n")
			TEXT("Operators: + - * / % ^, unary - and !, < <= > >= == !=, && ||, c ? a : b.\n")
			TEXT("Result is the waveform value: Amplitude, Bias, Exponent and Range still apply.\n\nFunctions:\n");
		for (const ProceduralExpression::FFunctionInfo& Function : ProceduralExpression::GetFunctions())
		{
			Help += FString::Printf(TEXT("%s  %s\n"), Function.Signature, Function.Description);
		}
		return FText::FromString(Help.TrimEnd());
	}
}

#pragma region FProceduralCurveCustomization

TSharedRef<IPropertyTypeCustomization> FProceduralCurveCustomization::MakeInstance()
{
	return MakeShared<FProceduralCurveCustomization>();
}

void FProceduralCurveCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(220.0f)
		[
			MakePlot(MakeCurveSeries(PropertyHandle), 32.0f, 1.0f, false)
		];
}

void FProceduralCurveCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// Inside a vector or rotator curve the combined plot of the parent already shows this component at full size.
	if (!IsInsideCurveGroup(PropertyHandle))
	{
		ChildBuilder.AddCustomRow(LOCTEXT("PreviewFilter", "Preview"))
			.WholeRowContent()
			[
				MakePlot(MakeCurveSeries(PropertyHandle), 110.0f, 2.0f, true)
			];
	}

	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		if (const TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(Index))
		{
			ChildBuilder.AddProperty(Child.ToSharedRef());
		}
	}
}

#pragma endregion FProceduralCurveCustomization

#pragma region FProceduralCurveGroupCustomization

TSharedRef<IPropertyTypeCustomization> FProceduralCurveGroupCustomization::MakeInstance()
{
	return MakeShared<FProceduralCurveGroupCustomization>();
}

void FProceduralCurveGroupCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(220.0f)
		[
			MakePlot(MakeGroupSeries(GetComponentCurves(PropertyHandle)), 32.0f, 1.0f, false)
		];
}

void FProceduralCurveGroupCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TArray<TSharedPtr<IPropertyHandle>> Components = GetComponentCurves(PropertyHandle);

	TSharedRef<SHorizontalBox> Legend = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Components.Num(); ++Index)
	{
		Legend->AddSlot()
			.AutoWidth()
			.Padding(FMargin(0.0f, 0.0f, 12.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(Components[Index]->GetPropertyDisplayName())
				.Font(IDetailLayoutBuilder::GetDetailFontBold())
				.ColorAndOpacity(GetComponentColor(Index))
			];
	}

	ChildBuilder.AddCustomRow(LOCTEXT("GroupPreviewFilter", "Preview"))
		.WholeRowContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				MakePlot(MakeGroupSeries(Components), 120.0f, 2.0f, true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 2.0f, 0.0f, 4.0f))
			[
				Legend
			]
		];

	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		if (const TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(Index))
		{
			ChildBuilder.AddProperty(Child.ToSharedRef());
		}
	}
}

#pragma endregion FProceduralCurveGroupCustomization

#pragma region FProceduralCurveTermCustomization

TSharedRef<IPropertyTypeCustomization> FProceduralCurveTermCustomization::MakeInstance()
{
	return MakeShared<FProceduralCurveTermCustomization>();
}

void FProceduralCurveTermCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> EnabledHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, bEnabled));
	const TSharedPtr<IPropertyHandle> WaveformHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Waveform));
	const TSharedPtr<IPropertyHandle> BlendHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Blend));

	const TSharedPtr<IPropertyHandle> TermHandle = PropertyHandle;
	auto GetSeries = [TermHandle](TArray<FProceduralCurvePreviewSeries>& OutSeries)
	{
		if (const FProceduralCurveTerm* Term = GetFirstRaw<FProceduralCurveTerm>(TermHandle))
		{
			const FLinearColor Color = Term->bEnabled ? CurveColor : FLinearColor(0.4f, 0.4f, 0.4f);
			OutSeries.Add({ [Term](float Time) { return Term->Evaluate(Time); }, Color });
		}
	};

	auto GetSummary = [TermHandle]()
	{
		const FProceduralCurveTerm* Term = GetFirstRaw<FProceduralCurveTerm>(TermHandle);
		if (!Term)
		{
			return FText::GetEmpty();
		}
		if (Term->Waveform == EProceduralWaveform::Expression)
		{
			return FText::FromString(Term->Expression.IsEmpty() ? TEXT("(empty formula)") : Term->Expression.Replace(TEXT("\n"), TEXT(" ")).Left(48));
		}
		return FText::FromString(FString::Printf(TEXT("A %.3g   f %.3g   %.0f\u00B0%s"), Term->Amplitude, Term->Frequency, Term->Phase,
			Term->Bias != 0.0f ? *FString::Printf(TEXT("   +%.3g"), Term->Bias) : TEXT("")));
	};

	HeaderRow
		.NameContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				EnabledHandle->CreatePropertyValueWidget()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
		]
		.ValueContent()
		.MinDesiredWidth(420.0f)
		.MaxDesiredWidth(800.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SBox)
				.MinDesiredWidth(110.0f)
				[
					WaveformHandle->CreatePropertyValueWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
			[
				SNew(SBox)
				.MinDesiredWidth(80.0f)
				[
					BlendHandle->CreatePropertyValueWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0.0f, 0.0f, 6.0f, 0.0f))
			[
				SNew(SBox)
				.WidthOverride(72.0f)
				[
					SNew(SProceduralCurvePreview)
					.OnGetSeries(GetSeries)
					.Height(20.0f)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text_Lambda(GetSummary)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				PropertyHandle->CreateDefaultPropertyButtonWidgets()
			]
		];
}

void FProceduralCurveTermCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const TSharedPtr<IPropertyHandle> TermHandle = PropertyHandle;
	const TSharedPtr<IPropertyHandle> ExpressionHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Expression));

	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		const TSharedPtr<IPropertyHandle> Child = PropertyHandle->GetChildHandle(Index);
		if (!Child.IsValid())
		{
			continue;
		}

		const FName Name = Child->GetProperty()->GetFName();
		// Header fields, and the formula box below.
		if (Name == GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, bEnabled)
			|| Name == GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Waveform)
			|| Name == GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Blend)
			|| Name == GET_MEMBER_NAME_CHECKED(FProceduralCurveTerm, Expression))
		{
			continue;
		}
		ChildBuilder.AddProperty(Child.ToSharedRef());
	}

	TSharedRef<FText> Error = MakeShared<FText>();
	auto Validate = [Error](const FString& Source)
	{
		FText Message;
		*Error = Source.IsEmpty() || ProceduralExpression::Validate(Source, Message) ? FText::GetEmpty() : Message;
	};

	FString CurrentSource;
	if (ExpressionHandle->GetValue(CurrentSource) == FPropertyAccess::Success)
	{
		Validate(CurrentSource);
	}

	ChildBuilder.AddCustomRow(LOCTEXT("ExpressionFilter", "Expression"))
		.Visibility(TAttribute<EVisibility>::CreateLambda([TermHandle]()
		{
			const FProceduralCurveTerm* Term = GetFirstRaw<FProceduralCurveTerm>(TermHandle);
			return Term && Term->Waveform == EProceduralWaveform::Expression ? EVisibility::Visible : EVisibility::Collapsed;
		}))
		.NameContent()
		[
			ExpressionHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth(320.0f)
		.MaxDesiredWidth(800.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SMultiLineEditableTextBox)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AutoWrapText(true)
				.HintText(LOCTEXT("ExpressionHint", "e.g. sine(x) + 0.3 * sine(2 * x + 0.25)"))
				.Text_Lambda([ExpressionHandle]()
				{
					FString Source;
					ExpressionHandle->GetValue(Source);
					return FText::FromString(Source);
				})
				.OnTextChanged_Lambda([Validate](const FText& Text)
				{
					Validate(Text.ToString());
				})
				.OnTextCommitted_Lambda([ExpressionHandle, Validate](const FText& Text, ETextCommit::Type)
				{
					const FString NewSource = Text.ToString();
					Validate(NewSource);

					// A multi-selection with differing formulas shows an empty box: leaving it untouched must not wipe them all.
					FString CurrentSource;
					const FPropertyAccess::Result Access = ExpressionHandle->GetValue(CurrentSource);
					const bool bUntouchedMultiple = Access == FPropertyAccess::MultipleValues && NewSource.IsEmpty();
					const bool bUnchanged = Access == FPropertyAccess::Success && NewSource.Equals(CurrentSource, ESearchCase::CaseSensitive);
					if (!bUntouchedMultiple && !bUnchanged)
					{
						ExpressionHandle->SetValue(NewSource);
					}
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 2.0f))
			[
				SNew(STextBlock)
				.Text_Lambda([Error]() { return *Error; })
				.Visibility_Lambda([Error]() { return Error->IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
				.ColorAndOpacity(FLinearColor(1.0f, 0.3f, 0.25f))
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ExpressionHelp", "t, x, p, pi, tau, e and functions; hover for the full list"))
				.ToolTipText(BuildFunctionHelp())
				.Font(IDetailLayoutBuilder::GetDetailFontItalic())
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

#pragma endregion FProceduralCurveTermCustomization

#undef LOCTEXT_NAMESPACE
