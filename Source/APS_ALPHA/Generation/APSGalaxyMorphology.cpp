#include "APSGalaxyMorphology.h"

#include "APSHashStream.h"
#include "APS_ALPHA/Core/Enums/StarClusterComposition.h"
#include "APS_ALPHA/Core/Enums/StarClusterPopulation.h"
#include "HAL/IConsoleManager.h"

namespace APSGalaxyMorphology
{
namespace
{
	using APSHashStream::FStream;

	constexpr double TwoPi = UE_DOUBLE_TWO_PI;
	constexpr uint64 SaltDisk = 0x4756325f4449534bull; // GV2_DISK
	constexpr uint64 SaltKnot = 0x4756325f4b4e4f54ull; // GV2_KNOT
	constexpr uint64 SaltLobe = 0x4756325f4c4f4245ull; // GV2_LOBE
	constexpr uint64 SaltFilament = 0x4756325f46494c41ull; // GV2_FILA
	constexpr uint64 SaltAge = 0x4756325f4147455full; // GV2_AGE_

	// Tuned on an offline numpy port of these formulas (1800-point preview and 250k-point views,
	// top and edge). Sb, SBb and the warped peculiar disk route to the historic code instead.
	const FProfile ProfileTable[] = {
		// Spiral: bulge shrinks, arms open, the disk gets younger and clumpier from Sa to Sm.
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSa, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.30, .BulgeRadius = 0.27, .BulgeExponent = 0.55,
			.DiskExponent = 0.56, .DiskThickness = 0.020, .Arms = 2, .ArmStart = 0.10, .ArmTurns = 2.7,
			.ArmJitter = 0.20, .ArmShare = 0.50, .ArmClumps = 0.03, .Age = -0.6},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSab, .Form = EForm::Disk,
			.Halo = 0.17, .Bulge = 0.23, .BulgeRadius = 0.25, .BulgeExponent = 0.48,
			.DiskExponent = 0.53, .DiskThickness = 0.022, .Arms = 2, .ArmStart = 0.08, .ArmTurns = 2.5,
			.ArmJitter = 0.20, .ArmShare = 0.65, .ArmClumps = 0.06, .Age = -0.3},
		// The historic two-arm spiral (Rio's accepted look): Sd/E0/E4 all give two arms there.
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSb, .Form = EForm::Historic,
			.HistoricClass = EGalaxyClass::Sd},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSbc, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.12, .BulgeRadius = 0.19, .BulgeExponent = 0.42,
			.DiskThickness = 0.026, .Arms = 2, .ArmStart = 0.05, .ArmTurns = 1.95,
			.ArmJitter = 0.24, .ArmShare = 0.82, .ArmClumps = 0.15, .Age = 0.3},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSc, .Form = EForm::Disk,
			.Halo = 0.15, .Bulge = 0.08, .BulgeRadius = 0.16, .BulgeExponent = 0.40,
			.DiskThickness = 0.028, .Arms = 3, .ArmStart = 0.04, .ArmTurns = 1.65,
			.ArmJitter = 0.27, .ArmShare = 0.84, .ArmClumps = 0.22, .Age = 0.5},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralScd, .Form = EForm::Disk,
			.Halo = 0.13, .Bulge = 0.05, .BulgeRadius = 0.13, .BulgeExponent = 0.38,
			.DiskThickness = 0.030, .Arms = 3, .ArmStart = 0.03, .ArmTurns = 1.40,
			.ArmJitter = 0.33, .ArmShare = 0.80, .ArmClumps = 0.26, .Age = 0.6},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSd, .Form = EForm::Disk,
			.Halo = 0.11, .Bulge = 0.03, .BulgeRadius = 0.10, .BulgeExponent = 0.36,
			.DiskExponent = 0.48, .DiskThickness = 0.036, .Arms = 4, .ArmTurns = 1.15,
			.ArmJitter = 0.42, .ArmShare = 0.72, .ArmClumps = 0.30, .Age = 0.7},
		{.Type = EGalaxyType::Spiral, .Class = EGalaxyClass::SpiralSm, .Form = EForm::Disk,
			.Halo = 0.09, .Bulge = 0.01, .Bar = 0.10, .BulgeRadius = 0.08, .BulgeExponent = 0.36,
			.BarLength = 0.22, .BarWidth = 0.06, .BarAngle = 0.3, .OffsetX = 0.08, .OffsetY = -0.04,
			.DiskExtent = 0.88, .DiskExponent = 0.48, .DiskThickness = 0.045, .Arms = 2, .ArmTurns = 0.85,
			.ArmJitter = 0.48, .ArmShare = 0.65, .ArmLead = 0.7, .ArmClumps = 0.32, .Age = 0.8},

		// Barred spiral: arms leave the bar ends; the bar shortens and the arms open from SBa to SBm.
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBa, .Form = EForm::Disk,
			.Halo = 0.15, .Bulge = 0.24, .Bar = 0.30, .BulgeRadius = 0.22, .BulgeExponent = 0.55,
			.BarLength = 0.50, .BarWidth = 0.075, .DiskThickness = 0.020, .Arms = 2, .ArmStart = 0.50,
			.ArmTurns = 0.75, .ArmJitter = 0.16, .ArmShare = 0.55, .ArmClumps = 0.03, .InnerRing = 0.14,
			.Age = -0.6},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBab, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.20, .Bar = 0.28, .BulgeRadius = 0.21, .BulgeExponent = 0.50,
			.BarLength = 0.46, .BarWidth = 0.068, .DiskThickness = 0.022, .Arms = 2, .ArmStart = 0.46,
			.ArmTurns = 0.72, .ArmJitter = 0.18, .ArmShare = 0.65, .ArmClumps = 0.06, .InnerRing = 0.10,
			.Age = -0.3},
		// The historic barred spiral with two arms (SBd/E0/E4 give two arms there).
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBb, .Form = EForm::Historic,
			.HistoricClass = EGalaxyClass::SBd},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBbc, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.12, .Bar = 0.22, .BulgeRadius = 0.18, .BulgeExponent = 0.42,
			.BarLength = 0.40, .BarWidth = 0.060, .DiskThickness = 0.026, .Arms = 2, .ArmStart = 0.40,
			.ArmTurns = 0.68, .ArmJitter = 0.22, .ArmShare = 0.82, .ArmClumps = 0.14, .InnerRing = 0.04,
			.Age = 0.3},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBc, .Form = EForm::Disk,
			.Halo = 0.15, .Bulge = 0.08, .Bar = 0.20, .BulgeRadius = 0.15, .BulgeExponent = 0.40,
			.BarLength = 0.36, .BarWidth = 0.055, .DiskThickness = 0.028, .Arms = 2, .ArmStart = 0.36,
			.ArmTurns = 0.62, .ArmJitter = 0.26, .ArmShare = 0.84, .ArmClumps = 0.20, .Age = 0.5},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBcd, .Form = EForm::Disk,
			.Halo = 0.13, .Bulge = 0.05, .Bar = 0.17, .BulgeRadius = 0.12, .BulgeExponent = 0.38,
			.BarLength = 0.32, .BarWidth = 0.055, .DiskThickness = 0.030, .Arms = 2, .ArmStart = 0.32,
			.ArmTurns = 0.58, .ArmJitter = 0.32, .ArmShare = 0.80, .ArmClumps = 0.24, .Age = 0.6},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBd, .Form = EForm::Disk,
			.Halo = 0.11, .Bulge = 0.03, .Bar = 0.13, .BulgeRadius = 0.10, .BulgeExponent = 0.36,
			.BarLength = 0.28, .BarWidth = 0.060, .DiskExponent = 0.48, .DiskThickness = 0.036,
			.Arms = 2, .ArmStart = 0.28, .ArmTurns = 0.52, .ArmJitter = 0.42, .ArmShare = 0.72,
			.ArmClumps = 0.28, .Age = 0.7},
		{.Type = EGalaxyType::BarredSpiral, .Class = EGalaxyClass::BarredSBm, .Form = EForm::Disk,
			.Halo = 0.09, .Bulge = 0.01, .Bar = 0.18, .BulgeRadius = 0.08, .BulgeExponent = 0.36,
			.BarLength = 0.30, .BarWidth = 0.08, .BarAngle = 0.25, .OffsetX = 0.10, .OffsetY = -0.05,
			.DiskExtent = 0.86, .DiskExponent = 0.48, .DiskThickness = 0.045, .Arms = 2, .ArmStart = 0.30,
			.ArmTurns = 0.50, .ArmJitter = 0.46, .ArmShare = 0.66, .ArmLead = 0.75, .ArmClumps = 0.30,
			.Age = 0.8},

		// Lenticular: S0 stays historic; S0/a gains faint smooth arms, SB0 a bar. Old disks.
		{.Type = EGalaxyType::Lenticular, .Class = EGalaxyClass::S0a, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.20, .BulgeRadius = 0.23, .BulgeExponent = 0.5,
			.DiskThickness = 0.03, .ThickShare = 0.15, .ThickScale = 4.0, .Arms = 2, .ArmStart = 0.12,
			.ArmTurns = 1.9, .ArmJitter = 0.16, .ArmShare = 0.25, .Age = -0.5},
		{.Type = EGalaxyType::Lenticular, .Class = EGalaxyClass::SB0, .Form = EForm::Disk,
			.Halo = 0.16, .Bulge = 0.20, .Bar = 0.20, .BulgeRadius = 0.22, .BulgeExponent = 0.5,
			.BarLength = 0.38, .BarWidth = 0.06, .DiskThickness = 0.035, .ThickShare = 0.18,
			.ThickScale = 4.5, .Age = -0.7},

		// Elliptical: E0..E7 stay historic; cD has a bright core in a vast envelope, dE is a
		// diffuse flattened dwarf with a small nucleus.
		{.Type = EGalaxyType::Elliptical, .Class = EGalaxyClass::cD, .Form = EForm::Spheroid,
			.Core = 0.55, .CoreRadius = 0.42, .CoreExponent = 1.35, .EnvelopeInner = 0.18,
			.EnvelopeOuter = 0.98, .EnvelopeExponent = 0.75, .AxisY = 0.92, .AxisZ = 0.85, .Age = -0.9},
		{.Type = EGalaxyType::Elliptical, .Class = EGalaxyClass::dE, .Form = EForm::Spheroid,
			.EnvelopeOuter = 0.80, .EnvelopeExponent = 0.60, .AxisY = 0.82, .AxisZ = 0.62,
			.Nucleus = 0.08, .Age = -0.7},

		// Irregular: Irr stays historic.
		{.Type = EGalaxyType::Irregular, .Class = EGalaxyClass::Im, .Form = EForm::Irregular,
			.Halo = 0.06, .Bar = 0.28, .Stub = 0.14, .HaloExtent = 0.75, .BarLength = 0.30, .BarWidth = 0.07,
			.BarAngle = 0.35, .OffsetX = 0.10, .OffsetY = -0.05, .Lobes = 3, .LobeSpread = 0.34,
			.LobeRadiusMin = 0.18, .LobeRadiusMax = 0.30, .StubTurns = 0.30, .StubReach = 0.38,
			.Clumps = 0.14, .Age = 0.7},
		{.Type = EGalaxyType::Irregular, .Class = EGalaxyClass::IBm, .Form = EForm::Irregular,
			.Halo = 0.06, .Bar = 0.38, .HaloExtent = 0.75, .BarLength = 0.40, .BarWidth = 0.085,
			.OffsetX = 0.03, .Lobes = 2, .LobeSpread = 0.30, .LobeRadiusMin = 0.20, .LobeRadiusMax = 0.28,
			.Clumps = 0.12, .Age = 0.6},
		{.Type = EGalaxyType::Irregular, .Class = EGalaxyClass::dIrr, .Form = EForm::Irregular,
			.Halo = 0.06, .HaloExtent = 0.70, .Lobes = 4, .LobeSpread = 0.30, .LobeRadiusMin = 0.14,
			.LobeRadiusMax = 0.26, .Clumps = 0.10, .Age = 0.4},
		{.Type = EGalaxyType::Irregular, .Class = EGalaxyClass::I0, .Form = EForm::Starburst, .Age = 0.6},

		// Peculiar: the default keeps the historic warped disk (it ignores the class).
		{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecWarped, .Form = EForm::Historic,
			.HistoricClass = EGalaxyClass::Irr},
		{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecRing, .Form = EForm::Ring, .Age = 0.9},
		{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecInteracting, .Form = EForm::Interacting,
			.Age = 0.5},
		{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecTidalTails, .Form = EForm::TidalTails,
			.Age = 0.4},
		{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecPolarRing, .Form = EForm::PolarRing,
			.Age = 0.8},
	};

	// The primary disk of an interacting pair is a small spiral of its own.
	const FProfile InteractingPrimary{.Type = EGalaxyType::Peculiar, .Class = EGalaxyClass::PecInteracting,
		.Form = EForm::Disk, .Bulge = 0.16, .BulgeRadius = 0.10, .BulgeExponent = 0.55, .DiskExtent = 0.40,
		.DiskThickness = 0.02, .Arms = 2, .ArmTurns = 0.95, .ArmJitter = 0.22, .ArmShare = 0.85,
		.ArmClumps = 0.15, .Age = 0.5};

	FVector RotateX(const FVector& Value, const double Angle)
	{
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return FVector(Value.X, Value.Y * C - Value.Z * S, Value.Y * S + Value.Z * C);
	}

	FVector RotateY(const FVector& Value, const double Angle)
	{
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return FVector(Value.X * C + Value.Z * S, Value.Y, -Value.X * S + Value.Z * C);
	}

	FVector RotateZ(const FVector& Value, const double Angle)
	{
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return FVector(Value.X * C - Value.Y * S, Value.X * S + Value.Y * C, Value.Z);
	}

	FVector Bezier(const FVector& A, const FVector& B, const FVector& C, const double T)
	{
		const double OneMinusT = 1.0 - T;
		return A * (OneMinusT * OneMinusT) + B * (2.0 * OneMinusT * T) + C * (T * T);
	}

	FVector Bell3(FStream& Stream, const FVector& Scale)
	{
		const double X = Stream.Bell();
		const double Y = Stream.Bell();
		const double Z = Stream.Bell();
		return FVector(X, Y, Z) / 1.5 * Scale;
	}

	/** Stellar halo: the historic radial law U^0.55, mildly flattened, soft rim. */
	FVector SampleHalo(FStream& Stream, const double Extent, const double Flattening)
	{
		FVector Direction = Stream.UnitVector();
		Direction.Z *= Flattening;
		const double Radius = FMath::Pow(Stream.U(), 0.55) * Extent;
		return Direction * (Radius * (1.0 - 0.12 * FMath::Square(Stream.U())));
	}

	FVector SampleBulge(FStream& Stream, const double Radius, const double Exponent, const double Flattening)
	{
		FVector Direction = Stream.UnitVector();
		Direction.Z *= Flattening;
		const double Distance = Radius * FMath::Pow(Stream.U(), Exponent);
		return Direction * (Distance * (0.82 + 0.36 * Stream.U()));
	}

	FVector SampleBar(FStream& Stream, const double Length, const double Width, const double Angle)
	{
		const double SafeLength = FMath::Max(Length, 1.0e-6);
		const double X = FMath::Clamp(Stream.Bell() / 1.2, -1.0, 1.0) * SafeLength;
		const double LocalWidth = Width * (1.0 - 0.45 * FMath::Square(X / SafeLength));
		const double Y = Stream.Bell() / 1.5 * LocalWidth;
		const double Z = Stream.Bell() / 1.5 * LocalWidth * 0.7;
		return RotateZ(FVector(X, Y, Z), Angle);
	}

	/** Truncated Plummer radius (scale A, cut at Cut) from one draw. */
	double PlummerRadius(FStream& Stream, const double A, const double Cut)
	{
		const double MaxFraction = FMath::Pow(Cut * Cut / (Cut * Cut + A * A), 1.5);
		const double Fraction = FMath::Max(Stream.U() * MaxFraction, 1.0e-12);
		return A / FMath::Sqrt(FMath::Max(FMath::Pow(Fraction, -2.0 / 3.0) - 1.0, 1.0e-12));
	}

	/** Archimedean arm angle as in the historic spiral, measured from ArmStart. */
	double ArmAngle(const FProfile& Profile, const int32 Arm, const double Rho)
	{
		const double Span = FMath::Max(Profile.DiskExtent - Profile.ArmStart, 1.0e-6);
		const double T = FMath::Clamp((Rho - Profile.ArmStart) / Span, 0.0, 1.0);
		return TwoPi * Arm / FMath::Max(Profile.Arms, 1) + T * TwoPi * Profile.ArmTurns;
	}

	FSample SampleDisk(const FProfile& P, FStream& Stream, const double Selector, const int32 Seed)
	{
		FSample Out;
		const FVector Offset(P.OffsetX, P.OffsetY, 0.0);
		const double HaloEnd = P.Halo;
		const double BulgeEnd = HaloEnd + P.Bulge;
		const double BarEnd = BulgeEnd + P.Bar;
		if (Selector < HaloEnd)
		{
			Out.Position = SampleHalo(Stream, P.HaloExtent, P.HaloFlattening);
			Out.Age = -0.8f;
			return Out;
		}
		if (Selector < BulgeEnd)
		{
			Out.Position = SampleBulge(Stream, P.BulgeRadius, P.BulgeExponent, P.BulgeFlattening) + Offset * 0.5;
			Out.Age = -0.6f;
			return Out;
		}
		if (Selector < BarEnd)
		{
			Out.Position = SampleBar(Stream, P.BarLength, P.BarWidth, P.BarAngle) + Offset;
			Out.Age = -0.2f;
			return Out;
		}

		// Soft rim: the outer tenth thins out instead of ending on a hard circle.
		const double RadialDraw = Stream.U();
		double Rho = P.DiskExtent * FMath::Pow(RadialDraw, P.DiskExponent)
			* (1.0 - 0.15 * FMath::Square(Stream.U()));
		double Theta = Stream.U() * TwoPi;
		double Age = P.Age * 0.5;
		if (P.InnerRing > 0.0 && Stream.U() < P.InnerRing)
		{
			Rho = P.ArmStart * 1.04 + Stream.Bell() / 1.5 * 0.025;
		}
		else if (P.Arms > 0 && Rho >= P.ArmStart && Stream.U() < P.ArmShare)
		{
			const int32 Arm = Stream.U() < P.ArmLead ? 0 : Stream.Index(P.Arms);
			// Bell-shaped arm cross-section (same sigma as the historic uniform jitter).
			Theta = ArmAngle(P, Arm, Rho) + Stream.Bell() * P.ArmJitter * 1.15 * (0.35 + Rho);
			Age = P.Age;
			if (P.ArmClumps > 0.0 && Stream.U() < P.ArmClumps)
			{
				// Star-forming knots: fixed per galaxy seed, shared by every LOD.
				const int32 KnotCount = FMath::Max(P.KnotsPerArm, 1) * P.Arms;
				const int32 Knot = Stream.Index(KnotCount);
				FStream KnotStream(APSHashStream::Key(Seed, Knot, SaltKnot));
				const double KnotRho = P.ArmStart
					+ (P.DiskExtent - P.ArmStart) * (0.12 + 0.84 * KnotStream.U());
				const double KnotTheta = ArmAngle(P, Knot % P.Arms, KnotRho)
					+ KnotStream.Range(-0.12, 0.12) * (0.35 + KnotRho);
				const double X = FMath::Cos(KnotTheta) * KnotRho + Stream.Bell() / 1.5 * 0.022;
				const double Y = FMath::Sin(KnotTheta) * KnotRho + Stream.Bell() / 1.5 * 0.022;
				Rho = FMath::Sqrt(X * X + Y * Y);
				Theta = FMath::Atan2(Y, X);
				Age = 1.0;
			}
		}
		double Height = P.DiskThickness * (1.15 - FMath::Min(Rho, 1.0));
		if (Stream.U() < P.ThickShare)
		{
			Height *= P.ThickScale;
		}
		Out.Position = FVector(FMath::Cos(Theta) * Rho, FMath::Sin(Theta) * Rho,
			Stream.Bell() * 1.15 * Height) + Offset;
		Out.Age = static_cast<float>(Age);
		return Out;
	}

	FSample SampleSpheroid(const FProfile& P, FStream& Stream, const double Selector)
	{
		FVector Position;
		if (Selector < P.Core)
		{
			const FVector Direction = Stream.UnitVector();
			Position = Direction * (P.CoreRadius * FMath::Pow(Stream.U(), P.CoreExponent));
		}
		else
		{
			const double Radius = P.EnvelopeInner
				+ (P.EnvelopeOuter - P.EnvelopeInner) * FMath::Pow(Stream.U(), P.EnvelopeExponent);
			const FVector Direction = Stream.UnitVector();
			Position = Direction * (Radius * (1.0 - 0.1 * FMath::Square(Stream.U())));
		}
		if (P.Nucleus > 0.0 && Stream.U() < P.Nucleus)
		{
			const FVector Direction = Stream.UnitVector();
			Position = Direction * (FMath::Pow(Stream.U(), 0.6) * 0.045);
		}
		Position.Y *= P.AxisY;
		Position.Z *= P.AxisZ;
		return {Position, static_cast<float>(P.Age)};
	}

	FSample SampleIrregular(const FProfile& P, FStream& Stream, const double Selector, const int32 Seed)
	{
		const FVector Offset(P.OffsetX, P.OffsetY, 0.0);
		const double HaloEnd = P.Halo;
		const double BarEnd = HaloEnd + P.Bar;
		const double StubEnd = BarEnd + P.Stub;
		if (Selector < HaloEnd)
		{
			return {SampleHalo(Stream, P.HaloExtent, 1.0), -0.6f};
		}
		if (Selector < BarEnd)
		{
			return {SampleBar(Stream, P.BarLength, P.BarWidth, P.BarAngle) + Offset, -0.1f};
		}
		if (Selector < StubEnd)
		{
			// One stubby arm leaving the bar end, as in the Magellanic Clouds.
			const double T = FMath::Pow(Stream.U(), 0.8);
			const double Angle = P.BarAngle + T * TwoPi * P.StubTurns;
			const double Radius = P.BarLength + T * P.StubReach;
			const double Spread = 0.03 + 0.05 * T;
			const double X = FMath::Cos(Angle) * Radius + Stream.Bell() / 1.5 * Spread;
			const double Y = FMath::Sin(Angle) * Radius + Stream.Bell() / 1.5 * Spread;
			const double Z = Stream.Bell() / 1.5 * 0.03;
			return {FVector(X, Y, Z) + Offset, static_cast<float>(P.Age)};
		}
		if (P.Clumps > 0.0 && Stream.U() < P.Clumps)
		{
			FStream KnotStream(APSHashStream::Key(Seed, Stream.Index(10), SaltKnot));
			const FVector KnotDirection = KnotStream.UnitVector();
			FVector Center = KnotDirection * (KnotStream.U() * P.LobeSpread * 1.1);
			Center.Z *= 0.4;
			return {Center + Offset + Bell3(Stream, FVector(0.025)), 1.0f};
		}
		FStream LobeStream(APSHashStream::Key(Seed, Stream.Index(FMath::Max(P.Lobes, 1)), SaltLobe));
		const FVector LobeDirection = LobeStream.UnitVector();
		FVector Center = LobeDirection * (LobeStream.U() * P.LobeSpread);
		Center.Z *= 0.5;
		const double LobeRadius = FMath::Lerp(P.LobeRadiusMin, P.LobeRadiusMax, LobeStream.U());
		return {Center + Offset + Bell3(Stream, FVector(LobeRadius, LobeRadius, LobeRadius * 0.5)),
			static_cast<float>(P.Age)};
	}

	/** I0 / M82-like: a dusty cigar body, a starburst core and a bipolar filamentary outflow. */
	FSample SampleStarburst(FStream& Stream, const double Selector, const int32 Seed)
	{
		if (Selector < 0.55)
		{
			return {Bell3(Stream, FVector(0.68, 0.24, 0.10)), 0.1f};
		}
		if (Selector < 0.70)
		{
			const FVector Direction = Stream.UnitVector();
			return {Direction * (FMath::Pow(Stream.U(), 0.7) * 0.07), 1.0f};
		}
		const double Reach = FMath::Pow(Stream.U(), 0.8) * 0.70;
		const double Height = Stream.U() < 0.5 ? Reach : -Reach;
		FStream FilamentStream(APSHashStream::Key(Seed, Stream.Index(9), SaltFilament));
		const double Angle = FilamentStream.U() * TwoPi + Stream.Bell() / 1.5 * 0.35;
		const double Lateral = (0.03 + 0.30 * FMath::Abs(Height)) * FMath::Sqrt(Stream.U());
		return {FVector(FMath::Cos(Angle) * Lateral, FMath::Sin(Angle) * Lateral, Height), 0.6f};
	}

	/** Cartwheel-like: old core, a young knotty ring, faint curved spokes and a halo. */
	FSample SampleRing(FStream& Stream, const double Selector, const int32 Seed)
	{
		if (Selector < 0.18)
		{
			FVector Direction = Stream.UnitVector();
			Direction.Z *= 0.8;
			return {Direction * (FMath::Pow(Stream.U(), 0.65) * 0.11), -0.7f};
		}
		if (Selector < 0.70)
		{
			double Theta = Stream.U() * TwoPi;
			if (Stream.U() < 0.30)
			{
				FStream KnotStream(APSHashStream::Key(Seed, Stream.Index(16), SaltKnot));
				Theta = KnotStream.U() * TwoPi + Stream.Bell() / 1.5 * 0.05;
			}
			const double Radius = 0.70 + Stream.Bell() / 1.5 * 0.035 + 0.035 * FMath::Cos(Theta - 0.7);
			return {FVector(FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius,
				Stream.Bell() / 1.5 * 0.014), 0.9f};
		}
		if (Selector < 0.82)
		{
			const int32 Spoke = Stream.Index(9);
			const double Radius = Stream.Range(0.14, 0.64);
			const double Theta = TwoPi * Spoke / 9.0 + 0.15 + Stream.Bell() / 1.5 * 0.05 + Radius * 0.6;
			return {FVector(FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius,
				Stream.Bell() / 1.5 * 0.01), 0.2f};
		}
		return {SampleHalo(Stream, 0.92, 0.85), -0.8f};
	}

	/** A spiral and its compact companion joined by a bridge, with a long counter-tail. */
	FSample SampleInteracting(FStream& Stream, const double Selector, const int32 Seed)
	{
		if (Selector < 0.52)
		{
			FSample Primary = SampleDisk(InteractingPrimary, Stream, Selector / 0.52, Seed);
			Primary.Position = RotateX(Primary.Position, 0.45) + FVector(-0.30, -0.08, 0.0);
			return Primary;
		}
		if (Selector < 0.70)
		{
			FVector Direction = Stream.UnitVector();
			Direction.Z *= 0.55;
			const FVector Local = Direction * (FMath::Pow(Stream.U(), 0.8) * 0.17);
			return {RotateY(Local, 1.0) + FVector(0.52, 0.22, 0.04), -0.3f};
		}
		if (Selector < 0.82)
		{
			const FVector Point = Bezier(FVector(0.06, 0.02, 0.0), FVector(0.30, 0.34, 0.06),
				FVector(0.46, 0.20, 0.04), Stream.U());
			return {Point + Bell3(Stream, FVector(0.03, 0.03, 0.0165)), 0.8f};
		}
		if (Selector < 0.94)
		{
			const double T = FMath::Pow(Stream.U(), 0.8);
			const FVector Point = Bezier(FVector(-0.62, -0.20, 0.0), FVector(-0.95, 0.10, -0.06),
				FVector(-0.70, 0.55, -0.10), T);
			return {Point + Bell3(Stream, FVector(0.02 + 0.05 * T)), 0.6f};
		}
		return {SampleHalo(Stream, 0.90, 1.0), -0.8f};
	}

	/** Antennae-like merger: two disturbed cores, young knots and two long curved tails. */
	FSample SampleTidalTails(FStream& Stream, const double Selector, const int32 Seed)
	{
		if (Selector < 0.34)
		{
			const bool bFirst = Stream.U() < 0.5;
			FVector Direction = Stream.UnitVector();
			Direction.Z *= 0.6;
			const FVector Local = Direction * (FMath::Pow(Stream.U(), 0.7) * 0.10);
			return {bFirst ? RotateX(Local, 0.6) + FVector(0.08, -0.035, 0.0)
				: RotateY(Local, -0.8) + FVector(-0.08, 0.04, 0.0), -0.2f};
		}
		if (Selector < 0.48)
		{
			FStream KnotStream(APSHashStream::Key(Seed, Stream.Index(14), SaltKnot));
			const double KnotX = KnotStream.Range(-1.0, 1.0) * 0.16;
			const double KnotY = KnotStream.Range(-1.0, 1.0) * 0.12;
			const double KnotZ = KnotStream.Range(-1.0, 1.0) * 0.04;
			return {FVector(KnotX, KnotY, KnotZ) + Bell3(Stream, FVector(0.018)), 1.0f};
		}
		if (Selector < 0.90)
		{
			const bool bFirst = Stream.U() < 0.5;
			const double T = FMath::Pow(Stream.U(), 0.75);
			const double Radius = 0.12 + 0.78 * T;
			const double Phi = 0.4 + (bFirst ? 0.0 : UE_DOUBLE_PI) + 2.3 * T;
			const double Sigma = (0.012 + 0.04 * T) * 1.6;
			const FVector Point(FMath::Cos(Phi) * Radius * (bFirst ? 1.0 : 0.92),
				FMath::Sin(Phi) * Radius * 0.62, (bFirst ? 1.0 : -1.0) * 0.14 * FMath::Pow(T, 1.5));
			return {Point + Bell3(Stream, FVector(Sigma)), 0.4f};
		}
		return {SampleHalo(Stream, 0.85, 1.0), -0.8f};
	}

	/** An old lenticular host crossed by a young, slightly warped polar ring. */
	FSample SamplePolarRing(FStream& Stream, const double Selector)
	{
		if (Selector < 0.42)
		{
			const double RadialDraw = Stream.U();
			const double Radius = 0.42 * FMath::Sqrt(RadialDraw) * (1.0 - 0.2 * FMath::Square(Stream.U()));
			const double Theta = Stream.U() * TwoPi;
			const double Height = Stream.Bell() * 1.15 * 0.03 * (1.1 - Radius / 0.42 * 0.6);
			return {FVector(FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius, Height), -0.7f};
		}
		if (Selector < 0.58)
		{
			return {SampleBulge(Stream, 0.12, 0.5, 0.75), -0.8f};
		}
		if (Selector < 0.90)
		{
			const double Theta = Stream.U() * TwoPi;
			const double Radius = 0.62 + Stream.Bell() / 1.5 * 0.045;
			const FVector Local(Stream.Bell() / 1.5 * 0.03 + 0.015 * FMath::Sin(2.0 * Theta),
				FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius);
			return {RotateZ(Local, 0.18), 0.8f};
		}
		return {SampleHalo(Stream, 0.90, 1.0), -0.8f};
	}
}

