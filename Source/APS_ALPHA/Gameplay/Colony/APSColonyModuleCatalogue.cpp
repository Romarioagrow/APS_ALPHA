#include "APSColonyModuleCatalogue.h"

#include "APS_ALPHA/Gameplay/Origins/APSOrigins.h"

#define LOCTEXT_NAMESPACE "APSColonyModules"

namespace APSColonyModuleParts
{
	// The pack meshes a module prefers; the SpaceColonies pack lives outside the repository, so each has a fallback.
	const TCHAR* const ColonyBox = TEXT("/Game/SpaceColonies/Meshes/SM_ColonyBox01.SM_ColonyBox01");
	const TCHAR* const FieldLamp = TEXT("/Game/SpaceColonies/Meshes/SM_FieldLamp_Bottom.SM_FieldLamp_Bottom");
	const TCHAR* const WindowGlass = TEXT("/Game/SpaceColonies/Materials/MI_WindowGlass02.MI_WindowGlass02");
	const TCHAR* const Tree = TEXT("/Game/SpaceColonies/Meshes/SM_EvergreenTree.SM_EvergreenTree");

	const FLinearColor Hull(0.72f, 0.74f, 0.76f);
	const FLinearColor Metal(0.22f, 0.23f, 0.25f);
	const FLinearColor Dark(0.03f, 0.035f, 0.04f);
	const FLinearColor Solar(0.015f, 0.03f, 0.10f);
	const FLinearColor Amber(0.90f, 0.45f, 0.05f);
	const FLinearColor Cyan(0.05f, 0.55f, 0.70f);
	const FLinearColor Soil(0.10f, 0.06f, 0.035f);
	const FLinearColor Plant(0.06f, 0.25f, 0.05f);
	const FLinearColor Warm(1.0f, 0.85f, 0.65f);
	const FLinearColor Grow(1.0f, 0.40f, 0.80f);
	const FLinearColor Red(1.0f, 0.05f, 0.03f);
	const FLinearColor Green(0.10f, 1.0f, 0.20f);

	FAPSColonyModulePart Shape(const EAPSColonyPartShape InShape, const FVector& Center, const FVector& Size,
		const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator, const bool bCollides = true)
	{
		FAPSColonyModulePart Part;
		Part.Shape = InShape;
		Part.Center = Center;
		Part.Size = Size;
		Part.Color = Color;
		Part.Rotation = Rotation;
		Part.bCollides = bCollides;
		return Part;
	}

	FAPSColonyModulePart Trim(const EAPSColonyPartShape InShape, const FVector& Center, const FVector& Size,
		const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		return Shape(InShape, Center, Size, Color, Rotation, false);
	}

	FAPSColonyModulePart Glow(const EAPSColonyPartShape InShape, const FVector& Center, const FVector& Size,
		const FLinearColor& Color, const float Strength)
	{
		FAPSColonyModulePart Part = Trim(InShape, Center, Size, Color);
		Part.Glow = Strength;
		return Part;
	}

	FAPSColonyModulePart Mesh(const TCHAR* Path, const EAPSColonyPartShape Fallback, const FVector& Center,
		const FVector& Size, const FLinearColor& FallbackColor, const FRotator& Rotation = FRotator::ZeroRotator)
	{
		FAPSColonyModulePart Part = Shape(Fallback, Center, Size, FallbackColor, Rotation);
		Part.PreferredMesh = Path;
		return Part;
	}

	FAPSColonyModuleLight Light(const FVector& Location, const FLinearColor& Color, const float Candelas,
		const float RadiusCm, const FRotator& Rotation = FRotator::ZeroRotator, const float ConeDegrees = 0.0f)
	{
		FAPSColonyModuleLight Result;
		Result.Location = Location;
		Result.Rotation = Rotation;
		Result.Color = Color;
		Result.IntensityCandelas = Candelas;
		Result.RadiusCm = RadiusCm;
		Result.ConeDegrees = ConeDegrees;
		return Result;
	}

