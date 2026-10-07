#include "SAPSWorldSchemePreview.h"
#include "APS_ALPHA/UI/Style/APSSlateLineGuard.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Math/RandomStream.h"
#include "Misc/Crc.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"

namespace APSWorldSchemePrivate
{
	enum class EKind : uint8
	{
		Rocky,
		Green,
		Ocean,
		Desert,
		Lava,
		Ice,
		GasGiant,
		IceGiant,
		HotGiant,
		Dwarf,
		Metal,
		Greenhouse,
		Sulfur,
		Crystal,
		Ammonia,
		Unknown
	};

	struct FPlannedBody
	{
		EKind Kind{EKind::Unknown};
		bool bSuper{false};
		double Orbit{0.0};
		int32 RadiusKm{0};
		int32 Moons{0};
		int32 Star{0};
		bool bInhabited{false};
	};

	/** One soft disc of a star's glow: radius in star radii and its opacity. */
	struct FHalo
	{
		float Scale;
		float Alpha;
	};

	constexpr int32 MaxSpecks = 56;
	constexpr int32 MaxDrawnPlanets = 12;
	constexpr int32 MaxCompanions = 2;
	constexpr int32 OrbitSegments = 72;
	constexpr float GoldenAngle = 2.39996323f;
	const FLinearColor OrbitCyan(0.26f, 0.72f, 0.86f, 1.0f);
	const FLinearColor Amber(0.89f, 0.46f, 0.012f, 1.0f);

