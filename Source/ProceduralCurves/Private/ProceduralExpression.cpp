// Copyright PsinaDev. All Rights Reserved.

#include "ProceduralExpression.h"
#include "Containers/Map.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/CString.h"
#include "Misc/ScopeRWLock.h"
#include "ProceduralCurvesCompat.h"
#include "ProceduralWaves.h"

DEFINE_LOG_CATEGORY_STATIC(LogProceduralExpression, Log, All);

#define LOCTEXT_NAMESPACE "ProceduralExpression"

namespace ProceduralExpression
{
	namespace
	{
		constexpr int32 MaxStackDepth = 64;

		enum class EOp : uint8
		{
			Const, T, X, P,
			Add, Sub, Mul, Div, Mod, Pow, Neg, Not,
			Less, LessEqual, Greater, GreaterEqual, Equal, NotEqual, And, Or, Select,
			Sin, Cos, Tan, Asin, Acos, Atan, Atan2,
			Abs, Sign, Floor, Ceil, Round, Frac, Sqrt, Exp, Log, Log2,
			Min, Max, Clamp, Saturate, Lerp, Step, SmoothStep, SmootherStep, Remap,
			Sine, Tri, Square, Saw, Bounce, Noise, PNoise, SNoise, Hash
		};

		struct FInstruction
		{
			EOp Op;
			float Value;
		};

		struct FFunctionDef
		{
			FFunctionInfo Info;
			EOp Op;
			int32 MinArgs;
			int32 MaxArgs;
			float Defaults[2];
		};

