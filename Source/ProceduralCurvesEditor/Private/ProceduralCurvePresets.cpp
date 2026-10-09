// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralCurvePresets.h"

#include "PropertyHandle.h"

namespace ProceduralCurvePresets
{
	namespace Private
	{
		FProceduralCurvePresetData MakePreset(const TCHAR* Category, const TCHAR* Name, const TCHAR* Expression, const TCHAR* Description)
		{
			FProceduralCurvePresetData Preset;
			Preset.PresetName = Name;
			Preset.Category = Category;
			Preset.Description = Description;
			Preset.Curve.Add(EProceduralWaveform::Expression, 1.0f, 1.0f, 0.0f);
			Preset.Curve.Terms.Last().Expression = Expression;
			return Preset;
		}

		FString ExportCurve(const FProceduralCurve& Curve)
		{
			FString Text;
			FProceduralCurve::StaticStruct()->ExportText(Text, &Curve, nullptr, nullptr, PPF_None, nullptr);
			return Text;
		}

		int32 FindUserIndex(const UProceduralCurvesSettings& Settings, const FString& Name)
		{
			return Settings.UserPresets.IndexOfByPredicate([&Name](const FProceduralCurvePresetData& Preset)
			{
				return Preset.PresetName == Name;
			});
		}
	}