	const FSlateBrush* Disc()
	{
		// No corner radius: half the height, so a square box paints a circle (as SAPSSystemScheme does).
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White);
		return &Brush;
	}

	FLinearColor Opaque(const FLinearColor& Color, float Alpha)
	{
		return FLinearColor(Color.R, Color.G, Color.B, Alpha);
	}

	FLinearColor Scaled(const FLinearColor& Color, float Factor, float Alpha = 1.0f)
	{
		return FLinearColor(Color.R * Factor, Color.G * Factor, Color.B * Factor, Alpha);
	}

	/** Display names ("Forest Planet") and bare enum names ("Forest") alike. */
	EKind KindFromName(const FString& Name, bool& bOutSuper)
	{
		const FString Upper = Name.ToUpper();
		const auto Has = [&Upper](const TCHAR* Token) { return Upper.Contains(Token); };
		bOutSuper = Has(TEXT("SUPER"));
		if (Has(TEXT("GAS GIANT")) || Has(TEXT("GASGIANT"))) return EKind::GasGiant;
		if (Has(TEXT("ICE GIANT")) || Has(TEXT("ICEGIANT"))) return EKind::IceGiant;
		if (Has(TEXT("HOT GIANT")) || Has(TEXT("HOTGIANT"))) return EKind::HotGiant;
		if (Has(TEXT("LAVA")) || Has(TEXT("VOLCAN")) || Has(TEXT("MELTED"))) return EKind::Lava;
		if (Has(TEXT("OCEAN")) || Has(TEXT("WATER")) || Has(TEXT("ARCHIPELAGO"))) return EKind::Ocean;
		if (Has(TEXT("FOREST")) || Has(TEXT("TERRESTRIAL")) || Has(TEXT("OASIS")) || Has(TEXT("PANGEA"))
			|| Has(TEXT("SAVANNA")) || Has(TEXT("EARTH"))) return EKind::Green;
		if (Has(TEXT("DESERT")) || Has(TEXT("SAND"))) return EKind::Desert;
		if (Has(TEXT("ICE")) || Has(TEXT("FROZEN")) || Has(TEXT("NORDIC")) || Has(TEXT("TUNDRA"))) return EKind::Ice;
		if (Has(TEXT("METAL")) || Has(TEXT("CARBON"))) return EKind::Metal;
		if (Has(TEXT("GREENHOUSE"))) return EKind::Greenhouse;
		if (Has(TEXT("SULFUR"))) return EKind::Sulfur;
		if (Has(TEXT("CRYSTAL"))) return EKind::Crystal;
		if (Has(TEXT("AMMONIA"))) return EKind::Ammonia;
		if (Has(TEXT("DWARF"))) return EKind::Dwarf;
		if (Has(TEXT("ROCK")) || Has(TEXT("BASALT")) || Has(TEXT("MOUNTAIN")) || Has(TEXT("ROGUE"))
			|| Has(TEXT("EXOPLANET"))) return EKind::Rocky;
		return EKind::Unknown;
	}

	FLinearColor KindColor(const EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Lava: return FLinearColor(1.00f, 0.34f, 0.10f);
		case EKind::HotGiant: return FLinearColor(1.00f, 0.50f, 0.22f);
		case EKind::GasGiant: return FLinearColor(0.92f, 0.68f, 0.40f);
		case EKind::IceGiant: return FLinearColor(0.40f, 0.72f, 0.98f);
		case EKind::Ice: return FLinearColor(0.80f, 0.92f, 1.00f);
		case EKind::Ocean: return FLinearColor(0.18f, 0.50f, 1.00f);
		case EKind::Green: return FLinearColor(0.26f, 0.82f, 0.46f);
		case EKind::Desert: return FLinearColor(0.94f, 0.72f, 0.38f);
		case EKind::Metal: return FLinearColor(0.70f, 0.66f, 0.86f);
		case EKind::Greenhouse: return FLinearColor(0.96f, 0.84f, 0.56f);
		case EKind::Sulfur: return FLinearColor(0.92f, 0.86f, 0.30f);
		case EKind::Crystal: return FLinearColor(0.60f, 0.96f, 0.94f);
		case EKind::Ammonia: return FLinearColor(0.74f, 0.80f, 0.54f);
		case EKind::Dwarf: return FLinearColor(0.60f, 0.58f, 0.62f);
		case EKind::Rocky: return FLinearColor(0.66f, 0.63f, 0.60f);
		default: return FLinearColor(0.58f, 0.64f, 0.70f);
		}
	}

	bool IsGiant(const EKind Kind)
	{
		return Kind == EKind::GasGiant || Kind == EKind::IceGiant || Kind == EKind::HotGiant;
	}

	/** Disc radius in planet units, by type when the radius is unknown. */
	float KindScale(const EKind Kind, const bool bSuper)
	{
		switch (Kind)
		{
		case EKind::GasGiant: return 1.50f;
		case EKind::HotGiant: return 1.40f;
		case EKind::IceGiant: return 1.25f;
		case EKind::Dwarf: return 0.55f;
		default: return bSuper ? 1.00f : 0.80f;
		}
	}

	/** Logarithmic: 2,500 km 0.5, Earth 0.78, 25,000 km 1.2, Jupiter 1.5 planet units. */
	float RadiusScale(const int32 RadiusKm)
	{
		return FMath::Clamp(0.5f + 0.42f * FMath::Loge(RadiusKm / 2500.0f) / FMath::Loge(4.0f), 0.45f, 1.7f);
	}

	/** An old record's planets are not known: a plausible layout by orbital zone (0 inner .. 1 outer). */
	EKind DerivedKind(const float Zone, const float Roll)
	{
		if (Zone < 0.22f) return Roll < 0.38f ? EKind::Lava : Roll < 0.72f ? EKind::Rocky : Roll < 0.86f ? EKind::Metal : EKind::Desert;
		if (Zone < 0.50f) return Roll < 0.30f ? EKind::Rocky : Roll < 0.52f ? EKind::Desert : Roll < 0.68f ? EKind::Greenhouse
			: Roll < 0.86f ? EKind::Ocean : EKind::Green;
		if (Zone < 0.80f) return Roll < 0.55f ? EKind::GasGiant : Roll < 0.75f ? EKind::IceGiant : Roll < 0.90f ? EKind::Ice : EKind::Rocky;
		return Roll < 0.40f ? EKind::Ice : Roll < 0.72f ? EKind::IceGiant : EKind::Dwarf;
	}

	float StellarScale(const FString& Stellar)
	{
		if (Stellar.Contains(TEXT("HYPERGIANT"))) return 2.1f;
		if (Stellar.Contains(TEXT("SUPERGIANT"))) return 1.75f;
		if (Stellar.Contains(TEXT("BRIGHTGIANT"))) return 1.5f;
		if (Stellar.Contains(TEXT("SUBGIANT"))) return 1.15f;
		if (Stellar.Contains(TEXT("GIANT"))) return 1.35f;
		if (Stellar.Contains(TEXT("SUBDWARF"))) return 0.85f;
		if (Stellar.Contains(TEXT("WHITEDWARF"))) return 0.42f;
		if (Stellar.Contains(TEXT("BROWNDWARF"))) return 0.6f;
		return 1.0f;
	}

	float ClassScale(const FString& StarClass)
	{
		if (StarClass == TEXT("O")) return 1.45f;
		if (StarClass == TEXT("B")) return 1.30f;
		if (StarClass == TEXT("A")) return 1.15f;
		if (StarClass == TEXT("F")) return 1.05f;
		if (StarClass == TEXT("K")) return 0.90f;
		if (StarClass == TEXT("M")) return 0.75f;
		if (StarClass == TEXT("L")) return 0.62f;
		if (StarClass == TEXT("T")) return 0.56f;
		if (StarClass == TEXT("Y")) return 0.52f;
		if (StarClass == TEXT("NS")) return 0.45f;
		if (StarClass == TEXT("PS")) return 1.2f;
		if (StarClass == TEXT("BH")) return 0.9f;
		return 1.0f;
	}

	/** "SUPER GIANT" and "SuperGiant" alike. */
	FString CompactStellar(const FString& StellarType)
	{
		return StellarType.ToUpper().Replace(TEXT(" "), TEXT("")).Replace(TEXT("-"), TEXT(""));
	}

	void AppendEllipse(TArray<FVector2D>& Out, TArray<FLinearColor>* OutColors, const FVector2D& Centre, const float RadiusX,
		const float RadiusY, const FVector2D& Roll, const float From, const float To, const int32 Segments,
		const FLinearColor& Color, const float BackAlpha, const float FrontAlpha)
	{
		Out.Reserve(Out.Num() + Segments + 1);
		for (int32 Step = 0; Step <= Segments; ++Step)
		{
			const float Angle = FMath::Lerp(From, To, static_cast<float>(Step) / Segments);
			const FVector2D Flat(RadiusX * FMath::Cos(Angle), RadiusY * FMath::Sin(Angle));
			Out.Add(Centre + FVector2D(Flat.X * Roll.X - Flat.Y * Roll.Y, Flat.X * Roll.Y + Flat.Y * Roll.X));
			if (OutColors)
			{
				// Near side (lower half) brighter than the far side: the plane reads as tilted toward the viewer.
				const float Depth = 0.5f + 0.5f * FMath::Sin(Angle);
				OutColors->Add(Opaque(Color, FMath::Lerp(BackAlpha, FrontAlpha, Depth)));
			}
		}
	}
}

namespace APSWorldScheme
{
	FString StarClassFromLabel(const FString& SpectralLabel)
	{
		const FString Label = SpectralLabel.TrimStartAndEnd().ToUpper();
		if (Label.IsEmpty() || Label.StartsWith(TEXT("UNKNOWN")))
		{
			return FString();
		}
		if (Label == TEXT("NS") || Label.Contains(TEXT("NEUTRON")) || Label.Contains(TEXT("PULSAR"))) return TEXT("NS");
		if (Label == TEXT("BH") || Label.Contains(TEXT("BLACK HOLE"))) return TEXT("BH");
		if (Label == TEXT("PS") || Label.Contains(TEXT("PROTO"))) return TEXT("PS");
		// "G - Yellow", "G2V", "G"; never "GENERATED STAR".
		const TCHAR First = Label[0];
		const bool bLetter = FCString::Strchr(TEXT("OBAFGKMLTY"), First) != nullptr;
		return bLetter && (Label.Len() == 1 || !FChar::IsAlpha(Label[1])) ? FString::Chr(First) : FString();
	}