		const FFunctionDef GFunctions[] =
		{
			{ { TEXT("sin"), TEXT("sin(a)"), TEXT("Sine of a in radians.") }, EOp::Sin, 1, 1, {} },
			{ { TEXT("cos"), TEXT("cos(a)"), TEXT("Cosine of a in radians.") }, EOp::Cos, 1, 1, {} },
			{ { TEXT("tan"), TEXT("tan(a)"), TEXT("Tangent of a in radians.") }, EOp::Tan, 1, 1, {} },
			{ { TEXT("asin"), TEXT("asin(v)"), TEXT("Arcsine in radians; v clamped to [-1, 1].") }, EOp::Asin, 1, 1, {} },
			{ { TEXT("acos"), TEXT("acos(v)"), TEXT("Arccosine in radians; v clamped to [-1, 1].") }, EOp::Acos, 1, 1, {} },
			{ { TEXT("atan"), TEXT("atan(v)"), TEXT("Arctangent in radians.") }, EOp::Atan, 1, 1, {} },
			{ { TEXT("atan2"), TEXT("atan2(y, x)"), TEXT("Angle of (x, y) in radians.") }, EOp::Atan2, 2, 2, {} },
			{ { TEXT("abs"), TEXT("abs(v)"), TEXT("Absolute value.") }, EOp::Abs, 1, 1, {} },
			{ { TEXT("sign"), TEXT("sign(v)"), TEXT("-1, 0 or 1.") }, EOp::Sign, 1, 1, {} },
			{ { TEXT("floor"), TEXT("floor(v)"), TEXT("Largest whole number not above v.") }, EOp::Floor, 1, 1, {} },
			{ { TEXT("ceil"), TEXT("ceil(v)"), TEXT("Smallest whole number not below v.") }, EOp::Ceil, 1, 1, {} },
			{ { TEXT("round"), TEXT("round(v)"), TEXT("Nearest whole number.") }, EOp::Round, 1, 1, {} },
			{ { TEXT("frac"), TEXT("frac(v)"), TEXT("v - floor(v), in [0, 1).") }, EOp::Frac, 1, 1, {} },
			{ { TEXT("sqrt"), TEXT("sqrt(v)"), TEXT("Square root; 0 for negative v.") }, EOp::Sqrt, 1, 1, {} },
			{ { TEXT("exp"), TEXT("exp(v)"), TEXT("e to the power v.") }, EOp::Exp, 1, 1, {} },
			{ { TEXT("log"), TEXT("log(v)"), TEXT("Natural logarithm; 0 for v <= 0.") }, EOp::Log, 1, 1, {} },
			{ { TEXT("log2"), TEXT("log2(v)"), TEXT("Base-2 logarithm; 0 for v <= 0.") }, EOp::Log2, 1, 1, {} },
			{ { TEXT("pow"), TEXT("pow(a, b)"), TEXT("a to the power b; same as a ^ b.") }, EOp::Pow, 2, 2, {} },
			{ { TEXT("min"), TEXT("min(a, b)"), TEXT("Smaller of a and b.") }, EOp::Min, 2, 2, {} },
			{ { TEXT("max"), TEXT("max(a, b)"), TEXT("Larger of a and b.") }, EOp::Max, 2, 2, {} },
			{ { TEXT("mod"), TEXT("mod(a, b)"), TEXT("Floored modulo, same sign as b; same as a % b.") }, EOp::Mod, 2, 2, {} },
			{ { TEXT("clamp"), TEXT("clamp(v, lo, hi)"), TEXT("v limited to [lo, hi].") }, EOp::Clamp, 3, 3, {} },
			{ { TEXT("saturate"), TEXT("saturate(v)"), TEXT("v limited to [0, 1].") }, EOp::Saturate, 1, 1, {} },
			{ { TEXT("lerp"), TEXT("lerp(a, b, alpha)"), TEXT("a + (b - a) * alpha.") }, EOp::Lerp, 3, 3, {} },
			{ { TEXT("step"), TEXT("step(edge, v)"), TEXT("1 when v >= edge, else 0.") }, EOp::Step, 2, 2, {} },
			{ { TEXT("smoothstep"), TEXT("smoothstep(lo, hi, v)"), TEXT("Smooth 0..1 ramp of v between lo and hi.") }, EOp::SmoothStep, 3, 3, {} },
			{ { TEXT("smootherstep"), TEXT("smootherstep(lo, hi, v)"), TEXT("Smoothstep with zero second derivative at the ends.") }, EOp::SmootherStep, 3, 3, {} },
			{ { TEXT("remap"), TEXT("remap(v, inLo, inHi, outLo, outHi)"), TEXT("Linear map of v from one range to another; not clamped.") }, EOp::Remap, 5, 5, {} },
			{ { TEXT("select"), TEXT("select(c, a, b)"), TEXT("a when c is non-zero, else b; same as c ? a : b.") }, EOp::Select, 3, 3, {} },
			{ { TEXT("sine"), TEXT("sine(cycles)"), TEXT("Sine wave with a period of 1.") }, EOp::Sine, 1, 1, {} },
			{ { TEXT("tri"), TEXT("tri(cycles, skew = 0.5)"), TEXT("Triangle wave peaking at skew / 2; skew 1 is a sawtooth.") }, EOp::Tri, 1, 2, { 0.5f } },
			{ { TEXT("square"), TEXT("square(cycles, duty = 0.5, smooth = 0)"), TEXT("+1 for duty of each cycle, -1 for the rest; smooth rounds the jumps.") }, EOp::Square, 1, 3, { 0.5f, 0.0f } },
			{ { TEXT("saw"), TEXT("saw(cycles, smooth = 0)"), TEXT("Rising sawtooth; smooth rounds the drop.") }, EOp::Saw, 1, 2, { 0.0f } },
			{ { TEXT("bounce"), TEXT("bounce(cycles)"), TEXT("Rectified sine touching -1 at whole cycles.") }, EOp::Bounce, 1, 1, {} },
			{ { TEXT("noise"), TEXT("noise(v, seed = 0)"), TEXT("Smooth random value, knots at whole v; never repeats.") }, EOp::Noise, 1, 2, { 0.0f } },
			{ { TEXT("pnoise"), TEXT("pnoise(v, period, seed = 0)"), TEXT("Smooth random value repeating every period knots.") }, EOp::PNoise, 2, 3, { 0.0f } },
			{ { TEXT("snoise"), TEXT("snoise(v, seed = 0, smooth = 0)"), TEXT("Random value held between whole v; smooth rounds the steps.") }, EOp::SNoise, 1, 3, { 0.0f, 0.0f } },
			{ { TEXT("hash"), TEXT("hash(v, seed = 0)"), TEXT("Random value in [0, 1) per whole v.") }, EOp::Hash, 1, 2, { 0.0f } },
		};

