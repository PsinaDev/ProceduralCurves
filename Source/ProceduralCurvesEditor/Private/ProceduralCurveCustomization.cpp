// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurveCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "ProceduralCurve.h"
#include "ProceduralCurvePresets.h"
#include "ProceduralExpression.h"
#include "PropertyHandle.h"
#include "SProceduralCurvePreview.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SWindow.h"
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

	/**
	 * True when an enclosing array element has no counterpart in the defaults: the engine then reports the element as changed
	 * and none of its members, so members get no reset arrow of their own.
	 */
	bool IsMissingFromDefaults(const TSharedPtr<IPropertyHandle>& Handle)
	{
		for (TSharedPtr<IPropertyHandle> Node = Handle; Node.IsValid(); Node = Node->GetParentHandle())
		{
			if (Node->GetIndexInArray() == INDEX_NONE || !Node->DiffersFromDefault())
			{
				continue;
			}

			uint32 NumChildren = 0;
			Node->GetNumChildren(NumChildren);
			bool bAnyChildDiffers = false;
			for (uint32 Index = 0; Index < NumChildren && !bAnyChildDiffers; ++Index)
			{
				const TSharedPtr<IPropertyHandle> Child = Node->GetChildHandle(Index);
				bAnyChildDiffers = Child.IsValid() && Child->DiffersFromDefault();
			}
			if (NumChildren > 0 && !bAnyChildDiffers)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * True when an edited object's archetype lacks the top-level property, so the engine has nothing to compare against;
	 * e.g. the Default Value of a Blueprint variable, edited on the class default object whose archetype is the parent's.
	 */
	bool HasNoArchetypeValue(const TSharedPtr<IPropertyHandle>& Handle)
	{
		// The top of the chain can be the object node, which has no property.
		TSharedPtr<IPropertyHandle> Root = Handle;
		for (TSharedPtr<IPropertyHandle> Parent = Root->GetParentHandle(); Parent.IsValid() && Parent->GetProperty(); Parent = Parent->GetParentHandle())
		{
			Root = Parent;
		}

		const FProperty* RootProperty = Root->GetProperty();
		const UClass* OwnerClass = RootProperty ? RootProperty->GetOwnerClass() : nullptr;
		if (!OwnerClass)
		{
			return false;
		}

		TArray<UObject*> Outers;
		Handle->GetOuterObjects(Outers);
		for (const UObject* Outer : Outers)
		{
			const UObject* Archetype = Outer ? Outer->GetArchetype() : nullptr;
			if (!Archetype || !Archetype->IsA(OwnerClass))
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Engine reset where the engine has a default; inside array elements missing from the defaults, the value of a
	 * default-constructed TOwner instead.
	 */
	template<typename TOwner>
	FResetToDefaultOverride MakeMemberResetOverride()
	{
		auto GetTypeDefault = [](const FProperty* Property) -> const void*
		{
			static const TOwner Default;
			return Property->ContainerPtrToValuePtr<void>(&Default);
		};

		auto DiffersFromTypeDefault = [GetTypeDefault](const TSharedPtr<IPropertyHandle>& Handle)
		{
			const FProperty* Property = Handle->GetProperty();
			TArray<void*> RawData;
			Handle->AccessRawData(RawData);
			for (const void* Raw : RawData)
			{
				if (Raw && !Property->Identical(Raw, GetTypeDefault(Property)))
				{
					return true;
				}
			}
			return false;
		};

		return FResetToDefaultOverride::Create(
			FIsResetToDefaultVisible::CreateLambda([DiffersFromTypeDefault](TSharedPtr<IPropertyHandle> Handle)
			{
				if (!Handle.IsValid() || !Handle->IsValidHandle() || !Handle->GetProperty())
				{
					return false;
				}
				return Handle->DiffersFromDefault()
					|| ((HasNoArchetypeValue(Handle) || IsMissingFromDefaults(Handle)) && DiffersFromTypeDefault(Handle));
			}),
			FResetToDefaultHandler::CreateLambda([GetTypeDefault](TSharedPtr<IPropertyHandle> Handle)
			{
				if (!Handle.IsValid() || !Handle->IsValidHandle() || !Handle->GetProperty())
				{
					return;
				}
				if (Handle->DiffersFromDefault())
				{
					Handle->ResetToDefault();
					return;
				}

				const FProperty* Property = Handle->GetProperty();
				FString Text;
				Property->ExportText_Direct(Text, GetTypeDefault(Property), nullptr, nullptr, PPF_None);
				Handle->SetValueFromFormattedString(Text);
			}));
	}

	/**
	 * Override acting on Handle for a custom row with no property handle of its own; listing the handle on the row instead
	 * makes the details view evaluate the property's EditCondition, which fails outside the property's own row.
	 */
	FResetToDefaultOverride BindResetOverride(const FResetToDefaultOverride& Override, const TSharedPtr<IPropertyHandle>& Handle)
	{
		const FResetToDefaultHandler OnReset = Override.GetPropertyResetToDefaultDelegate();
		return FResetToDefaultOverride::Create(
			TAttribute<bool>::CreateLambda([Override, Handle]() { return Override.IsResetToDefaultVisible(Handle); }),
			FSimpleDelegate::CreateLambda([OnReset, Handle]() { OnReset.ExecuteIfBound(Handle); }));
	}

	void ApplyPreset(TSharedPtr<IPropertyHandle> Handle, FString PresetName)
	{
		FProceduralCurvePresetData Preset;
		if (Handle.IsValid() && Handle->IsValidHandle() && ProceduralCurvePresets::Find(PresetName, Preset))
		{
			ProceduralCurvePresets::Apply(*Handle, Preset);
		}
	}

	void ShowSaveAsPresetDialog(TSharedPtr<IPropertyHandle> Handle)
	{
		const FProceduralCurve* Curve = GetFirstRaw<FProceduralCurve>(Handle);
		if (!Curve)
		{
			return;
		}
		const FProceduralCurve CurveCopy = *Curve;

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("SavePresetTitle", "Save as Preset"))
			.SizingRule(ESizingRule::UserSized)
			.ClientSize(FVector2D(400, 210))
			.SupportsMaximize(false)
			.SupportsMinimize(false);
		const TWeakPtr<SWindow> WeakWindow = Window;

		TSharedPtr<SEditableTextBox> NameBox;
		TSharedPtr<SEditableTextBox> CategoryBox;
		TSharedPtr<SEditableTextBox> DescriptionBox;

		auto CloseWindow = [WeakWindow]()
		{
			if (const TSharedPtr<SWindow> Pinned = WeakWindow.Pin())
			{
				Pinned->RequestDestroyWindow();
			}
			return FReply::Handled();
		};

		Window->SetContent(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 10.0f, 10.0f, 4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("PresetName", "Preset Name:"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 0.0f)
			[
				SAssignNew(NameBox, SEditableTextBox).HintText(LOCTEXT("EnterPresetName", "Enter preset name..."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 8.0f, 10.0f, 4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("PresetCategory", "Category (optional):"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 0.0f)
			[
				SAssignNew(CategoryBox, SEditableTextBox).HintText(LOCTEXT("EnterCategory", "Enter category..."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 8.0f, 10.0f, 4.0f)
			[
				SNew(STextBlock).Text(LOCTEXT("PresetDescription", "Description (optional):"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f, 0.0f)
			[
				SAssignNew(DescriptionBox, SEditableTextBox).HintText(LOCTEXT("EnterDescription", "Enter description..."))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(10.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Save", "Save"))
					.HAlign(HAlign_Center)
					.OnClicked_Lambda([CurveCopy, NameBox, CategoryBox, DescriptionBox, CloseWindow]()
					{
						const FString Name = NameBox->GetText().ToString().TrimStartAndEnd();
						if (Name.IsEmpty())
						{
							return FReply::Handled();
						}

						FProceduralCurvePresetData Existing;
						if (ProceduralCurvePresets::Find(Name, Existing))
						{
							const FText Message = FText::Format(LOCTEXT("PresetExists", "Preset '{0}' already exists. Overwrite?"), FText::FromString(Name));
							if (FMessageDialog::Open(EAppMsgType::YesNo, Message) != EAppReturnType::Yes)
							{
								return FReply::Handled();
							}
						}

						ProceduralCurvePresets::SaveUser(Name, CategoryBox->GetText().ToString().TrimStartAndEnd(), DescriptionBox->GetText().ToString(), CurveCopy);
						return CloseWindow();
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("PresetSaveCancel", "Cancel"))
					.HAlign(HAlign_Center)
					.OnClicked_Lambda([CloseWindow]() { return CloseWindow(); })
				]
			]);

		FSlateApplication::Get().AddModalWindow(Window, nullptr);
	}

	void DeletePreset(FString PresetName)
	{
		const FText Message = FText::Format(LOCTEXT("ConfirmDeletePreset", "Delete preset '{0}'?"), FText::FromString(PresetName));
		if (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes)
		{
			ProceduralCurvePresets::DeleteUser(PresetName);
		}
	}

	/** Presets in their category submenus, uncategorized ones directly in the section. */
	void AddPresetEntries(FMenuBuilder& MenuBuilder, const TArray<FProceduralCurvePresetData>& Presets, TFunction<void(FMenuBuilder&, const FProceduralCurvePresetData&)> AddEntry)
	{
		TArray<FString> Categories;
		for (const FProceduralCurvePresetData& Preset : Presets)
		{
			if (Preset.Category.IsEmpty())
			{
				AddEntry(MenuBuilder, Preset);
			}
			else
			{
				Categories.AddUnique(Preset.Category);
			}
		}

		for (const FString& Category : Categories)
		{
			MenuBuilder.AddSubMenu(
				FText::FromString(Category),
				FText::GetEmpty(),
				FNewMenuDelegate::CreateLambda([Presets, Category, AddEntry](FMenuBuilder& SubMenuBuilder)
				{
					for (const FProceduralCurvePresetData& Preset : Presets)
					{
						if (Preset.Category == Category)
						{
							AddEntry(SubMenuBuilder, Preset);
						}
					}
				}));
		}
	}

	FText MakePresetTooltip(const FProceduralCurvePresetData& Preset)
	{
		const FProceduralCurveTerm* Term = Preset.Curve.Terms.Num() == 1 ? &Preset.Curve.Terms[0] : nullptr;
		const bool bSingleExpression = Term && Term->Waveform == EProceduralWaveform::Expression;
		return FText::FromString(bSingleExpression ? FString::Printf(TEXT("%s\n\n%s"), *Preset.Description, *Term->Expression).TrimStartAndEnd() : Preset.Description);
	}

	TSharedRef<SWidget> GeneratePresetsMenu(TSharedPtr<IPropertyHandle> Handle)
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SaveAsPreset", "Save as Preset..."),
			LOCTEXT("SaveAsPresetTooltip", "Store this curve as a reusable preset in the project settings"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"),
			FUIAction(FExecuteAction::CreateLambda([Handle]() { ShowSaveAsPresetDialog(Handle); })));

		MenuBuilder.BeginSection("BuiltInPresets", LOCTEXT("BuiltInPresets", "Built-in"));
		AddPresetEntries(MenuBuilder, ProceduralCurvePresets::GetBuiltIn(), [Handle](FMenuBuilder& Builder, const FProceduralCurvePresetData& Preset)
		{
			const FString PresetName = Preset.PresetName;
			Builder.AddMenuEntry(
				FText::FromString(PresetName),
				MakePresetTooltip(Preset),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Handle, PresetName]() { ApplyPreset(Handle, PresetName); })));
		});
		MenuBuilder.EndSection();

		const TArray<FProceduralCurvePresetData> UserPresets = ProceduralCurvePresets::GetUser();
		if (UserPresets.Num() > 0)
		{
			MenuBuilder.BeginSection("UserPresets", LOCTEXT("UserPresets", "User"));
			AddPresetEntries(MenuBuilder, UserPresets, [Handle](FMenuBuilder& Builder, const FProceduralCurvePresetData& Preset)
			{
				const FString PresetName = Preset.PresetName;
				Builder.AddSubMenu(
					FText::FromString(PresetName),
					MakePresetTooltip(Preset),
					FNewMenuDelegate::CreateLambda([Handle, PresetName](FMenuBuilder& SubMenuBuilder)
					{
						SubMenuBuilder.AddMenuEntry(
							LOCTEXT("ApplyPreset", "Apply"),
							LOCTEXT("ApplyPresetTooltip", "Replace the curve with this preset"),
							FSlateIcon(),
							FUIAction(FExecuteAction::CreateLambda([Handle, PresetName]() { ApplyPreset(Handle, PresetName); })));
						SubMenuBuilder.AddMenuEntry(
							LOCTEXT("DeletePreset", "Delete"),
							LOCTEXT("DeletePresetTooltip", "Remove this preset from the project settings"),
							FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
							FUIAction(FExecuteAction::CreateLambda([PresetName]() { DeletePreset(PresetName); })));
					}),
					false,
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Star"));
			});
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
	}

	TSharedRef<SWidget> MakeStepButton(TSharedPtr<IPropertyHandle> Handle, int32 Direction)
	{
		return SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(FMargin(1.0f, 0.0f))
			.ToolTipText(Direction < 0
				? LOCTEXT("PreviousPresetTooltip", "Apply the previous preset")
				: LOCTEXT("NextPresetTooltip", "Apply the next preset"))
			.OnClicked_Lambda([Handle, Direction]()
			{
				if (Handle.IsValid() && Handle->IsValidHandle())
				{
					ProceduralCurvePresets::Step(*Handle, Direction);
				}
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(Direction < 0 ? "Icons.ChevronLeft" : "Icons.ChevronRight"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			];
	}

	TSharedRef<SWidget> MakePresetsButton(TSharedPtr<IPropertyHandle> Handle)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				MakeStepButton(Handle, -1)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SComboButton)
				.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
				.HasDownArrow(false)
				.ToolTipText(LOCTEXT("PresetsTooltip", "Apply a built-in or user preset, or save this curve as one"))
				.OnGetMenuContent_Lambda([Handle]() { return GeneratePresetsMenu(Handle); })
				.ButtonContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.Star"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SBox)
						.MaxDesiredWidth(140.0f)
						[
							SNew(STextBlock)
							.Font(IDetailLayoutBuilder::GetDetailFont())
							.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
							.Text_Lambda([Handle]()
							{
								const FProceduralCurve* Curve = GetFirstRaw<FProceduralCurve>(Handle);
								const FString Name = Curve ? ProceduralCurvePresets::FindMatchingName(*Curve) : FString();
								return Name.IsEmpty() ? LOCTEXT("CustomCurve", "Custom") : FText::FromString(Name);
							})
							.ColorAndOpacity_Lambda([Handle]()
							{
								const FProceduralCurve* Curve = GetFirstRaw<FProceduralCurve>(Handle);
								const bool bPreset = Curve && !ProceduralCurvePresets::FindMatchingName(*Curve).IsEmpty();
								return bPreset ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
							})
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				MakeStepButton(Handle, 1)
			];
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
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				MakePlot(MakeCurveSeries(PropertyHandle), 32.0f, 1.0f, false)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				MakePresetsButton(PropertyHandle)
			]
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
			ChildBuilder.AddProperty(Child.ToSharedRef()).OverrideResetToDefault(MakeMemberResetOverride<FProceduralCurve>());
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
		ChildBuilder.AddProperty(Child.ToSharedRef()).OverrideResetToDefault(MakeMemberResetOverride<FProceduralCurveTerm>());
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
		.OverrideResetToDefault(BindResetOverride(MakeMemberResetOverride<FProceduralCurveTerm>(), ExpressionHandle))
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