	FString DescribeStar(const FString& StarClass, const FString& StellarType)
	{
		const FString Stellar = APSWorldSchemePrivate::CompactStellar(StellarType);
		if (StarClass == TEXT("BH") || Stellar.Contains(TEXT("BLACKHOLE"))) return TEXT("BLACK HOLE");
		if (Stellar.Contains(TEXT("PULSAR"))) return TEXT("PULSAR");
		if (StarClass == TEXT("NS") || Stellar.Contains(TEXT("NEUTRON"))) return TEXT("NEUTRON STAR");
		if (StarClass == TEXT("PS") || Stellar.Contains(TEXT("PROTO"))) return TEXT("PROTOSTAR");
		if (Stellar.Contains(TEXT("WHITEDWARF"))) return TEXT("WHITE DWARF");
		if (StarClass == TEXT("L") || StarClass == TEXT("T") || StarClass == TEXT("Y") || Stellar.Contains(TEXT("BROWNDWARF")))
		{
			return StarClass.IsEmpty() ? FString(TEXT("BROWN DWARF")) : StarClass + TEXT(" BROWN DWARF");
		}
		const TCHAR* Colour = StarClass == TEXT("O") ? TEXT("BLUE") : StarClass == TEXT("B") ? TEXT("BLUE-WHITE")
			: StarClass == TEXT("A") ? TEXT("WHITE") : StarClass == TEXT("F") ? TEXT("YELLOW-WHITE")
			: StarClass == TEXT("G") ? TEXT("YELLOW") : StarClass == TEXT("K") ? TEXT("ORANGE")
			: StarClass == TEXT("M") ? TEXT("RED") : nullptr;
		if (!Colour)
		{
			return TEXT("UNKNOWN STAR");
		}
		const TCHAR* Noun = TEXT("STAR");
		if (Stellar.Contains(TEXT("HYPERGIANT"))) Noun = TEXT("HYPERGIANT");
		else if (Stellar.Contains(TEXT("SUPERGIANT"))) Noun = TEXT("SUPERGIANT");
		else if (Stellar.Contains(TEXT("BRIGHTGIANT"))) Noun = TEXT("BRIGHT GIANT");
		else if (Stellar.Contains(TEXT("SUBGIANT"))) Noun = TEXT("SUBGIANT");
		else if (Stellar.Contains(TEXT("GIANT"))) Noun = TEXT("GIANT");
		else if (Stellar.Contains(TEXT("SUBDWARF"))) Noun = TEXT("SUBDWARF");
		// A main-sequence G, K or M star is a dwarf; hotter ones are simply stars.
		else if (Stellar.Contains(TEXT("MAINSEQUENCE"))
			&& (StarClass == TEXT("G") || StarClass == TEXT("K") || StarClass == TEXT("M"))) Noun = TEXT("DWARF");
		return FString::Printf(TEXT("%s %s %s"), *StarClass, Colour, Noun);
	}

	FLinearColor StarColor(const FString& StarClass)
	{
		if (StarClass == TEXT("O")) return FLinearColor(0.62f, 0.72f, 1.00f);
		if (StarClass == TEXT("B")) return FLinearColor(0.72f, 0.82f, 1.00f);
		if (StarClass == TEXT("A")) return FLinearColor(0.90f, 0.94f, 1.00f);
		if (StarClass == TEXT("F")) return FLinearColor(1.00f, 0.96f, 0.84f);
		if (StarClass == TEXT("G")) return FLinearColor(1.00f, 0.84f, 0.48f);
		if (StarClass == TEXT("K")) return FLinearColor(1.00f, 0.62f, 0.28f);
		if (StarClass == TEXT("M")) return FLinearColor(1.00f, 0.40f, 0.22f);
		if (StarClass == TEXT("L")) return FLinearColor(0.86f, 0.34f, 0.22f);
		if (StarClass == TEXT("T")) return FLinearColor(0.72f, 0.30f, 0.42f);
		if (StarClass == TEXT("Y")) return FLinearColor(0.56f, 0.30f, 0.44f);
		if (StarClass == TEXT("NS")) return FLinearColor(0.72f, 0.86f, 1.00f);
		if (StarClass == TEXT("PS")) return FLinearColor(1.00f, 0.50f, 0.26f);
		if (StarClass == TEXT("BH")) return FLinearColor(1.00f, 0.56f, 0.20f);
		return FLinearColor(0.98f, 0.92f, 0.80f);
	}
}