TConstArrayView<EGalaxyClass> GetSubclasses(const EGalaxyType Type)
{
	static const EGalaxyClass Elliptical[] = {
		EGalaxyClass::E0, EGalaxyClass::E1, EGalaxyClass::E2, EGalaxyClass::E3, EGalaxyClass::E4,
		EGalaxyClass::E5, EGalaxyClass::E6, EGalaxyClass::E7, EGalaxyClass::cD, EGalaxyClass::dE};
	static const EGalaxyClass Lenticular[] = {EGalaxyClass::S0, EGalaxyClass::S0a, EGalaxyClass::SB0};
	static const EGalaxyClass Spiral[] = {
		EGalaxyClass::SpiralSa, EGalaxyClass::SpiralSab, EGalaxyClass::SpiralSb, EGalaxyClass::SpiralSbc,
		EGalaxyClass::SpiralSc, EGalaxyClass::SpiralScd, EGalaxyClass::SpiralSd, EGalaxyClass::SpiralSm};
	static const EGalaxyClass Barred[] = {
		EGalaxyClass::BarredSBa, EGalaxyClass::BarredSBab, EGalaxyClass::BarredSBb, EGalaxyClass::BarredSBbc,
		EGalaxyClass::BarredSBc, EGalaxyClass::BarredSBcd, EGalaxyClass::BarredSBd, EGalaxyClass::BarredSBm};
	static const EGalaxyClass Irregular[] = {
		EGalaxyClass::Irr, EGalaxyClass::Im, EGalaxyClass::IBm, EGalaxyClass::dIrr, EGalaxyClass::I0};
	static const EGalaxyClass Peculiar[] = {
		EGalaxyClass::PecWarped, EGalaxyClass::PecRing, EGalaxyClass::PecInteracting,
		EGalaxyClass::PecTidalTails, EGalaxyClass::PecPolarRing};
	switch (Type)
	{
	case EGalaxyType::Elliptical: return Elliptical;
	case EGalaxyType::Lenticular: return Lenticular;
	case EGalaxyType::Spiral: return Spiral;
	case EGalaxyType::BarredSpiral: return Barred;
	case EGalaxyType::Irregular: return Irregular;
	case EGalaxyType::Peculiar: return Peculiar;
	default: return {};
	}
}