	TArray<FAPSColonyModuleSpec> Build()
	{
		using EShape = EAPSColonyPartShape;
		TArray<FAPSColonyModuleSpec> Catalogue;

		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("Habitat");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("HabitatName", "HABITAT MODULE");
			Spec.Description = LOCTEXT("HabitatDescription",
				"Pressurised quarters for the first crew: bunks, galley and an airlock facing the base.");
			Spec.SizeCm = FVector(1000.0, 1400.0, 700.0);
			Spec.BuildSeconds = 20.0;
			Spec.YieldPerMinute[4] = 1.0f;
			Spec.bFoundation = true;
			for (const double X : {-200.0, 200.0})
			{
				for (const double Y : {-450.0, 450.0})
				{
					Spec.Parts.Add(Shape(EShape::Cylinder, FVector(X, Y, 40.0), FVector(45.0, 45.0, 80.0), Metal));
				}
			}
			// A lying cylinder along Y with rounded ends; the airlock and its lit door face the base (+X).
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 310.0), FVector(460.0, 460.0, 1200.0), Hull,
				FRotator(0.0, 0.0, 90.0)));
			Spec.Parts.Add(Shape(EShape::Sphere, FVector(0.0, -600.0, 310.0), FVector(440.0, 220.0, 440.0), Hull));
			Spec.Parts.Add(Shape(EShape::Sphere, FVector(0.0, 600.0, 310.0), FVector(440.0, 220.0, 440.0), Hull));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(228.0, 0.0, 350.0), FVector(14.0, 900.0, 60.0), Dark));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(-228.0, 0.0, 350.0), FVector(14.0, 900.0, 60.0), Dark));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(330.0, 0.0, 230.0), FVector(200.0, 240.0, 300.0), Hull));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(432.0, 0.0, 200.0), FVector(6.0, 130.0, 220.0), Amber));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(438.0, 0.0, 338.0), FVector(8.0, 70.0, 14.0), Warm, 8.0f));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, -300.0, 580.0), FVector(160.0, 220.0, 90.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, 380.0, 690.0), FVector(10.0, 10.0, 280.0), Metal));
			Spec.Lights.Add(Light(FVector(460.0, 0.0, 330.0), Warm, 400.0f, 1500.0f));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("Greenhouse");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("GreenhouseName", "GREENHOUSE");
			Spec.Description = LOCTEXT("GreenhouseDescription",
				"Glass walls over hydroponic beds that feed the colony; grow lamps keep them going at night.");
			Spec.SizeCm = FVector(900.0, 1300.0, 500.0);
			Spec.BuildSeconds = 16.0;
			Spec.YieldPerMinute[1] = 1.0f;
			Spec.bFoundation = true;
			for (const double X : {-210.0, 210.0})
			{
				Spec.Parts.Add(Shape(EShape::Cube, FVector(X, 0.0, 30.0), FVector(240.0, 1100.0, 60.0), Soil));
				Spec.Parts.Add(Trim(EShape::Sphere, FVector(X, 0.0, 95.0), FVector(150.0, 500.0, 70.0), Plant));
				for (const double Y : {-350.0, 350.0})
				{
					Spec.Parts.Add(Mesh(Tree, EShape::Cone, FVector(X, Y, 210.0), FVector(180.0, 180.0, 300.0), Plant));
				}
			}
			for (const double X : {-430.0, 430.0})
			{
				for (const double Y : {-630.0, 630.0})
				{
					Spec.Parts.Add(Shape(EShape::Cube, FVector(X, Y, 235.0), FVector(30.0, 30.0, 470.0), Metal));
				}
			}
			Spec.Parts.Add(Trim(EShape::Cube, FVector(0.0, -630.0, 475.0), FVector(880.0, 24.0, 24.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(0.0, 630.0, 475.0), FVector(880.0, 24.0, 24.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(-430.0, 0.0, 475.0), FVector(24.0, 1280.0, 24.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(430.0, 0.0, 475.0), FVector(24.0, 1280.0, 24.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(0.0, 0.0, 475.0), FVector(24.0, 1280.0, 24.0), Metal));
			// Glass panes on all four sides, in the pack's window glass; without the pack the frame stays open.
			const auto Glass = [&Spec](const FVector& Center, const FVector& Size)
			{
				FAPSColonyModulePart Pane = Trim(EShape::Cube, Center, Size, Hull);
				Pane.PreferredMaterial = WindowGlass;
				Pane.bSkipWithoutPack = true;
				Spec.Parts.Add(Pane);
			};
			Glass(FVector(-436.0, 0.0, 235.0), FVector(6.0, 1240.0, 450.0));
			Glass(FVector(436.0, 0.0, 235.0), FVector(6.0, 1240.0, 450.0));
			Glass(FVector(0.0, -636.0, 235.0), FVector(840.0, 6.0, 450.0));
			Glass(FVector(0.0, 636.0, 235.0), FVector(840.0, 6.0, 450.0));
			for (const double Y : {-300.0, 300.0})
			{
				Spec.Parts.Add(Glow(EShape::Cube, FVector(0.0, Y, 458.0), FVector(500.0, 20.0, 8.0), Grow, 5.0f));
				Spec.Lights.Add(Light(FVector(0.0, Y, 420.0), Grow, 300.0f, 1200.0f));
			}
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("Storage");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("StorageName", "STORAGE DEPOT");
			Spec.Description = LOCTEXT("StorageDescription",
				"Crates of tools, spares and supplies on a pad beside the base, with a loading jib.");
			Spec.SizeCm = FVector(800.0, 1100.0, 520.0);
			Spec.BuildSeconds = 8.0;
			Spec.bFoundation = true;
			const FLinearColor CrateColors[] = {Amber, Metal, Hull};
			int32 Crate = 0;
			for (const double X : {-190.0, 190.0})
			{
				for (const double Y : {-340.0, 0.0, 340.0})
				{
					Spec.Parts.Add(Mesh(ColonyBox, EShape::Cube, FVector(X, Y, 130.0), FVector(300.0, 280.0, 260.0),
						CrateColors[Crate++ % UE_ARRAY_COUNT(CrateColors)]));
				}
			}
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 500.0, 250.0), FVector(30.0, 30.0, 500.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(100.0, 500.0, 490.0), FVector(260.0, 20.0, 20.0), Amber));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("Floodlight");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("FloodlightName", "FLOODLIGHT MAST");
			Spec.Description = LOCTEXT("FloodlightDescription",
				"A tall work light aimed at the base, so the colony stays lit through the night.");
			Spec.SizeCm = FVector(300.0, 300.0, 900.0);
			Spec.BuildSeconds = 5.0;
			Spec.bFoundation = true;
			Spec.Parts.Add(Mesh(FieldLamp, EShape::Cube, FVector(0.0, 0.0, 60.0), FVector(140.0, 140.0, 120.0), Metal));
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 470.0), FVector(28.0, 28.0, 720.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(20.0, 0.0, 830.0), FVector(60.0, 180.0, 20.0), Metal));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(60.0, -50.0, 805.0), FVector(30.0, 70.0, 45.0), Warm, 14.0f));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(60.0, 50.0, 805.0), FVector(30.0, 70.0, 45.0), Warm, 14.0f));
			Spec.Lights.Add(Light(FVector(80.0, 0.0, 790.0), Warm, 60000.0f, 6000.0f, FRotator(-35.0, 0.0, 0.0), 55.0f));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("SolarArray");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("SolarArrayName", "SOLAR ARRAY");
			Spec.Description = LOCTEXT("SolarArrayDescription",
				"Six panels on posts, tilted toward the star at build time: power for the modules.");
			Spec.SizeCm = FVector(900.0, 1400.0, 380.0);
			Spec.BuildSeconds = 12.0;
			Spec.YieldPerMinute[2] = 3.0f;
			Spec.bFoundation = true;
			Spec.bFaceSun = true;
			for (const double X : {-220.0, 220.0})
			{
				Spec.Parts.Add(Trim(EShape::Cube, FVector(X, 0.0, 205.0), FVector(20.0, 1300.0, 16.0), Metal));
				for (const double Y : {-440.0, 0.0, 440.0})
				{
					Spec.Parts.Add(Shape(EShape::Cylinder, FVector(X, Y, 100.0), FVector(18.0, 18.0, 200.0), Metal));
					// Negative pitch tilts the panel's face toward the module front, which faces the star.
					Spec.Parts.Add(Shape(EShape::Cube, FVector(X, Y, 225.0), FVector(360.0, 420.0, 8.0), Solar,
						FRotator(-30.0, 0.0, 0.0)));
				}
			}
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 640.0, 60.0), FVector(120.0, 90.0, 120.0), Metal));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("CommsMast");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("CommsMastName", "COMMS MAST");
			Spec.Description = LOCTEXT("CommsMastDescription",
				"A relay antenna with a red beacon, linking the colony with the headquarters in orbit.");
			Spec.SizeCm = FVector(500.0, 500.0, 1800.0);
			Spec.BuildSeconds = 14.0;
			Spec.YieldPerMinute[3] = 1.0f;
			Spec.bFoundation = true;
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 110.0), FVector(300.0, 300.0, 220.0), Hull));
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 900.0), FVector(40.0, 40.0, 1400.0), Metal));
			Spec.Parts.Add(Shape(EShape::Sphere, FVector(90.0, 0.0, 1280.0), FVector(280.0, 280.0, 70.0), Hull,
				FRotator(-60.0, 0.0, 0.0)));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(150.0, 0.0, 1320.0), FVector(8.0, 8.0, 120.0), Metal,
				FRotator(-60.0, 0.0, 0.0)));
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(0.0, 0.0, 1625.0), FVector(45.0, 45.0, 45.0), Red, 20.0f));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, -70.0, 1720.0), FVector(8.0, 8.0, 160.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, 70.0, 1720.0), FVector(8.0, 8.0, 160.0), Metal));
			Spec.Lights.Add(Light(FVector(0.0, 0.0, 1660.0), Red, 600.0f, 2500.0f));
		}
		{
			// Rio 07-09.10, ORIGIN (T-06): the colony's workshop; in the ladder the first rover is assembled here.
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("Fab");
			Spec.Site = EAPSSpawnSite::Surface;
			Spec.Name = LOCTEXT("FabName", "FABRICATION BAY");
			Spec.Description = LOCTEXT("FabDescription",
				"A roofed bay with a printer and a hoist: spares for every machine, and the colony's first rover.");
			Spec.SizeCm = FVector(1100.0, 1300.0, 650.0);
			Spec.BuildSeconds = 18.0;
			Spec.bFoundation = true;
			Spec.YieldPerMinute[0] = 2.0f;
			Spec.UnlocksToken = APSProgressionTokens::VehiclesRover();
			// Floor slab, two side walls and a back wall, a roof with a hoist beam; the bay opens toward the base (+X).
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 15.0), FVector(1000.0, 1200.0, 30.0), Metal));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(-100.0, -580.0, 300.0), FVector(800.0, 40.0, 560.0), Hull));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(-100.0, 580.0, 300.0), FVector(800.0, 40.0, 560.0), Hull));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(-480.0, 0.0, 300.0), FVector(40.0, 1200.0, 560.0), Hull));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(-100.0, 0.0, 600.0), FVector(800.0, 1200.0, 40.0), Hull));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(-100.0, 0.0, 560.0), FVector(760.0, 30.0, 30.0), Amber));
			Spec.Parts.Add(Trim(EShape::Cube, FVector(260.0, 0.0, 560.0), FVector(30.0, 1100.0, 30.0), Dark));
			// The printer at the back wall and its lit screen; two marker lamps at the roof's open edge.
			Spec.Parts.Add(Shape(EShape::Cube, FVector(-380.0, 0.0, 170.0), FVector(140.0, 300.0, 280.0), Metal));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(-305.0, 0.0, 200.0), FVector(6.0, 200.0, 120.0), Cyan, 10.0f));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(290.0, -560.0, 590.0), FVector(20.0, 20.0, 20.0), Amber, 20.0f));
			Spec.Parts.Add(Glow(EShape::Cube, FVector(290.0, 560.0, 590.0), FVector(20.0, 20.0, 20.0), Amber, 20.0f));
			Spec.Lights.Add(Light(FVector(0.0, 0.0, 560.0), Warm, 600.0f, 2000.0f, FRotator(-90.0, 0.0, 0.0), 80.0f));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("SolarWing");
			Spec.Site = EAPSSpawnSite::Orbit;
			Spec.Name = LOCTEXT("SolarWingName", "SOLAR WING");
			Spec.Description = LOCTEXT("SolarWingDescription",
				"Two panel wings on a truss, bolted to the headquarters by a boom.");
			Spec.SizeCm = FVector(400.0, 3000.0, 420.0);
			Spec.BuildSeconds = 15.0;
			Spec.YieldPerMinute[2] = 4.0f;
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 210.0), FVector(80.0, 2900.0, 80.0), Metal));
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 210.0), FVector(170.0, 170.0, 320.0), Hull,
				FRotator(90.0, 0.0, 0.0)));
			for (const double Y : {-780.0, 780.0})
			{
				Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, Y, 210.0), FVector(6.0, 1300.0, 400.0), Solar));
				Spec.Parts.Add(Trim(EShape::Cube, FVector(0.0, Y, 412.0), FVector(10.0, 1300.0, 10.0), Metal));
				Spec.Parts.Add(Trim(EShape::Cube, FVector(0.0, Y, 8.0), FVector(10.0, 1300.0, 10.0), Metal));
			}
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(0.0, -1450.0, 210.0), FVector(30.0, 30.0, 30.0), Red, 15.0f));
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(0.0, 1450.0, 210.0), FVector(30.0, 30.0, 30.0), Green, 15.0f));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("CargoPods");
			Spec.Site = EAPSSpawnSite::Orbit;
			Spec.Name = LOCTEXT("CargoPodsName", "CARGO PODS");
			Spec.Description = LOCTEXT("CargoPodsDescription",
				"Pressurised containers racked outside the station, freeing the hangar.");
			Spec.SizeCm = FVector(900.0, 700.0, 500.0);
			Spec.BuildSeconds = 10.0;
			Spec.YieldPerMinute[0] = 1.0f;
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 0.0, 250.0), FVector(900.0, 60.0, 60.0), Metal));
			const FLinearColor PodColors[] = {Amber, Hull, Cyan};
			int32 Pod = 0;
			for (const double X : {-300.0, 0.0, 300.0})
			{
				for (const double Y : {-190.0, 190.0})
				{
					Spec.Parts.Add(Mesh(ColonyBox, EShape::Cube, FVector(X, Y, 250.0), FVector(280.0, 300.0, 300.0),
						PodColors[Pod++ % UE_ARRAY_COUNT(PodColors)]));
				}
			}
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(-440.0, -340.0, 480.0), FVector(26.0, 26.0, 26.0), Red, 15.0f));
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(-440.0, 340.0, 480.0), FVector(26.0, 26.0, 26.0), Green, 15.0f));
		}
		{
			FAPSColonyModuleSpec& Spec = Catalogue.AddDefaulted_GetRef();
			Spec.Id = TEXT("NavBeacon");
			Spec.Site = EAPSSpawnSite::Orbit;
			Spec.Name = LOCTEXT("NavBeaconName", "NAV BEACON");
			Spec.Description = LOCTEXT("NavBeaconDescription",
				"An amber marker lamp that guides ships to the headquarters.");
			Spec.SizeCm = FVector(400.0, 400.0, 620.0);
			Spec.BuildSeconds = 6.0;
			Spec.YieldPerMinute[4] = 1.0f;
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 200.0), FVector(160.0, 160.0, 300.0), Hull));
			Spec.Parts.Add(Shape(EShape::Cylinder, FVector(0.0, 0.0, 360.0), FVector(220.0, 220.0, 30.0), Metal));
			Spec.Parts.Add(Glow(EShape::Sphere, FVector(0.0, 0.0, 440.0), FVector(120.0, 120.0, 120.0), Amber, 25.0f));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, 0.0, 580.0), FVector(8.0, 8.0, 180.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, -50.0, 540.0), FVector(6.0, 6.0, 120.0), Metal));
			Spec.Parts.Add(Trim(EShape::Cylinder, FVector(0.0, 50.0, 540.0), FVector(6.0, 6.0, 120.0), Metal));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, -170.0, 200.0), FVector(6.0, 160.0, 200.0), Solar));
			Spec.Parts.Add(Shape(EShape::Cube, FVector(0.0, 170.0, 200.0), FVector(6.0, 160.0, 200.0), Solar));
			Spec.Lights.Add(Light(FVector(0.0, 0.0, 440.0), Amber, 2000.0f, 6000.0f));
		}
		return Catalogue;
	}
}