void SAPSWorldSchemePreview::Construct(const FArguments& InArgs)
{
	using namespace APSWorldSchemePrivate;
	SetVisibility(EVisibility::HitTestInvisible);
	SetClipping(EWidgetClipping::ClipToBounds);
	const FAPSWorldSchemeInput& Input = InArgs._Input;
	bRecordedSystem = Input.bRecorded;
	bCentred = InArgs._Centred;

	// Cosmetic choices come from the slot name: almost every save shares the default generation seed (Rio 03.10 audit),
	// so a seed would paint the same picture on every card. The same slot always paints the same picture.
	FRandomStream Stream(static_cast<int32>(FCrc::StrCrc32(*Input.Key.ToUpper())));
	Specks.SetNum(MaxSpecks);
	for (FSpeck& Speck : Specks)
	{
		Speck.X = Stream.FRand();
		Speck.Y = Stream.FRand();
		Speck.Size = 0.7f + FMath::Square(Stream.FRand()) * 1.1f;
		Speck.Alpha = 0.10f + FMath::Pow(Stream.FRand(), 1.7f) * 0.42f;
		Speck.bWarm = Stream.FRand() < 0.3f;
	}
	PlaneRoll = -FMath::DegreesToRadians(2.5f + 5.5f * Stream.FRand());
	const float PhaseBase = UE_TWO_PI * Stream.FRand();

	// The star: its class is the menu's home star setting, which the generator applies, so even an old record keeps it.
	const FString Stellar = CompactStellar(Input.StellarType);
	StarTint = APSWorldScheme::StarColor(Input.StarClass);
	StarScale = ClassScale(Input.StarClass) * StellarScale(Stellar);
	if (Stellar.Contains(TEXT("WHITEDWARF")))
	{
		StarTint = FLinearColor(0.86f, 0.92f, 1.00f);
	}
	StarLook = Input.StarClass == TEXT("BH") || Stellar.Contains(TEXT("BLACKHOLE")) ? EStarLook::BlackHole
		: Input.StarClass == TEXT("NS") || Stellar.Contains(TEXT("NEUTRON")) || Stellar.Contains(TEXT("PULSAR")) ? EStarLook::Neutron
		: Input.StarClass == TEXT("PS") || Stellar.Contains(TEXT("PROTO")) ? EStarLook::Proto : EStarLook::Normal;

	// The planets: recorded, or for an old record a plausible layout drawn from the slot name.
	TArray<FPlannedBody> Planned;
	if (Input.bRecorded)
	{
		for (const FAPSWorldSchemePlanet& Planet : Input.Planets)
		{
			FPlannedBody& Body = Planned.AddDefaulted_GetRef();
			Body.Kind = KindFromName(Planet.Type, Body.bSuper);
			Body.Orbit = Planet.Orbit;
			Body.RadiusKm = Planet.RadiusKm;
			Body.Moons = Planet.Moons;
			Body.Star = Planet.Star;
			Body.bInhabited = Planet.bInhabited;
		}
		HomeBody = Input.Planets.IsValidIndex(Input.HomeIndex) ? Input.HomeIndex : INDEX_NONE;
		for (int32 Companion = 1; Companion < FMath::Min(Input.StarCount, MaxCompanions + 1); ++Companion)
		{
			const float Roll = Stream.FRand();
			CompanionColors.Add(APSWorldScheme::StarColor(Roll < 0.45f ? TEXT("K") : Roll < 0.8f ? TEXT("M") : TEXT("G")));
		}
	}
	else
	{
		const bool bHot = Input.StarClass == TEXT("O") || Input.StarClass == TEXT("B") || Input.StarClass == TEXT("A");
		const bool bCool = Input.StarClass == TEXT("K") || Input.StarClass == TEXT("M");
		const int32 Count = 2 + Stream.RandHelper(6);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Zone = (Index + 0.5f) / Count + (bHot ? -0.1f : bCool ? 0.1f : 0.0f);
			FPlannedBody& Body = Planned.AddDefaulted_GetRef();
			Body.Kind = DerivedKind(Zone, Stream.FRand());
			Body.Moons = IsGiant(Body.Kind) ? Stream.RandHelper(4) : Stream.RandHelper(2);
		}
		// A dusty belt between the rocky worlds and the first giant, in about half of the illustrations.
		if (Count >= 3 && Stream.FRand() < 0.5f)
		{
			int32 Inner = Count / 2 - 1;
			for (int32 Index = 1; Index < Count; ++Index)
			{
				if (IsGiant(Planned[Index].Kind))
				{
					Inner = Index - 1;
					break;
				}
			}
			BeltOrbit = FMath::Clamp(Inner, 0, Count - 2) + 0.5f;
			BeltWidth = 0.16f;
		}
	}
	if (Planned.Num() > MaxDrawnPlanets)
	{
		// Very large edited systems draw their first dozen; the captions keep the true count.
		Planned.SetNum(MaxDrawnPlanets);
		if (HomeBody >= MaxDrawnPlanets)
		{
			HomeBody = INDEX_NONE;
		}
	}

	// Orbit order per star; spacing from the recorded radii (logarithmic) or even with a little jitter.
	Bodies.SetNum(Planned.Num());
	const int32 DrawnStars = 1 + CompanionColors.Num();
	for (int32 StarIndex = 0; StarIndex < DrawnStars; ++StarIndex)
	{
		TArray<int32> Members;
		bool bRealOrbits = true;
		for (int32 Index = 0; Index < Planned.Num(); ++Index)
		{
			if (FMath::Min(Planned[Index].Star, DrawnStars - 1) == StarIndex)
			{
				Members.Add(Index);
				bRealOrbits &= Planned[Index].Orbit > 0.0;
			}
		}
		const int32 Count = Members.Num();
		const double InnerLog = Count > 0 && bRealOrbits ? FMath::Loge(Planned[Members[0]].Orbit) : 0.0;
		const double OuterLog = Count > 0 && bRealOrbits ? FMath::Loge(Planned[Members.Last()].Orbit) : 0.0;
		bRealOrbits &= OuterLog - InnerLog > 1.0e-3;
		float Previous = -1.0f;
		for (int32 Order = 0; Order < Count; ++Order)
		{
			float Fraction = Count == 1 ? 0.5f : static_cast<float>(Order) / (Count - 1);
			if (bRealOrbits)
			{
				Fraction = static_cast<float>((FMath::Loge(Planned[Members[Order]].Orbit) - InnerLog) / (OuterLog - InnerLog));
			}
			else if (Order > 0 && Order < Count - 1)
			{
				Fraction += Stream.FRandRange(-0.22f, 0.22f) / (Count - 1);
			}
			// Neighbours keep a readable gap even when their real orbits nearly touch.
			if (Count > 1 && Previous >= 0.0f)
			{
				Fraction = FMath::Max(Fraction, Previous + 0.32f / (Count - 1));
			}
			Previous = Fraction;
			Bodies[Members[Order]].Orbit = Fraction;
			Bodies[Members[Order]].Star = StarIndex;
		}
		if (Count > 1 && Previous > 1.0f)
		{
			for (const int32 Member : Members)
			{
				Bodies[Member].Orbit /= Previous;
			}
		}
	}

	for (int32 Index = 0; Index < Planned.Num(); ++Index)
	{
		const FPlannedBody& Plan = Planned[Index];
		FBody& Body = Bodies[Index];
		Body.Color = KindColor(Plan.Kind);
		Body.Scale = Plan.RadiusKm > 0 ? RadiusScale(Plan.RadiusKm) : KindScale(Plan.Kind, Plan.bSuper);
		Body.Phase = PhaseBase + Index * GoldenAngle + Stream.FRandRange(-0.3f, 0.3f);
		Body.Moons = FMath::Clamp(Plan.Moons, 0, 3);
		Body.bInhabited = Plan.bInhabited;
		Body.bBands = IsGiant(Plan.Kind);
		Body.bRing = (Plan.Kind == EKind::GasGiant && Stream.FRand() < 0.55f)
			|| (Plan.Kind == EKind::IceGiant && Stream.FRand() < 0.3f);
		Body.bAtmosphere = Plan.Kind == EKind::Green || Plan.Kind == EKind::Ocean;
		if (!bRecordedSystem)
		{
			// An illustration, not a record: calmer colours than a recorded system.
			Body.Color = FMath::Lerp(Body.Color, FLinearColor(0.56f, 0.64f, 0.72f), 0.35f);
		}
	}

	BeltDots.SetNum(BeltOrbit >= 0.0f ? 46 : 0);
	for (FBeltDot& Dot : BeltDots)
	{
		Dot.Phase = UE_TWO_PI * Stream.FRand();
		Dot.Offset = Stream.FRand() + Stream.FRand() - 1.0f;
		Dot.Size = 0.8f + 0.6f * Stream.FRand();
		Dot.Alpha = 0.18f + 0.30f * Stream.FRand();
	}
	if (BeltOrbit >= 0.0f && Planned.Num() > 1)
	{
		// BeltOrbit holds the gap between two orbit indices; turn it into the orbit fraction between them.
		const int32 Inner = FMath::FloorToInt(BeltOrbit);
		BeltWidth = (Bodies[Inner + 1].Orbit - Bodies[Inner].Orbit) * 0.32f;
		BeltOrbit = (Bodies[Inner].Orbit + Bodies[Inner + 1].Orbit) * 0.5f;
	}
}