		const FFunctionDef* FindFunction(const FString& Name)
		{
			for (const FFunctionDef& Def : GFunctions)
			{
				// Case-insensitive like the cache key and FString ==, so "Sin" and "sin" compile alike.
				if (Name.Equals(Def.Info.Name, ESearchCase::IgnoreCase))
				{
					return &Def;
				}
			}
			return nullptr;
		}

		int32 RoundToSeed(float Value)
		{
			return FMath::RoundToInt32(Value);
		}

		class FCompiler
		{
		public:
			FCompiler(const FString& InSource, TArray<FInstruction>& InCode)
				: Source(InSource), Code(InCode)
			{
			}

			bool Compile(FText& OutError)
			{
				Advance();
				if (Kind == EToken::End)
				{
					return Fail(LOCTEXT("Empty", "Expression is empty."), OutError);
				}
				if (ParseTernary() && Kind != EToken::End)
				{
					SetError(FText::Format(LOCTEXT("Trailing", "Unexpected '{0}'."), FText::FromString(TokenText)));
				}
				if (!Error.IsEmpty())
				{
					OutError = FText::Format(LOCTEXT("ErrorAt", "{0} (at {1})"), Error, FText::AsNumber(ErrorPosition + 1));
					return false;
				}
				if (MaxDepth > MaxStackDepth)
				{
					return Fail(LOCTEXT("TooDeep", "Expression nests too deeply."), OutError);
				}
				return true;
			}

		private:
			enum class EToken : uint8 { End, Number, Ident, Symbol };

			const FString& Source;
			TArray<FInstruction>& Code;
			int32 Position = 0;
			EToken Kind = EToken::End;
			FString TokenText;
			float TokenNumber = 0.0f;
			int32 TokenStart = 0;
			int32 Depth = 0;
			int32 MaxDepth = 0;
			FText Error;
			int32 ErrorPosition = 0;

			bool Fail(const FText& Message, FText& OutError)
			{
				OutError = Message;
				return false;
			}

			bool SetError(const FText& Message)
			{
				if (Error.IsEmpty())
				{
					Error = Message;
					ErrorPosition = TokenStart;
				}
				return false;
			}

			void Emit(EOp Op, int32 StackDelta, float Value = 0.0f)
			{
				Code.Add({ Op, Value });
				Depth += StackDelta;
				MaxDepth = FMath::Max(MaxDepth, Depth);
			}

			bool IsSymbol(const TCHAR* Symbol) const
			{
				return Kind == EToken::Symbol && TokenText == Symbol;
			}

			void Advance()
			{
				while (Position < Source.Len() && FChar::IsWhitespace(Source[Position]))
				{
					++Position;
				}

				TokenStart = Position;
				TokenText.Reset();

				if (Position >= Source.Len())
				{
					Kind = EToken::End;
					return;
				}

				const TCHAR C = Source[Position];
				if (FChar::IsDigit(C) || (C == TEXT('.') && Position + 1 < Source.Len() && FChar::IsDigit(Source[Position + 1])))
				{
					while (Position < Source.Len() && (FChar::IsDigit(Source[Position]) || Source[Position] == TEXT('.')))
					{
						++Position;
					}
					if (Position < Source.Len() && (Source[Position] == TEXT('e') || Source[Position] == TEXT('E')))
					{
						int32 Exponent = Position + 1;
						if (Exponent < Source.Len() && (Source[Exponent] == TEXT('+') || Source[Exponent] == TEXT('-')))
						{
							++Exponent;
						}
						if (Exponent < Source.Len() && FChar::IsDigit(Source[Exponent]))
						{
							Position = Exponent;
							while (Position < Source.Len() && FChar::IsDigit(Source[Position]))
							{
								++Position;
							}
						}
					}
					TokenText = Source.Mid(TokenStart, Position - TokenStart);
					TokenNumber = FCString::Atof(*TokenText);
					Kind = EToken::Number;
					return;
				}

				if (FChar::IsAlpha(C) || C == TEXT('_'))
				{
					while (Position < Source.Len() && (FChar::IsAlnum(Source[Position]) || Source[Position] == TEXT('_')))
					{
						++Position;
					}
					TokenText = Source.Mid(TokenStart, Position - TokenStart);
					Kind = EToken::Ident;
					return;
				}

				static const TCHAR* TwoCharSymbols[] = { TEXT("<="), TEXT(">="), TEXT("=="), TEXT("!="), TEXT("&&"), TEXT("||") };
				for (const TCHAR* Symbol : TwoCharSymbols)
				{
					if (Position + 1 < Source.Len() && Source[Position] == Symbol[0] && Source[Position + 1] == Symbol[1])
					{
						TokenText = Symbol;
						Position += 2;
						Kind = EToken::Symbol;
						return;
					}
				}

				TokenText = FString::Chr(C);
				++Position;
				Kind = EToken::Symbol;
			}