	const TArray<FProceduralCurvePresetData>& GetBuiltIn()
	{
		using Private::MakePreset;

		static const TArray<FProceduralCurvePresetData> BuiltIn = {
			MakePreset(TEXT("Steps"), TEXT("Weighted Bob"), TEXT("sine(2 * x) + 0.3 * sine(4 * x + 0.25)"), TEXT("Bob with weight on every step")),
			MakePreset(TEXT("Steps"), TEXT("Heel-Toe Roll"), TEXT("sine(2 * x) + 0.25 * sine(6 * x)"), TEXT("Two bumps per step")),
			MakePreset(TEXT("Steps"), TEXT("Dip At Contact"), TEXT("-(pow(sin(pi * frac(2 * x)), 0.6))"), TEXT("Dip at each foot contact, flat mid-step")),
			MakePreset(TEXT("Steps"), TEXT("Impact"), TEXT("-12 * frac(2 * x) * exp(1 - 12 * frac(2 * x))"), TEXT("Sharp drop right after each contact, then recovery")),
			MakePreset(TEXT("Steps"), TEXT("Spring After Step"), TEXT("exp(-6 * frac(2 * x)) * sin(tau * 3 * frac(2 * x))"), TEXT("Decaying wobble after every step")),
			MakePreset(TEXT("Steps"), TEXT("Contact Pulses"), TEXT("exp(-200 * min(p, 1 - p)^2) + 0.7 * exp(-200 * (p - 0.5)^2)"), TEXT("Pulse at each contact, the left foot stronger")),
			MakePreset(TEXT("Steps"), TEXT("Limp"), TEXT("sine(2 * x) * (p < 0.5 ? 1 : 0.6)"), TEXT("One foot's step weaker than the other")),
			MakePreset(TEXT("Steps"), TEXT("Random Step Strength"), TEXT("sine(2 * x) * lerp(0.7, 1.3, hash(floor(2 * x), 5))"), TEXT("Every step a little stronger or weaker; continuous since the sine is zero at step boundaries")),
			MakePreset(TEXT("Steps"), TEXT("Bouncy Run"), TEXT("bounce(2 * x)"), TEXT("Bouncing run")),
			MakePreset(TEXT("Steps"), TEXT("Sharp Dips"), TEXT("-abs(sine(x))"), TEXT("Sharp dips at contacts, soft top")),
			MakePreset(TEXT("Steps"), TEXT("Side Sway With Hang"), TEXT("sine(x + 0.15 * sine(2 * x))"), TEXT("Side sway that hangs over the planted foot")),

			MakePreset(TEXT("Breathing"), TEXT("Even Breath"), TEXT("pow(sin(pi * p), 2)"), TEXT("Even inhale and exhale")),
			MakePreset(TEXT("Breathing"), TEXT("Quick Inhale"), TEXT("pow(sin(pi * pow(p, 0.6)), 2)"), TEXT("Quick inhale, slow exhale")),
			MakePreset(TEXT("Breathing"), TEXT("Breath Hold"), TEXT("smootherstep(0, 0.35, p) - smootherstep(0.55, 1, p)"), TEXT("Inhale, hold at the top, exhale")),
			MakePreset(TEXT("Breathing"), TEXT("Harmonic Breath"), TEXT("sine(x) + 0.3 * sine(2 * x + 0.28) + 0.1 * sine(3 * x + 0.05)"), TEXT("Breathing with harmonics")),
			MakePreset(TEXT("Breathing"), TEXT("Uneven Breath"), TEXT("sine(x) * (0.8 + 0.2 * sine(3 * x))"), TEXT("Breathing with an uneven rhythm")),
			MakePreset(TEXT("Breathing"), TEXT("Heartbeat"), TEXT("exp(-200 * (p - 0.1)^2) + 0.6 * exp(-200 * (p - 0.25)^2)"), TEXT("Double heartbeat pulse")),

			MakePreset(TEXT("Noise"), TEXT("Slow Drift"), TEXT("pnoise(x * 4, 4)"), TEXT("Slow random drift, repeats once per cycle")),
			MakePreset(TEXT("Noise"), TEXT("Organic, 3 Octaves"), TEXT("pnoise(x * 4, 4) * 0.6 + pnoise(x * 8, 8, 1) * 0.3 + pnoise(x * 16, 16, 2) * 0.1"), TEXT("Three noise octaves, repeats once per cycle")),
			MakePreset(TEXT("Noise"), TEXT("Tremor"), TEXT("pnoise(x * 24, 24, 3)"), TEXT("Fine hand tremor; use a small amplitude")),
			MakePreset(TEXT("Noise"), TEXT("Living Sine"), TEXT("sine(x) + 0.2 * pnoise(x * 8, 8, 7)"), TEXT("Sine with a slight random wobble")),
			MakePreset(TEXT("Noise"), TEXT("Shiver Burst"), TEXT("pnoise(x * 32, 32) * smoothstep(0.2, 0.3, p) * (1 - smoothstep(0.4, 0.5, p))"), TEXT("Short burst of shivering mid-cycle")),
			MakePreset(TEXT("Noise"), TEXT("Glitch"), TEXT("sine(x) + (hash(floor(x * 16), 7) > 0.85 ? (hash(floor(x * 16), 3) - 0.5) * 3 : 0)"), TEXT("Sine with rare random jumps; discontinuous")),

			MakePreset(TEXT("Modulation"), TEXT("Tremolo"), TEXT("sine(4 * x) * (0.6 + 0.4 * sine(x))"), TEXT("Fast wave under a slow envelope")),
			MakePreset(TEXT("Modulation"), TEXT("FM Wobble"), TEXT("sine(x + 0.3 * sine(3 * x))"), TEXT("The wave drifts back and forth in time")),
			MakePreset(TEXT("Modulation"), TEXT("Beats"), TEXT("0.5 * (sine(5 * x) + sine(6 * x))"), TEXT("Amplitude swells and fades once per cycle")),
			MakePreset(TEXT("Modulation"), TEXT("Triple In A Hump"), TEXT("sine(3 * x) * pow(sin(pi * p), 2)"), TEXT("Three wobbles inside one hump")),
			MakePreset(TEXT("Modulation"), TEXT("Sine To Triangle"), TEXT("lerp(sine(x), tri(x), 0.5 + 0.5 * sine(x))"), TEXT("Shape morphs between sine and triangle")),

			MakePreset(TEXT("Modulation"), TEXT("Nested FM"), TEXT("sine(x + 0.5 * sine(2 * x + 0.5 * sine(3 * x)))"), TEXT("Irregular swing that still repeats every cycle")),

			MakePreset(TEXT("Physics"), TEXT("Spring"), TEXT("exp(-5 * p) * sin(tau * 4 * p)"), TEXT("Hit at the start of the cycle, then a decaying spring")),
			MakePreset(TEXT("Physics"), TEXT("Jelly"), TEXT("exp(-6 * p) * sin(tau * 5 * p) + 0.3 * exp(-9 * p) * sin(tau * 9 * p)"), TEXT("Two springs of different stiffness decaying together")),
			MakePreset(TEXT("Physics"), TEXT("Bouncing Settle"), TEXT("-abs(sin(tau * 3 * p)) * exp(-4 * p)"), TEXT("Drops and bounces off a floor with ever smaller hops")),
			MakePreset(TEXT("Physics"), TEXT("Elastic Return"), TEXT("(1 - exp(-8 * p) * cos(tau * 3 * p)) * (1 - smoothstep(0.75, 1, p))"), TEXT("Springs to 1 with an overshoot, holds, eases back to 0 for the loop")),
			MakePreset(TEXT("Physics"), TEXT("Two-Step Spring"), TEXT("exp(-7 * frac(2 * x)) * sin(tau * 3 * frac(2 * x)) * (p < 0.5 ? 1 : -0.7)"), TEXT("Spring after each step, opposite direction and weaker on the second")),
			MakePreset(TEXT("Physics"), TEXT("Pendulum Kick"), TEXT("sin(tau * 2 * p) * exp(-2.5 * p) * (1 - p)"), TEXT("Pushed pendulum swinging out and dying down by the end of the cycle")),

			MakePreset(TEXT("Accents"), TEXT("Smile"), TEXT("-abs(sine(x))"), TEXT("Smile-shaped swing with sine(x) on the other axis")),
			MakePreset(TEXT("Accents"), TEXT("Kick And Return"), TEXT("10 * p * exp(1 - 10 * p)"), TEXT("Push at the start of the cycle, smooth return")),
			MakePreset(TEXT("Accents"), TEXT("Overshoot"), TEXT("sin(tau * p) * exp(-3 * p)"), TEXT("Overshoot that dies out within the cycle")),
			MakePreset(TEXT("Accents"), TEXT("Plateau"), TEXT("smoothstep(0.4, 0.5, p) - smoothstep(0.9, 1, p)"), TEXT("Smooth plateau in the second half of the cycle")),
			MakePreset(TEXT("Accents"), TEXT("Single Hump"), TEXT("exp(-80 * (p - 0.5)^2)"), TEXT("Gaussian hump mid-cycle")),
			MakePreset(TEXT("Accents"), TEXT("Hard Switch"), TEXT("step(0.5, p) * 2 - 1"), TEXT("-1 for the first half, +1 for the second; discontinuous")),

			MakePreset(TEXT("One-Shot"), TEXT("Ease To 1"), TEXT("smoothstep(0, 0.3, t)"), TEXT("Rises to 1 and holds; does not loop")),
			MakePreset(TEXT("One-Shot"), TEXT("Elastic"), TEXT("1 - exp(-6 * t) * cos(tau * 2 * t)"), TEXT("Settles on 1 with a spring; does not loop")),
			MakePreset(TEXT("One-Shot"), TEXT("Back Ease"), TEXT("t * t * (2.7 * t - 1.7)"), TEXT("Pulls back a little before going forward; does not loop")),
			MakePreset(TEXT("One-Shot"), TEXT("Bouncing Ball"), TEXT("abs(sine(1.5 * t)) * exp(-2 * t)"), TEXT("Decaying bounces; does not loop")),
			MakePreset(TEXT("One-Shot"), TEXT("Decaying Vibration"), TEXT("exp(-4 * t) * sine(3 * t)"), TEXT("Vibration that dies out, like recoil; does not loop")),
			MakePreset(TEXT("One-Shot"), TEXT("Ease-Out Cubic"), TEXT("1 - pow(1 - saturate(t), 3)"), TEXT("Cubic ease-out from 0 to 1; does not loop")),
		};
		return BuiltIn;
	}