EGalaxyClass GetDefaultSubclass(const EGalaxyType Type)
{
	switch (Type)
	{
	case EGalaxyType::Elliptical: return EGalaxyClass::E0;
	case EGalaxyType::Lenticular: return EGalaxyClass::S0;
	case EGalaxyType::Spiral: return EGalaxyClass::SpiralSb;
	case EGalaxyType::BarredSpiral: return EGalaxyClass::BarredSBb;
	case EGalaxyType::Irregular: return EGalaxyClass::Irr;
	case EGalaxyType::Peculiar: return EGalaxyClass::PecWarped;
	default: return EGalaxyClass::Unknown;
	}
}

bool IsSubclassOf(const EGalaxyType Type, const EGalaxyClass Class)
{
	return GetSubclasses(Type).Contains(Class);
}

EGalaxyClass CoerceSubclass(const EGalaxyType Type, const EGalaxyClass Class)
{
	return IsSubclassOf(Type, Class) ? Class : GetDefaultSubclass(Type);
}

TConstArrayView<FProfile> GetProfiles()
{
	return ProfileTable;
}

const FProfile* FindProfile(const EGalaxyType Type, const EGalaxyClass Class)
{
	static const TArray<const FProfile*> ByClass = []()
	{
		TArray<const FProfile*> Table;
		Table.Init(nullptr, 256);
		for (const FProfile& Profile : ProfileTable)
		{
			Table[static_cast<uint8>(Profile.Class)] = &Profile;
		}
		return Table;
	}();
	const FProfile* Profile = ByClass[static_cast<uint8>(Class)];
	return Profile && Profile->Type == Type ? Profile : nullptr;
}