			bool Expect(const TCHAR* Symbol)
			{
				if (!IsSymbol(Symbol))
				{
					return SetError(FText::Format(LOCTEXT("Expected", "Expected '{0}'."), FText::FromString(Symbol)));
				}
				Advance();
				return true;
			}

			bool ParseTernary()
			{
				if (!ParseOr())
				{
					return false;
				}
				if (IsSymbol(TEXT("?")))
				{
					Advance();
					if (!ParseTernary() || !Expect(TEXT(":")) || !ParseTernary())
					{
						return false;
					}
					Emit(EOp::Select, -2);
				}
				return true;
			}

			bool ParseOr()
			{
				if (!ParseAnd())
				{
					return false;
				}
				while (IsSymbol(TEXT("||")))
				{
					Advance();
					if (!ParseAnd())
					{
						return false;
					}
					Emit(EOp::Or, -1);
				}
				return true;
			}

			bool ParseAnd()
			{
				if (!ParseCompare())
				{
					return false;
				}
				while (IsSymbol(TEXT("&&")))
				{
					Advance();
					if (!ParseCompare())
					{
						return false;
					}
					Emit(EOp::And, -1);
				}
				return true;
			}

			bool ParseCompare()
			{
				if (!ParseAdd())
				{
					return false;
				}
				for (;;)
				{
					EOp Op;
					if (IsSymbol(TEXT("<"))) { Op = EOp::Less; }
					else if (IsSymbol(TEXT("<="))) { Op = EOp::LessEqual; }
					else if (IsSymbol(TEXT(">"))) { Op = EOp::Greater; }
					else if (IsSymbol(TEXT(">="))) { Op = EOp::GreaterEqual; }
					else if (IsSymbol(TEXT("=="))) { Op = EOp::Equal; }
					else if (IsSymbol(TEXT("!="))) { Op = EOp::NotEqual; }
					else { return true; }

					Advance();
					if (!ParseAdd())
					{
						return false;
					}
					Emit(Op, -1);
				}
			}

			bool ParseAdd()
			{
				if (!ParseMul())
				{
					return false;
				}
				while (IsSymbol(TEXT("+")) || IsSymbol(TEXT("-")))
				{
					const EOp Op = IsSymbol(TEXT("+")) ? EOp::Add : EOp::Sub;
					Advance();
					if (!ParseMul())
					{
						return false;
					}
					Emit(Op, -1);
				}
				return true;
			}

			bool ParseMul()
			{
				if (!ParseUnary())
				{
					return false;
				}
				while (IsSymbol(TEXT("*")) || IsSymbol(TEXT("/")) || IsSymbol(TEXT("%")))
				{
					const EOp Op = IsSymbol(TEXT("*")) ? EOp::Mul : IsSymbol(TEXT("/")) ? EOp::Div : EOp::Mod;
					Advance();
					if (!ParseUnary())
					{
						return false;
					}
					Emit(Op, -1);
				}
				return true;
			}

			bool ParseUnary()
			{
				if (IsSymbol(TEXT("-")) || IsSymbol(TEXT("!")))
				{
					const EOp Op = IsSymbol(TEXT("-")) ? EOp::Neg : EOp::Not;
					Advance();
					if (!ParseUnary())
					{
						return false;
					}
					Emit(Op, 0);
					return true;
				}
				if (IsSymbol(TEXT("+")))
				{
					Advance();
					return ParseUnary();
				}
				return ParsePower();
			}