	FString FindMatchingName(const FProceduralCurve& Curve)
	{
		// Exported once: the header asks every frame for every visible curve.
		static const TArray<FString> BuiltInTexts = []()
		{
			TArray<FString> Texts;
			for (const FProceduralCurvePresetData& Preset : GetBuiltIn())
			{
				Texts.Add(Private::ExportCurve(Preset.Curve));
			}
			return Texts;
		}();

		const FString CurveText = Private::ExportCurve(Curve);
		for (const FProceduralCurvePresetData& Preset : GetDefault<UProceduralCurvesSettings>()->UserPresets)
		{
			if (Private::ExportCurve(Preset.Curve) == CurveText)
			{
				return Preset.PresetName;
			}
		}

		const int32 Index = BuiltInTexts.IndexOfByKey(CurveText);
		return Index != INDEX_NONE ? GetBuiltIn()[Index].PresetName : FString();
	}

	TArray<FProceduralCurvePresetData> GetUser()
	{
		return GetDefault<UProceduralCurvesSettings>()->UserPresets;
	}

	bool Find(const FString& Name, FProceduralCurvePresetData& OutPreset)
	{
		const UProceduralCurvesSettings& Settings = *GetDefault<UProceduralCurvesSettings>();
		const int32 UserIndex = Private::FindUserIndex(Settings, Name);
		if (UserIndex != INDEX_NONE)
		{
			OutPreset = Settings.UserPresets[UserIndex];
			return true;
		}

		for (const FProceduralCurvePresetData& Preset : GetBuiltIn())
		{
			if (Preset.PresetName == Name)
			{
				OutPreset = Preset;
				return true;
			}
		}

		return false;
	}