FSample Sample(const FProfile& Profile, const int32 Seed, const int64 CatalogIndex,
	const uint64 RecordHash, const double GalaxyRadius)
{
	// The historic population selector: switching between subclasses with similar shares keeps
	// the same stars in the halo and bulge, so neighbouring subclasses morph instead of reshuffle.
	const double Selector = static_cast<double>((RecordHash >> 24) & 0xffffull) / 65535.0;
	FStream Stream(APSHashStream::Key(Seed, CatalogIndex, SaltDisk));
	FSample Out;
	switch (Profile.Form)
	{
	case EForm::Disk: Out = SampleDisk(Profile, Stream, Selector, Seed); break;
	case EForm::Spheroid: Out = SampleSpheroid(Profile, Stream, Selector); break;
	case EForm::Irregular: Out = SampleIrregular(Profile, Stream, Selector, Seed); break;
	case EForm::Starburst: Out = SampleStarburst(Stream, Selector, Seed); break;
	case EForm::Ring: Out = SampleRing(Stream, Selector, Seed); break;
	case EForm::Interacting: Out = SampleInteracting(Stream, Selector, Seed); break;
	case EForm::TidalTails: Out = SampleTidalTails(Stream, Selector, Seed); break;
	case EForm::PolarRing: Out = SamplePolarRing(Stream, Selector); break;
	case EForm::Historic:
	default: break;
	}
	// The catalogue envelope (and the canonical projection frame) is 1.05 radii; keep every
	// parametric star inside one radius even for the rare all-tails-aligned bell draw.
	const double Size = Out.Position.Size();
	if (Size > 1.0)
	{
		Out.Position *= 1.0 / Size;
	}
	Out.Position *= GalaxyRadius;
	return Out;
}