const TArray<FAPSColonyModuleSpec>& FAPSColonyModuleCatalogue::Get()
{
	static const TArray<FAPSColonyModuleSpec> Catalogue = APSColonyModuleParts::Build();
	return Catalogue;
}

const FAPSColonyModuleSpec* FAPSColonyModuleCatalogue::Find(const FName ModuleId)
{
	return Get().FindByPredicate([ModuleId](const FAPSColonyModuleSpec& Spec) { return Spec.Id == ModuleId; });
}

FPrimaryAssetId FAPSColonyModuleCatalogue::MakeDefinitionId(const FName ModuleId)
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("Buildable")), ModuleId);
}

FName FAPSColonyModuleCatalogue::ModuleIdFromDefinition(const FPrimaryAssetId& DefinitionId)
{
	return DefinitionId.PrimaryAssetType == FPrimaryAssetType(TEXT("Buildable")) ? DefinitionId.PrimaryAssetName : NAME_None;
}

FName FAPSColonyModuleCatalogue::SiteCategory(const EAPSSpawnSite Site)
{
	return Site == EAPSSpawnSite::Surface ? FName(TEXT("APS.Colony.Surface")) : FName(TEXT("APS.Colony.Orbit"));
}

FAPSProductionDefinition FAPSColonyModuleCatalogue::MakeDefinition(const FAPSColonyModuleSpec& Spec,
	const double TimeScale)
{
	FAPSProductionDefinition Definition;
	Definition.DefinitionId = MakeDefinitionId(Spec.Id);
	Definition.Domain = EAPSProductionDomain::Building;
	Definition.Category = SiteCategory(Spec.Site);
	Definition.DisplayName.Namespace = TEXT("APSColonyModules");
	Definition.DisplayName.Key = Spec.Id;
	Definition.DisplayName.DefaultText = Spec.Name.ToString();
	Definition.DurationSeconds = FMath::Max(0.0, Spec.BuildSeconds * (FMath::IsFinite(TimeScale) ? TimeScale : 1.0));
	Definition.MaximumBatchSize = 1;
	return Definition;
}

#undef LOCTEXT_NAMESPACE