			bool ParsePower()
			{
				if (!ParsePrimary())
				{
					return false;
				}
				if (IsSymbol(TEXT("^")))
				{
					Advance();
					// Right operand through ParseUnary: 2 ^ -1 and 2 ^ 3 ^ 2 = 2 ^ (3 ^ 2).
					if (!ParseUnary())
					{
						return false;
					}
					Emit(EOp::Pow, -1);
				}
				return true;
			}

			bool ParsePrimary()
			{
				if (Kind == EToken::Number)
				{
					Emit(EOp::Const, 1, TokenNumber);
					Advance();
					return true;
				}

				if (IsSymbol(TEXT("(")))
				{
					Advance();
					return ParseTernary() && Expect(TEXT(")"));
				}

				if (Kind != EToken::Ident)
				{
					return SetError(Kind == EToken::End
						? LOCTEXT("UnexpectedEnd", "Unexpected end of expression.")
						: FText::Format(LOCTEXT("Unexpected", "Unexpected '{0}'."), FText::FromString(TokenText)));
				}

				const FString Name = TokenText;
				Advance();

				if (IsSymbol(TEXT("(")))
				{
					return ParseCall(Name);
				}

				if (Name == TEXT("t")) { Emit(EOp::T, 1); return true; }
				if (Name == TEXT("x")) { Emit(EOp::X, 1); return true; }
				if (Name == TEXT("p")) { Emit(EOp::P, 1); return true; }
				if (Name == TEXT("pi")) { Emit(EOp::Const, 1, UE_PI); return true; }
				if (Name == TEXT("tau")) { Emit(EOp::Const, 1, UE_TWO_PI); return true; }
				if (Name == TEXT("e")) { Emit(EOp::Const, 1, UE_EULERS_NUMBER); return true; }

				return SetError(FText::Format(LOCTEXT("UnknownName", "Unknown name '{0}'."), FText::FromString(Name)));
			}

			bool ParseCall(const FString& Name)
			{
				const FFunctionDef* Def = FindFunction(Name);
				if (!Def)
				{
					return SetError(FText::Format(LOCTEXT("UnknownFunction", "Unknown function '{0}'."), FText::FromString(Name)));
				}

				Advance();
				int32 ArgCount = 0;
				if (!IsSymbol(TEXT(")")))
				{
					for (;;)
					{
						if (!ParseTernary())
						{
							return false;
						}
						++ArgCount;
						if (!IsSymbol(TEXT(",")))
						{
							break;
						}
						Advance();
					}
				}
				if (!Expect(TEXT(")")))
				{
					return false;
				}

				if (ArgCount < Def->MinArgs || ArgCount > Def->MaxArgs)
				{
					return SetError(FText::Format(LOCTEXT("ArgCount", "{0} takes {1}."), FText::FromString(Name), FText::FromString(Def->Info.Signature)));
				}

				for (int32 Arg = ArgCount; Arg < Def->MaxArgs; ++Arg)
				{
					Emit(EOp::Const, 1, Def->Defaults[Arg - Def->MinArgs]);
				}
				Emit(Def->Op, 1 - Def->MaxArgs);
				return true;
			}
		};

		float SafeDivide(float A, float B)
		{
			return B != 0.0f ? A / B : 0.0f;
		}

		float FlooredMod(float A, float B)
		{
			return B != 0.0f ? A - B * FMath::FloorToFloat(A / B) : 0.0f;
		}
	}

	class FProgram
	{
	public:
		FString Source;
		TArray<FInstruction> Code;
		bool bValid = false;
	};

	bool MatchesSource(const FProgram& Program, const FString& Source)
	{
		return Program.Source.Equals(Source, ESearchCase::IgnoreCase);
	}