ESpectralClass ApplyPopulationAge(const ESpectralClass Base, const float Age, const uint64 RecordHash)
{
	if (Age == 0.0f)
	{
		return Base;
	}
	FStream Stream(APSHashStream::Mix64(RecordHash ^ SaltAge));
	const double Draw = Stream.U();
	if (Age > 0.0f)
	{
		// Arms, knots and rings are lit by young massive stars.
		if (Draw < 0.22 * Age)
		{
			const double Pick = Stream.U();
			return Pick < 0.06 ? ESpectralClass::O : Pick < 0.42 ? ESpectralClass::B : ESpectralClass::A;
		}
		return Base;
	}
	// Old bulges, halos and ellipticals have lost their hot massive stars and protostars.
	const bool bYoungStar = Base == ESpectralClass::O || Base == ESpectralClass::B
		|| Base == ESpectralClass::A || Base == ESpectralClass::PS;
	if (bYoungStar && Draw < -Age)
	{
		return Stream.U() < 0.55 ? ESpectralClass::K : ESpectralClass::M;
	}
	return Base;
}

namespace
{
	TAutoConsoleVariable<int32> CVarGalaxyStarMix(
		TEXT("aps.Galaxy.StarMix"), 1,
		TEXT("Rio 03.10: 1 applies the galaxy POPULATION / COMPOSITION rows to the catalogue; 0 resolves the historic mix "
			"(regenerate to see it)."));