void SAPSWorldSchemePreview::UpdatePixels(const FVector2D& LocalSize) const
{
	using namespace APSWorldSchemePrivate;
	FPixels& Px = Pixels;
	Px = FPixels();
	Px.Size = LocalSize;
	const float Width = static_cast<float>(LocalSize.X);
	const float Height = static_cast<float>(LocalSize.Y);
	const int32 Companions = CompanionColors.Num();
	const float CentreY = Height * (bCentred ? 0.50f : 0.56f);
	Px.Centre = FVector2D(Width * 0.5f, CentreY);
	const float Unit = FMath::Clamp(Height * 0.052f, 2.6f, 9.0f);
	Px.StarRadius = FMath::Clamp(FMath::Clamp(Height * 0.068f, 3.5f, 13.0f) * StarScale, 2.5f, Height * 0.2f);
	const float LargestBody = Unit * 1.7f;
	const float Span = Width * (Companions > 0 ? 0.36f : 0.46f);
	const float OuterX = FMath::Max(Span - LargestBody, 24.0f);
	const float InnerX = FMath::Min(FMath::Max(Px.StarRadius * 2.4f + Unit, Width * 0.10f), OuterX * 0.55f);
	const float Room = FMath::Min(CentreY, Height - CentreY) - LargestBody - 3.0f;
	const float Tilt = FMath::Clamp(Room / OuterX, 0.16f, 0.34f);
	const FVector2D Roll(FMath::Cos(PlaneRoll), FMath::Sin(PlaneRoll));
	const auto Project = [&Roll](const FVector2D& Centre, const float RadiusX, const float RadiusY, const float Angle)
	{
		const FVector2D Flat(RadiusX * FMath::Cos(Angle), RadiusY * FMath::Sin(Angle));
		return Centre + FVector2D(Flat.X * Roll.X - Flat.Y * Roll.Y, Flat.X * Roll.Y + Flat.Y * Roll.X);
	};

	Px.Ecliptic = {Px.Centre - Roll * Width, Px.Centre + Roll * Width};
	Px.SpeckCount = FMath::Clamp(FMath::RoundToInt(Width * Height / 2600.0f), 14, Specks.Num());
	Px.SpeckPositions.Reserve(Px.SpeckCount);
	for (int32 Index = 0; Index < Px.SpeckCount; ++Index)
	{
		Px.SpeckPositions.Add(FVector2D(Specks[Index].X * Width, Specks[Index].Y * Height));
	}

	// Companions of a multiple star sit to the sides, beyond the main orbits.
	for (int32 Companion = 0; Companion < Companions; ++Companion)
	{
		const float Side = Companion == 0 ? 1.0f : -1.0f;
		Px.CompanionCentres.Add(FVector2D(Width * (0.5f + Side * 0.43f), CentreY - Side * Height * 0.12f));
		Px.CompanionRadii.Add(FMath::Max(Px.StarRadius * 0.55f, 2.5f));
	}

	const int32 BodyCount = Bodies.Num();
	Px.BodyCentres.SetNum(BodyCount);
	Px.BodyRadii.SetNum(BodyCount);
	Px.BodyFront.SetNum(BodyCount);
	Px.BodyLight.SetNum(BodyCount);
	Px.BodyDepth.SetNum(BodyCount);
	Px.OrbitBack.SetNum(BodyCount);
	Px.OrbitBackColors.SetNum(BodyCount);
	Px.OrbitFront.SetNum(BodyCount);
	Px.OrbitFrontColors.SetNum(BodyCount);
	Px.RingBack.SetNum(BodyCount);
	Px.RingFront.SetNum(BodyCount);
	Px.InhabitedRings.SetNum(BodyCount);
	TArray<int32> CompanionOrder;
	CompanionOrder.Init(0, Companions + 1);
	for (int32 Index = 0; Index < BodyCount; ++Index)
	{
		const FBody& Body = Bodies[Index];
		const bool bCompanion = Body.Star > 0 && Px.CompanionCentres.IsValidIndex(Body.Star - 1);
		const FVector2D Centre = bCompanion ? Px.CompanionCentres[Body.Star - 1] : Px.Centre;
		const float RadiusX = bCompanion
			? Px.CompanionRadii[Body.Star - 1] * 2.6f + Unit * 2.0f * CompanionOrder[Body.Star]++
			: FMath::Lerp(InnerX, OuterX, Body.Orbit);
		const float RadiusY = RadiusX * Tilt;
		float Angle = Body.Phase;
		const float CentreRadius = bCompanion ? Px.CompanionRadii[Body.Star - 1] : Px.StarRadius;
		if (RadiusY < CentreRadius * 1.9f && FMath::Abs(FMath::Cos(Angle)) < 0.6f)
		{
			// A tight orbit: keep the planet off the star's disc, to its side.
			Angle = FMath::Atan2(FMath::Sin(Angle) >= 0.0f ? 0.8f : -0.8f, FMath::Cos(Angle) >= 0.0f ? 0.6f : -0.6f);
		}
		const float Depth = 0.5f + 0.5f * FMath::Sin(Angle);
		Px.BodyCentres[Index] = Project(Centre, RadiusX, RadiusY, Angle);
		Px.BodyFront[Index] = FMath::Sin(Angle) >= 0.0f;
		Px.BodyDepth[Index] = Depth;
		Px.BodyRadii[Index] = FMath::Max(Unit * Body.Scale * (bCompanion ? 0.75f : 1.0f) * (0.86f + 0.24f * Depth), 1.6f);
		Px.BodyLight[Index] = (Centre - Px.BodyCentres[Index]).GetSafeNormal();

		const FLinearColor OrbitColor = Body.bInhabited ? FMath::Lerp(OrbitCyan, Amber, 0.35f) : OrbitCyan;
		const float Strength = Body.bInhabited ? 0.62f : bRecordedSystem ? 0.34f : 0.24f;
		const int32 Half = OrbitSegments / 2;
		AppendEllipse(Px.OrbitBack[Index], &Px.OrbitBackColors[Index], Centre, RadiusX, RadiusY, Roll, UE_PI, UE_TWO_PI,
			Half, OrbitColor, Strength * 0.35f, Strength);
		AppendEllipse(Px.OrbitFront[Index], &Px.OrbitFrontColors[Index], Centre, RadiusX, RadiusY, Roll, 0.0f, UE_PI,
			Half, OrbitColor, Strength * 0.35f, Strength);

		const float Radius = Px.BodyRadii[Index];
		if (Body.bRing)
		{
			const FVector2D RingRoll(FMath::Cos(PlaneRoll - 0.3f), FMath::Sin(PlaneRoll - 0.3f));
			AppendEllipse(Px.RingBack[Index], nullptr, Px.BodyCentres[Index], Radius * 2.0f, Radius * 0.5f, RingRoll,
				UE_PI, UE_TWO_PI, 20, FLinearColor::White, 1.0f, 1.0f);
			AppendEllipse(Px.RingFront[Index], nullptr, Px.BodyCentres[Index], Radius * 2.0f, Radius * 0.5f, RingRoll,
				0.0f, UE_PI, 20, FLinearColor::White, 1.0f, 1.0f);
		}
		if (Body.bInhabited)
		{
			AppendEllipse(Px.InhabitedRings[Index], nullptr, Px.BodyCentres[Index], Radius + 3.0f, Radius + 3.0f,
				FVector2D(1.0, 0.0), 0.0f, UE_TWO_PI, 28, FLinearColor::White, 1.0f, 1.0f);
		}
		if (Body.bBands && Radius >= 3.0f)
		{
			for (const float Offset : {-0.36f, 0.04f, 0.42f})
			{
				const float Y = Offset * Radius;
				const float HalfChord = FMath::Sqrt(FMath::Max(Radius * Radius - Y * Y, 0.0f)) * 0.92f;
				Px.BandLines.Add(TArray<FVector2D>{Px.BodyCentres[Index] + FVector2D(-HalfChord, Y),
					Px.BodyCentres[Index] + FVector2D(HalfChord, Y)});
				Px.BandOwners.Add(Index);
			}
		}
		if (Radius >= 2.5f)
		{
			// Moons on a small ellipse wide enough to stay off the disc.
			const float MoonX = Radius * 2.0f + 3.0f;
			for (int32 Moon = 0; Moon < Body.Moons; ++Moon)
			{
				Px.MoonPositions.Add(Px.BodyCentres[Index] + FVector2D(MoonX * FMath::Cos(Body.Phase * 3.0f + Moon * 2.1f),
					MoonX * 0.55f * FMath::Sin(Body.Phase * 3.0f + Moon * 2.1f)));
				Px.MoonOwners.Add(Index);
			}
		}
	}

	for (const FBeltDot& Dot : BeltDots)
	{
		const float RadiusX = FMath::Lerp(InnerX, OuterX, BeltOrbit + Dot.Offset * BeltWidth);
		Px.BeltPositions.Add(Project(Px.Centre, RadiusX, RadiusX * Tilt, Dot.Phase));
		Px.BeltFront.Add(FMath::Sin(Dot.Phase) >= 0.0f);
	}

	const float StarRadius = Px.StarRadius;
	if (StarLook == EStarLook::BlackHole || StarLook == EStarLook::Proto)
	{
		// The accretion disc of a black hole, the dusty disc of a protostar; split so the star hides its far side.
		const float DiscX = StarRadius * (StarLook == EStarLook::BlackHole ? 2.3f : 3.0f);
		AppendEllipse(Px.DiscBack, nullptr, Px.Centre, DiscX, DiscX * 0.27f, Roll, UE_PI, UE_TWO_PI, 24,
			FLinearColor::White, 1.0f, 1.0f);
		AppendEllipse(Px.DiscFront, nullptr, Px.Centre, DiscX, DiscX * 0.27f, Roll, 0.0f, UE_PI, 24,
			FLinearColor::White, 1.0f, 1.0f);
	}
	if (StarLook == EStarLook::BlackHole)
	{
		AppendEllipse(Px.PhotonRing, nullptr, Px.Centre, StarRadius * 1.1f, StarRadius * 1.1f, FVector2D(1.0, 0.0),
			0.0f, UE_TWO_PI, 28, FLinearColor::White, 1.0f, 1.0f);
	}
	if (StarLook == EStarLook::Neutron)
	{
		const FVector2D Beam(FMath::Cos(PlaneRoll + 1.15f), FMath::Sin(PlaneRoll + 1.15f));
		Px.Beams = {Px.Centre - Beam * StarRadius * 7.0f, Px.Centre + Beam * StarRadius * 7.0f};
	}
}