	void Apply(IPropertyHandle& CurveHandle, const FProceduralCurvePresetData& Preset)
	{
		CurveHandle.SetValueFromFormattedString(Private::ExportCurve(Preset.Curve));
	}

	void Step(IPropertyHandle& CurveHandle, int32 Direction)
	{
		TArray<FProceduralCurvePresetData> Presets = GetBuiltIn();
		Presets.Append(GetUser());
		if (Presets.IsEmpty() || Direction == 0)
		{
			return;
		}

		int32 Current = INDEX_NONE;
		TArray<const void*> RawData;
		CurveHandle.AccessRawData(RawData);
		if (RawData.Num() > 0 && RawData[0])
		{
			const FString CurrentText = Private::ExportCurve(*static_cast<const FProceduralCurve*>(RawData[0]));
			Current = Presets.IndexOfByPredicate([&CurrentText](const FProceduralCurvePresetData& Preset)
			{
				return Private::ExportCurve(Preset.Curve) == CurrentText;
			});
		}

		const int32 Next = Current == INDEX_NONE
			? (Direction > 0 ? 0 : Presets.Num() - 1)
			: (Current + Direction % Presets.Num() + Presets.Num()) % Presets.Num();
		Apply(CurveHandle, Presets[Next]);
	}

	void SaveUser(const FString& Name, const FString& Category, const FString& Description, const FProceduralCurve& Curve)
	{
		if (Name.IsEmpty())
		{
			return;
		}

		FProceduralCurvePresetData Preset;
		Preset.PresetName = Name;
		Preset.Category = Category;
		Preset.Description = Description;
		Preset.Curve = Curve;

		UProceduralCurvesSettings* Settings = GetMutableDefault<UProceduralCurvesSettings>();
		const int32 Existing = Private::FindUserIndex(*Settings, Name);
		if (Existing != INDEX_NONE)
		{
			Settings->UserPresets[Existing] = MoveTemp(Preset);
		}
		else
		{
			Settings->UserPresets.Add(MoveTemp(Preset));
		}

		Settings->TryUpdateDefaultConfigFile();
	}

	bool DeleteUser(const FString& Name)
	{
		UProceduralCurvesSettings* Settings = GetMutableDefault<UProceduralCurvesSettings>();
		const int32 Index = Private::FindUserIndex(*Settings, Name);
		if (Index == INDEX_NONE)
		{
			return false;
		}

		Settings->UserPresets.RemoveAt(Index);
		Settings->TryUpdateDefaultConfigFile();
		return true;
	}
}