	constexpr uint64 SaltMix = 0x4756325f4d49585full; // GV2_MIX_

	/** The cluster's stellar types (UStarGenerator::StarClusterPopulationWeights) as galaxy size classes. */
	enum class EMixType : uint8
	{
		Main, SubGiant, Giant, BrightGiant, SuperGiant, HyperGiant,
		SubDwarf, WhiteDwarf, BrownDwarf, Protostar, Neutron, Pulsar, BlackHole
	};

	template <typename T>
	struct TMixWeight
	{
		T Key;
		int32 Weight;
	};

	template <typename T>
	T PickWeighted(const std::initializer_list<TMixWeight<T>> Table, const double Draw)
	{
		int32 Total = 0;
		for (const TMixWeight<T>& Entry : Table) Total += Entry.Weight;
		double Remaining = Draw * Total;
		for (const TMixWeight<T>& Entry : Table)
		{
			Remaining -= Entry.Weight;
			if (Remaining < 0.0) return Entry.Key;
		}
		return (Table.end() - 1)->Key;
	}

	/** The cluster presets' weights (Giants, Dwarfs, Protostars); false for All Sequences, which keeps the catalogue. */
	bool DrawPopulation(const uint8 Population, const double Draw, EMixType& OutType)
	{
		using EP = EStarClusterPopulation;
		switch (static_cast<EP>(Population))
		{
		case EP::MainSequence: OutType = EMixType::Main; return true;
		case EP::Giants:
			OutType = PickWeighted<EMixType>({{EMixType::HyperGiant, 1}, {EMixType::SuperGiant, 5},
				{EMixType::BrightGiant, 10}, {EMixType::Giant, 50}, {EMixType::SubGiant, 30}}, Draw);
			return true;
		case EP::Dwarfs:
			OutType = PickWeighted<EMixType>({{EMixType::SubDwarf, 50}, {EMixType::WhiteDwarf, 12},
				{EMixType::BrownDwarf, 5}, {EMixType::Protostar, 5}, {EMixType::Neutron, 3}, {EMixType::Pulsar, 2},
				{EMixType::BlackHole, 1}}, Draw);
			return true;
		case EP::Protostars:
			OutType = PickWeighted<EMixType>({{EMixType::BrownDwarf, 15}, {EMixType::Protostar, 50},
				{EMixType::Neutron, 10}, {EMixType::Pulsar, 10}}, Draw);
			return true;
		default:
			return false;
		}
	}