int32 SAPSWorldSchemePreview::OnPaint(const FPaintArgs&, const FGeometry& AllottedGeometry, const FSlateRect&,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle&, bool) const
{
	using namespace APSWorldSchemePrivate;
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (LocalSize.X < 16.0 || LocalSize.Y < 16.0)
	{
		return LayerId;
	}
	if (!Pixels.Size.Equals(LocalSize, 0.5))
	{
		UpdatePixels(LocalSize);
	}
	const FPixels& Px = Pixels;
	const FSlateBrush* Fill = FAppStyle::GetBrush("WhiteBrush");
	const FPaintGeometry Area = AllottedGeometry.ToPaintGeometry();
	const auto DrawDisc = [&](const int32 Layer, const FVector2D& At, const float Radius, const FLinearColor& Color)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(FVector2D(Radius * 2.0f),
			FSlateLayoutTransform(At - FVector2D(Radius))), Disc(), ESlateDrawEffect::None, Color);
	};
	const auto DrawLine = [&](const int32 Layer, const TArray<FVector2D>& Points, const FLinearColor& Color, const float Thickness)
	{
		if (Points.Num() >= 2 && APSSlateLineGuard::IsDrawable(Points))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, Area, Points, ESlateDrawEffect::None, Color, true, Thickness);
		}
	};
	const auto DrawFadedLine = [&](const int32 Layer, const TArray<FVector2D>& Points, const TArray<FLinearColor>& Colors)
	{
		if (Points.Num() >= 2 && Colors.Num() == Points.Num() && APSSlateLineGuard::IsDrawable(Points))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, Area, Points, Colors, ESlateDrawEffect::None,
				FLinearColor::White, true, 1.0f);
		}
	};

	// Space: a deep plate, a faint halo of the star's light and a few far stars.
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Area, Fill, ESlateDrawEffect::None,
		FLinearColor(0.0035f, 0.010f, 0.019f, 1.0f));
	for (const float Reach : {0.95f, 0.74f, 0.55f, 0.38f})
	{
		// Faint nested discs: a soft radial falloff without a visible edge.
		DrawDisc(LayerId + 1, Px.Centre, static_cast<float>(LocalSize.Y) * Reach, Opaque(StarTint, 0.016f));
	}
	for (int32 Index = 0; Index < Px.SpeckCount; ++Index)
	{
		const FSpeck& Speck = Specks[Index];
		const FLinearColor Tone = Speck.bWarm ? FLinearColor(1.0f, 0.88f, 0.76f, Speck.Alpha) : FLinearColor(0.78f, 0.88f, 1.0f, Speck.Alpha);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(FVector2D(Speck.Size),
			FSlateLayoutTransform(Px.SpeckPositions[Index] - FVector2D(Speck.Size * 0.5f))), Fill, ESlateDrawEffect::None, Tone);
	}
	DrawLine(LayerId + 2, Px.Ecliptic, Opaque(OrbitCyan, 0.06f), 1.0f);

	// Far halves of the orbits and the belt, then the planets behind the star.
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		DrawFadedLine(LayerId + 3, Px.OrbitBack[Index], Px.OrbitBackColors[Index]);
	}
	const FLinearColor Dust(0.78f, 0.72f, 0.62f, 1.0f);
	for (int32 Index = 0; Index < Px.BeltPositions.Num(); ++Index)
	{
		const FBeltDot& Dot = BeltDots[Index];
		DrawDisc(Px.BeltFront[Index] ? LayerId + 8 : LayerId + 3, Px.BeltPositions[Index], Dot.Size * 0.5f,
			Opaque(Dust, Dot.Alpha * (Px.BeltFront[Index] ? 1.0f : 0.6f)));
	}

	const auto DrawBody = [&](const int32 Index, const int32 Layer)
	{
		const FBody& Body = Bodies[Index];
		const FVector2D At = Px.BodyCentres[Index];
		const float Radius = Px.BodyRadii[Index];
		const float Light = 0.78f + 0.22f * Px.BodyDepth[Index];
		const FLinearColor Base = Scaled(Body.Color, Light);
		DrawLine(Layer, Px.RingBack[Index], FLinearColor(0.92f, 0.84f, 0.68f, 0.55f), FMath::Max(1.0f, Radius * 0.16f));
		if (Body.bInhabited)
		{
			DrawDisc(Layer + 1, At, Radius + 6.0f, Opaque(Amber, 0.10f));
		}
		if (Body.bAtmosphere)
		{
			DrawDisc(Layer + 1, At, Radius * 1.35f, FLinearColor(0.45f, 0.75f, 1.0f, 0.16f));
		}
		DrawDisc(Layer + 1, At, Radius, Base);
		if (Radius >= 2.6f)
		{
			// Lit from its star: a darker far side and a highlight toward the light.
			const FVector2D ToLight = Px.BodyLight[Index];
			DrawDisc(Layer + 1, At - ToLight * Radius * 0.20f, Radius * 0.80f, Scaled(Body.Color, 0.32f * Light, 0.55f));
			DrawDisc(Layer + 1, At + ToLight * Radius * 0.30f, Radius * 0.50f,
				Opaque(FMath::Lerp(Base, FLinearColor::White, 0.35f), 0.5f));
		}
		DrawLine(Layer + 2, Px.RingFront[Index], FLinearColor(0.92f, 0.84f, 0.68f, 0.85f), FMath::Max(1.0f, Radius * 0.16f));
		DrawLine(Layer + 2, Px.InhabitedRings[Index], Amber, 1.3f);
	};
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		if (!Px.BodyFront[Index])
		{
			DrawBody(Index, LayerId + 4);
		}
	}

	// The star (and the companions of a multiple system) over the far side of its orbits.
	const float StarRadius = Px.StarRadius;
	const auto DrawGlow = [&](const FVector2D& At, const float Radius, const FLinearColor& Tint,
		std::initializer_list<FHalo> Halos)
	{
		for (const FHalo& Halo : Halos)
		{
			DrawDisc(LayerId + 7, At, Radius * Halo.Scale, Opaque(Tint, Halo.Alpha));
		}
	};
	switch (StarLook)
	{
	case EStarLook::BlackHole:
		DrawLine(LayerId + 6, Px.DiscBack, Opaque(StarTint, 0.65f), 2.0f);
		DrawGlow(Px.Centre, StarRadius, StarTint, {{3.6f, 0.05f}, {2.4f, 0.09f}});
		DrawDisc(LayerId + 7, Px.Centre, StarRadius, FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
		DrawLine(LayerId + 8, Px.PhotonRing, FLinearColor(1.0f, 0.78f, 0.48f, 0.85f), 1.2f);
		DrawLine(LayerId + 8, Px.DiscFront, Opaque(FMath::Lerp(StarTint, FLinearColor::White, 0.3f), 0.95f), 2.2f);
		break;
	case EStarLook::Neutron:
		DrawGlow(Px.Centre, StarRadius, StarTint, {{5.2f, 0.035f}, {3.2f, 0.08f}, {1.9f, 0.22f}});
		DrawDisc(LayerId + 7, Px.Centre, StarRadius, FMath::Lerp(StarTint, FLinearColor::White, 0.6f));
		DrawLine(LayerId + 8, Px.Beams, Opaque(StarTint, 0.35f), 1.2f);
		break;
	case EStarLook::Proto:
		DrawLine(LayerId + 6, Px.DiscBack, Opaque(StarTint, 0.22f), 3.0f);
		DrawGlow(Px.Centre, StarRadius, StarTint, {{5.0f, 0.05f}, {3.4f, 0.08f}, {2.3f, 0.14f}, {1.5f, 0.28f}});
		DrawDisc(LayerId + 7, Px.Centre, StarRadius, StarTint);
		DrawLine(LayerId + 8, Px.DiscFront, Opaque(StarTint, 0.32f), 3.0f);
		break;
	default:
		// Rio 03.10 ("the stars' size and spectral colour should read too, not a huge difference"): the class colour
		// keeps its hue through the disc (a lighter limb, a hot core only half way to white), and the glow carries it.
		DrawGlow(Px.Centre, StarRadius, StarTint, {{4.4f, 0.045f}, {3.1f, 0.08f}, {2.2f, 0.14f}, {1.6f, 0.26f}, {1.22f, 0.48f}});
		DrawDisc(LayerId + 7, Px.Centre, StarRadius, FMath::Lerp(StarTint, FLinearColor::White, 0.18f));
		DrawDisc(LayerId + 7, Px.Centre, StarRadius * 0.58f, Opaque(FMath::Lerp(StarTint, FLinearColor::White, 0.55f), 0.9f));
		break;
	}
	for (int32 Companion = 0; Companion < Px.CompanionCentres.Num(); ++Companion)
	{
		const FLinearColor Tint = CompanionColors[Companion];
		const float Radius = Px.CompanionRadii[Companion];
		DrawGlow(Px.CompanionCentres[Companion], Radius, Tint, {{3.4f, 0.05f}, {2.0f, 0.12f}, {1.35f, 0.3f}});
		DrawDisc(LayerId + 7, Px.CompanionCentres[Companion], Radius, FMath::Lerp(Tint, FLinearColor::White, 0.45f));
	}

	// Near halves of the orbits over the star, then the planets in front.
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		DrawFadedLine(LayerId + 8, Px.OrbitFront[Index], Px.OrbitFrontColors[Index]);
	}
	for (int32 Index = 0; Index < Bodies.Num(); ++Index)
	{
		if (Px.BodyFront[Index])
		{
			DrawBody(Index, LayerId + 9);
		}
	}
	for (int32 Band = 0; Band < Px.BandLines.Num(); ++Band)
	{
		const int32 Owner = Px.BandOwners[Band];
		DrawLine(Px.BodyFront[Owner] ? LayerId + 11 : LayerId + 6, Px.BandLines[Band],
			Scaled(Bodies[Owner].Color, 0.62f, 0.7f), FMath::Max(1.0f, Px.BodyRadii[Owner] * 0.18f));
	}
	for (int32 Moon = 0; Moon < Px.MoonPositions.Num(); ++Moon)
	{
		const int32 Owner = Px.MoonOwners[Moon];
		DrawDisc(Px.BodyFront[Owner] ? LayerId + 11 : LayerId + 6, Px.MoonPositions[Moon], 1.1f,
			FLinearColor(0.80f, 0.82f, 0.86f, 0.9f));
	}
	return LayerId + 11;
}