	TSharedRef<const FProgram> FindOrCompile(const FString& Source)
	{
		static FRWLock Lock;
		static TMap<FString, TSharedPtr<const FProgram>> Cache;

		{
			FReadScopeLock ReadLock(Lock);
			if (const TSharedPtr<const FProgram>* Found = Cache.Find(Source))
			{
				return Found->ToSharedRef();
			}
		}

		TSharedRef<FProgram> Program = MakeShared<FProgram>();
		Program->Source = Source;
		FText Error;
		Program->bValid = FCompiler(Source, Program->Code).Compile(Error);

		FWriteScopeLock WriteLock(Lock);
		if (const TSharedPtr<const FProgram>* Found = Cache.Find(Source))
		{
			return Found->ToSharedRef();
		}
		if (!Program->bValid)
		{
			Program->Code.Reset();
			UE_LOG(LogProceduralExpression, Warning, TEXT("'%s': %s"), *Source, *Error.ToString());
		}
		Cache.Add(Source, Program);
		return Program;
	}

	bool Validate(const FString& Source, FText& OutError)
	{
		TArray<FInstruction> Code;
		return FCompiler(Source, Code).Compile(OutError);
	}

	bool IsValid(const FProgram& Program)
	{
		return Program.bValid;
	}

	float Evaluate(const FProgram& Program, const FInputs& Inputs)
	{
		if (!Program.bValid)
		{
			return 0.0f;
		}

		float Stack[MaxStackDepth];
		int32 Top = 0;

		for (const FInstruction& Instruction : Program.Code)
		{
			switch (Instruction.Op)
			{
			case EOp::Const: Stack[Top++] = Instruction.Value; break;
			case EOp::T: Stack[Top++] = Inputs.T; break;
			case EOp::X: Stack[Top++] = Inputs.X; break;
			case EOp::P: Stack[Top++] = FMath::Frac(Inputs.X); break;

			case EOp::Neg: Stack[Top - 1] = -Stack[Top - 1]; break;
			case EOp::Not: Stack[Top - 1] = Stack[Top - 1] == 0.0f ? 1.0f : 0.0f; break;

			case EOp::Sin: Stack[Top - 1] = FMath::Sin(Stack[Top - 1]); break;
			case EOp::Cos: Stack[Top - 1] = FMath::Cos(Stack[Top - 1]); break;
			case EOp::Tan: Stack[Top - 1] = FMath::Tan(Stack[Top - 1]); break;
			case EOp::Asin: Stack[Top - 1] = FMath::Asin(FMath::Clamp(Stack[Top - 1], -1.0f, 1.0f)); break;
			case EOp::Acos: Stack[Top - 1] = FMath::Acos(FMath::Clamp(Stack[Top - 1], -1.0f, 1.0f)); break;
			case EOp::Atan: Stack[Top - 1] = FMath::Atan(Stack[Top - 1]); break;
			case EOp::Abs: Stack[Top - 1] = FMath::Abs(Stack[Top - 1]); break;
			case EOp::Sign: Stack[Top - 1] = FMath::Sign(Stack[Top - 1]); break;
			case EOp::Floor: Stack[Top - 1] = FMath::FloorToFloat(Stack[Top - 1]); break;
			case EOp::Ceil: Stack[Top - 1] = FMath::CeilToFloat(Stack[Top - 1]); break;
			case EOp::Round: Stack[Top - 1] = FMath::RoundToFloat(Stack[Top - 1]); break;
			case EOp::Frac: Stack[Top - 1] = FMath::Frac(Stack[Top - 1]); break;
			case EOp::Sqrt: Stack[Top - 1] = FMath::Sqrt(FMath::Max(Stack[Top - 1], 0.0f)); break;
			case EOp::Exp: Stack[Top - 1] = FMath::Exp(Stack[Top - 1]); break;
			case EOp::Log: Stack[Top - 1] = Stack[Top - 1] > 0.0f ? FMath::Loge(Stack[Top - 1]) : 0.0f; break;
			case EOp::Log2: Stack[Top - 1] = Stack[Top - 1] > 0.0f ? FMath::Log2(Stack[Top - 1]) : 0.0f; break;
			case EOp::Saturate: Stack[Top - 1] = FMath::Clamp(Stack[Top - 1], 0.0f, 1.0f); break;
			case EOp::Sine: Stack[Top - 1] = ProceduralWaves::Sine(Stack[Top - 1]); break;
			case EOp::Bounce: Stack[Top - 1] = ProceduralWaves::Bounce(Stack[Top - 1]); break;

			default:
			{
				// Two or more operands: B is the last pushed, A the one before it.
				const float B = Stack[Top - 1];
				const float A = Stack[Top - 2];
				float Result = 0.0f;
				int32 Consumed = 2;

				switch (Instruction.Op)
				{
				case EOp::Add: Result = A + B; break;
				case EOp::Sub: Result = A - B; break;
				case EOp::Mul: Result = A * B; break;
				case EOp::Div: Result = SafeDivide(A, B); break;
				case EOp::Mod: Result = FlooredMod(A, B); break;
				case EOp::Pow: Result = FMath::Pow(A, B); break;
				case EOp::Less: Result = A < B ? 1.0f : 0.0f; break;
				case EOp::LessEqual: Result = A <= B ? 1.0f : 0.0f; break;
				case EOp::Greater: Result = A > B ? 1.0f : 0.0f; break;
				case EOp::GreaterEqual: Result = A >= B ? 1.0f : 0.0f; break;
				case EOp::Equal: Result = A == B ? 1.0f : 0.0f; break;
				case EOp::NotEqual: Result = A != B ? 1.0f : 0.0f; break;
				case EOp::And: Result = (A != 0.0f && B != 0.0f) ? 1.0f : 0.0f; break;
				case EOp::Or: Result = (A != 0.0f || B != 0.0f) ? 1.0f : 0.0f; break;
				case EOp::Atan2: Result = FMath::Atan2(A, B); break;
				case EOp::Min: Result = FMath::Min(A, B); break;
				case EOp::Max: Result = FMath::Max(A, B); break;
				case EOp::Step: Result = B >= A ? 1.0f : 0.0f; break;
				case EOp::Tri: Result = ProceduralWaves::Triangle(A, B); break;
				case EOp::Saw: Result = ProceduralWaves::Sawtooth(A, B); break;
				case EOp::Noise: Result = ProceduralWaves::Noise(A, RoundToSeed(B), 0); break;
				case EOp::Hash: Result = 0.5f * ProceduralWaves::Knot(FMath::FloorToInt32(A), RoundToSeed(B)) + 0.5f; break;
				default:
				{
					const float C = Stack[Top - 3];
					Consumed = 3;
					switch (Instruction.Op)
					{
					// Three operands: C, A, B in push order.
					case EOp::Select: Result = C != 0.0f ? A : B; break;
					case EOp::Clamp: Result = FMath::Clamp(C, A, B); break;
					case EOp::Lerp: Result = FMath::Lerp(C, A, B); break;
					case EOp::SmoothStep: Result = FMath::SmoothStep(C, A, B); break;
					case EOp::SmootherStep:
					{
						const float Alpha = FMath::Clamp(SafeDivide(B - C, A - C), 0.0f, 1.0f);
						Result = Alpha * Alpha * Alpha * (Alpha * (Alpha * 6.0f - 15.0f) + 10.0f);
						break;
					}
					case EOp::Square: Result = ProceduralWaves::Square(C, A, B); break;
					case EOp::PNoise: Result = ProceduralWaves::Noise(C, RoundToSeed(B), FMath::RoundToInt32(A)); break;
					case EOp::SNoise: Result = ProceduralWaves::SteppedNoise(C, RoundToSeed(A), 0, B); break;
					case EOp::Remap:
					{
						// Five operands: V, InLo, InHi, OutLo, OutHi.
						const float V = Stack[Top - 5];
						const float InLo = Stack[Top - 4];
						Consumed = 5;
						Result = FMath::Lerp(A, B, SafeDivide(V - InLo, C - InLo));
						break;
					}
					default:
						break;
					}
					break;
				}
				}

				Top -= Consumed - 1;
				Stack[Top - 1] = Result;
				break;
			}
			}
		}

		const float Value = Top > 0 ? Stack[Top - 1] : 0.0f;
		return FMath::IsFinite(Value) ? Value : 0.0f;
	}

	TArrayView<const FFunctionInfo> GetFunctions()
	{
		static const TArray<FFunctionInfo> Functions = []
		{
			TArray<FFunctionInfo> Result;
			for (const FFunctionDef& Def : GFunctions)
			{
				Result.Add(Def.Info);
			}
			return Result;
		}();
		return Functions;
	}
}

#undef LOCTEXT_NAMESPACE