	/** Radius factor against the record's canonical class radius (remnants and brown dwarfs take their own class). */
	float GetMixRadiusScale(const EMixType Type)
	{
		switch (Type)
		{
		case EMixType::SubGiant: return 2.0f;
		case EMixType::Giant: return 8.0f;
		case EMixType::BrightGiant: return 20.0f;
		case EMixType::SuperGiant: return 50.0f;
		case EMixType::HyperGiant: return 100.0f;
		case EMixType::SubDwarf: return 0.6f;
		case EMixType::WhiteDwarf: return 0.006f;
		case EMixType::Protostar: return 2.5f;
		default: return 1.0f;
		}
	}

	/** The cluster's COMPOSITION tables (UStarGenerator::StarClusterCompositionWeights); false for All Spectral. */
	bool DrawComposition(const uint8 Composition, const double Draw, ESpectralClass& OutClass)
	{
		using EC = EStarClusterComposition;
		using ES = ESpectralClass;
		switch (static_cast<EC>(Composition))
		{
		case EC::OnlyBlue: OutClass = PickWeighted<ES>({{ES::O, 90}, {ES::B, 10}}, Draw); return true;
		case EC::MostlyBlue:
			OutClass = PickWeighted<ES>({{ES::O, 50}, {ES::B, 50}, {ES::A, 5}, {ES::F, 5}, {ES::G, 3}, {ES::K, 2},
				{ES::M, 1}}, Draw);
			return true;
		case EC::BlueWhite: OutClass = PickWeighted<ES>({{ES::O, 50}, {ES::B, 50}, {ES::A, 50}, {ES::F, 50}}, Draw); return true;
		case EC::OnlyWhite: OutClass = PickWeighted<ES>({{ES::A, 50}, {ES::F, 50}}, Draw); return true;
		case EC::MostlyWhite:
			OutClass = PickWeighted<ES>({{ES::A, 50}, {ES::F, 50}, {ES::O, 5}, {ES::B, 5}, {ES::G, 3}, {ES::K, 2},
				{ES::M, 1}}, Draw);
			return true;
		case EC::WhiteYellow: OutClass = PickWeighted<ES>({{ES::A, 50}, {ES::F, 50}, {ES::G, 50}}, Draw); return true;
		case EC::OnlyYellow: OutClass = PickWeighted<ES>({{ES::F, 5}, {ES::G, 90}, {ES::K, 5}}, Draw); return true;
		case EC::MostlyYellow:
			OutClass = PickWeighted<ES>({{ES::A, 5}, {ES::O, 5}, {ES::B, 5}, {ES::F, 10}, {ES::G, 80}, {ES::K, 10},
				{ES::M, 5}}, Draw);
			return true;
		case EC::YellowOrange: OutClass = PickWeighted<ES>({{ES::G, 50}, {ES::K, 50}}, Draw); return true;
		case EC::OnlyOrange: OutClass = ES::K; return true;
		case EC::MostlyOrange: OutClass = PickWeighted<ES>({{ES::G, 5}, {ES::K, 90}, {ES::M, 5}}, Draw); return true;
		case EC::OrangeRed: OutClass = PickWeighted<ES>({{ES::K, 50}, {ES::M, 50}}, Draw); return true;
		case EC::OnlyRed: OutClass = ES::M; return true;
		case EC::MostlyRed: OutClass = PickWeighted<ES>({{ES::G, 5}, {ES::K, 10}, {ES::M, 85}}, Draw); return true;
		default: return false;
		}
	}
}

void ApplyStarMix(const uint8 Population, const uint8 Composition, const uint64 RecordHash,
	ESpectralClass& InOutSpectralClass, float& OutRadiusScale)
{
	OutRadiusScale = 1.0f;
	if ((Population == 0 && Composition == 0) || CVarGalaxyStarMix.GetValueOnAnyThread() == 0) return;
	FStream Stream(APSHashStream::Mix64(RecordHash ^ SaltMix));
	// Three draws, always taken in this order (each its own statement), so either row alone keeps the other's draw.
	const double TypeDraw = Stream.U();
	const double ExoticDraw = Stream.U();
	const double ColourDraw = Stream.U();
	EMixType Type = EMixType::Main;
	const bool bPopulation = DrawPopulation(Population, TypeDraw, Type);
	bool bOwnClass = false;
	if (bPopulation)
	{
		OutRadiusScale = GetMixRadiusScale(Type);
		bOwnClass = true;
		switch (Type)
		{
		case EMixType::BrownDwarf:
			InOutSpectralClass = ExoticDraw < 0.6 ? ESpectralClass::L : ExoticDraw < 0.9 ? ESpectralClass::T : ESpectralClass::Y;
			break;
		case EMixType::WhiteDwarf:
			InOutSpectralClass = ExoticDraw < 0.7 ? ESpectralClass::A : ExoticDraw < 0.9 ? ESpectralClass::B : ESpectralClass::F;
			break;
		case EMixType::Neutron: InOutSpectralClass = ESpectralClass::NS; break;
		case EMixType::Pulsar: InOutSpectralClass = ESpectralClass::PS; break;
		case EMixType::BlackHole: InOutSpectralClass = ESpectralClass::BH; break;
		default: bOwnClass = false; break;
		}
	}
	ESpectralClass Colour = InOutSpectralClass;
	if (!bOwnClass && DrawComposition(Composition, ColourDraw, Colour)) InOutSpectralClass = Colour;
}

const TCHAR* GetSubclassSummary(const EGalaxyClass Class)
{
	switch (Class)
	{
	case EGalaxyClass::E0: return TEXT("Round elliptical.");
	case EGalaxyClass::E1: case EGalaxyClass::E2: case EGalaxyClass::E3:
		return TEXT("Elliptical, slightly elongated.");
	case EGalaxyClass::E4: case EGalaxyClass::E5: case EGalaxyClass::E6:
		return TEXT("Elliptical, elongated.");
	case EGalaxyClass::E7: return TEXT("Most elongated elliptical.");
	case EGalaxyClass::cD: return TEXT("Supergiant elliptical: bright core inside a vast old envelope.");
	case EGalaxyClass::dE: return TEXT("Dwarf elliptical: diffuse flattened body with a small nucleus.");
	case EGalaxyClass::S0: return TEXT("Lenticular: disk and bulge without arms.");
	case EGalaxyClass::S0a: return TEXT("Lenticular with faint, smooth arm traces.");
	case EGalaxyClass::SB0: return TEXT("Barred lenticular: an old disk with a bar.");
	case EGalaxyClass::SpiralSa: return TEXT("Large bulge, tightly wound smooth arms, old stars.");
	case EGalaxyClass::SpiralSab: return TEXT("Big bulge, tight arms.");
	case EGalaxyClass::SpiralSb: return TEXT("Classic two-armed spiral.");
	case EGalaxyClass::SpiralSbc: return TEXT("Smaller bulge, opening arms with young knots.");
	case EGalaxyClass::SpiralSc: return TEXT("Small bulge, three open knotty arms.");
	case EGalaxyClass::SpiralScd: return TEXT("Tiny bulge, loose bright arms.");
	case EGalaxyClass::SpiralSd: return TEXT("Almost no bulge, four fragmented arms.");
	case EGalaxyClass::SpiralSm: return TEXT("Magellanic spiral: one dominant arm, off-centre bar.");
	case EGalaxyClass::BarredSBa: return TEXT("Long bar, big bulge, inner ring, tight arms.");
	case EGalaxyClass::BarredSBab: return TEXT("Long bar, ring-like tight arms.");
	case EGalaxyClass::BarredSBb: return TEXT("Classic barred spiral.");
	case EGalaxyClass::BarredSBbc: return TEXT("Bar with open arms from its ends.");
	case EGalaxyClass::BarredSBc: return TEXT("Short bulge, long open arms.");
	case EGalaxyClass::BarredSBcd: return TEXT("Weak bulge, loose knotty arms.");
	case EGalaxyClass::BarredSBd: return TEXT("Short bar, fragmented arms.");
	case EGalaxyClass::BarredSBm: return TEXT("Barred Magellanic: off-centre bar, one arm.");
	case EGalaxyClass::Irr: return TEXT("Irregular: several bright star clouds.");
	case EGalaxyClass::Im: return TEXT("Magellanic irregular: off-centre bar and one stubby arm.");
	case EGalaxyClass::IBm: return TEXT("Barred irregular: a strong bar with two lobes.");
	case EGalaxyClass::dIrr: return TEXT("Dwarf irregular: soft star clouds.");
	case EGalaxyClass::I0: return TEXT("Starburst: dusty core with a bipolar outflow.");
	case EGalaxyClass::PecWarped: return TEXT("Warped disk.");
	case EGalaxyClass::PecRing: return TEXT("Ring galaxy: a young knotty ring around an old core.");
	case EGalaxyClass::PecInteracting: return TEXT("Interacting pair joined by a stellar bridge.");
	case EGalaxyClass::PecTidalTails: return TEXT("Merger with two long tidal tails.");
	case EGalaxyClass::PecPolarRing: return TEXT("Polar ring around a lenticular host.");
	default: return TEXT("Legacy class.");
	}
}

FDensityCompensation GetDensityCompensation(const int64 RenderedCount, const int64 ReferenceCount)
{
	FDensityCompensation Result;
	if (ReferenceCount <= 0 || RenderedCount <= ReferenceCount)
	{
		return Result;
	}
	const double Ratio = static_cast<double>(ReferenceCount) / static_cast<double>(RenderedCount);
	Result.EmissionScale = FMath::Pow(Ratio, 0.8);
	Result.RadiusScale = FMath::Max(FMath::Pow(Ratio, 0.25), 0.18);
	return Result;
}
}
