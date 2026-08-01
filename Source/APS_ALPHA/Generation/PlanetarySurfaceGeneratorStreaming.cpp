#include "PlanetarySurfaceGenerator.h"

#include "APS_ALPHA/Actors/Astro/Moon.h"
#include "APS_ALPHA/Actors/Astro/Planet.h"
#include "APS_ALPHA/Core/Enums/MoonType.h"
#include "APS_ALPHA/Core/Enums/PlanetType.h"
#include "Materials/MaterialInstance.h"
#include "UObject/UObjectGlobals.h"

namespace APSWorldScapeProfiles
{
	struct FSurfaceProfile
	{
		UWorldScapeNoiseClass* Noise{nullptr};
		UMaterialInstance* TerrainMaterial{nullptr};
		UMaterialInstance* OceanMaterial{nullptr};
		float NoiseScale{800.0f};
		float NoiseIntensity{1200000.0f};
		float OceanHeight{0.0f};
		bool bOcean{false};
	};
}

void APlanetarySurfaceGenerator::LoadSurfaceAssets()
{
	auto LoadNoise = [](UWorldScapeNoiseClass*& Target, const TCHAR* Path)
	{
		if (!IsValid(Target))
		{
			Target = LoadObject<UWorldScapeNoiseClass>(nullptr, Path);
		}
	};
	auto LoadMaterial = [](UMaterialInstance*& Target, const TCHAR* Path)
	{
		if (!IsValid(Target))
		{
			Target = LoadObject<UMaterialInstance>(nullptr, Path);
		}
	};

	LoadNoise(MoonLikeNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_MoonLike.WSCN_MoonLike"));
	LoadNoise(LavaWorldNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_LavaWorld.WSCN_LavaWorld"));
	LoadNoise(SelenaeNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_Selenae.WSCN_Selenae"));
	LoadNoise(SelenaeMetalNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_Selenae_Metal.WSCN_Selenae_Metal"));
	LoadNoise(EarthLikeNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_EarthLike.WSCN_EarthLike"));
	LoadNoise(EarthNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_EarthNoise.WSCN_EarthNoise"));
	LoadNoise(TerraNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_Terra.WSCN_Terra"));
	LoadNoise(IceWorldNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_IceWorld.WSCN_IceWorld"));
	LoadNoise(TerraDesert, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_Terra_Desert.WSCN_Terra_Desert"));
	LoadNoise(TerraForestNoise, TEXT("/Game/APS/APS_ALPHA/WSC/WSCN_Terra_Forest.WSCN_Terra_Forest"));

	LoadMaterial(MI_Terra, TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Terra.WSC_MI_Terra"));
	LoadMaterial(MI_Selenae, TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Selenae.WSC_MI_Selenae"));
	LoadMaterial(MI_Magma, TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Magma.WSC_MI_Magma"));
	LoadMaterial(MI_Planetary_Ocean, TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_Planetary_Ocean.WSC_MI_Planetary_Ocean"));
	LoadMaterial(MI_Lava_Ocean, TEXT("/Game/APS/APS_ALPHA/WSC/WSC_MI_LavaOcean.WSC_MI_LavaOcean"));
}

void APlanetarySurfaceGenerator::ApplySurfaceProfile(APlanetaryBody* Body)
{
	if (!IsValid(Body) || !IsValid(WorldScapeRootInstance))
	{
		return;
	}
	LoadSurfaceAssets();

	using namespace APSWorldScapeProfiles;
	FSurfaceProfile Profile;
	Profile.Noise = MoonLikeNoise;
	Profile.TerrainMaterial = MI_Selenae;

	if (const APlanet* Planet = Cast<APlanet>(Body))
	{
		switch (Planet->PlanetType)
		{
		case EPlanetType::Terrestrial:
		case EPlanetType::Greenhouse:
		case EPlanetType::SuperEarth:
		case EPlanetType::Pangea:
			Profile = {EarthNoise, MI_Terra, MI_Planetary_Ocean, 520.0f, 1050000.0f, 50000.0f, true};
			break;
		case EPlanetType::Ocean:
			Profile = {EarthLikeNoise, MI_Terra, MI_Planetary_Ocean, 140.0f, 420000.0f, 150000.0f, true};
			break;
		case EPlanetType::Water:
		case EPlanetType::Archipelago:
			Profile = {TerraNoise, MI_Terra, MI_Planetary_Ocean, 260.0f, 720000.0f, 90000.0f, true};
			break;
		case EPlanetType::Forest:
		case EPlanetType::Oasis:
			Profile = {TerraForestNoise, MI_Terra, MI_Planetary_Ocean, 620.0f, 950000.0f, 30000.0f, true};
			break;
		case EPlanetType::Desert:
		case EPlanetType::Sand:
			Profile = {TerraDesert, MI_Terra, nullptr, 700.0f, 1250000.0f, 0.0f, false};
			break;
		case EPlanetType::Volcanic:
		case EPlanetType::Melted:
		case EPlanetType::Lava:
			Profile = {LavaWorldNoise, MI_Magma, MI_Lava_Ocean, 430.0f, 1500000.0f, 25000.0f, true};
			break;
		case EPlanetType::Ice:
		case EPlanetType::Frozen:
		case EPlanetType::Nordic:
		case EPlanetType::Tundra:
			Profile = {IceWorldNoise, MI_Selenae, nullptr, 560.0f, 900000.0f, 0.0f, false};
			break;
		case EPlanetType::Metal:
		case EPlanetType::Metallic:
		case EPlanetType::Carbon:
			Profile = {SelenaeMetalNoise, MI_Selenae, nullptr, 820.0f, 1150000.0f, 0.0f, false};
			break;
		case EPlanetType::Ammonia:
			Profile = {SelenaeNoise, MI_Selenae, nullptr, 680.0f, 900000.0f, 0.0f, false};
			break;
		case EPlanetType::HighMountain:
			Profile = {TerraNoise, MI_Terra, nullptr, 900.0f, 1900000.0f, 0.0f, false};
			break;
		case EPlanetType::Rocky:
		case EPlanetType::Dwarf:
		case EPlanetType::Rogue:
		case EPlanetType::Exoplanet:
		case EPlanetType::Unknown:
		default:
			Profile = {MoonLikeNoise, MI_Selenae, nullptr, 800.0f, 1200000.0f, 0.0f, false};
			break;
		}
	}
	else if (const AMoon* Moon = Cast<AMoon>(Body))
	{
		switch (Moon->MoonType)
		{
		case EMoonType::Icy:
			Profile = {IceWorldNoise, MI_Selenae, nullptr, 520.0f, 650000.0f, 0.0f, false};
			break;
		case EMoonType::Iron:
			Profile = {SelenaeMetalNoise, MI_Selenae, nullptr, 760.0f, 800000.0f, 0.0f, false};
			break;
		case EMoonType::Volcanic:
			Profile = {LavaWorldNoise, MI_Magma, MI_Lava_Ocean, 420.0f, 900000.0f, 12000.0f, true};
			break;
		case EMoonType::Ocean:
			Profile = {EarthLikeNoise, MI_Terra, MI_Planetary_Ocean, 180.0f, 300000.0f, 75000.0f, true};
			break;
		case EMoonType::Continental:
			Profile = {TerraNoise, MI_Terra, MI_Planetary_Ocean, 360.0f, 550000.0f, 25000.0f, true};
			break;
		case EMoonType::Desert:
			Profile = {TerraDesert, MI_Terra, nullptr, 620.0f, 720000.0f, 0.0f, false};
			break;
		case EMoonType::Gas:
		case EMoonType::Peculiar:
			Profile = {SelenaeNoise, MI_Selenae, nullptr, 660.0f, 600000.0f, 0.0f, false};
			break;
		case EMoonType::Rocky:
		case EMoonType::TidallyLocked:
		case EMoonType::CapturedAsteroid:
		case EMoonType::Unknown:
		default:
			Profile = {MoonLikeNoise, MI_Selenae, nullptr, 720.0f, 700000.0f, 0.0f, false};
			break;
		}
	}

	if (Body->WorldScapeSeed == 0)
	{
		const uint32 IdentityHash = HashCombine(GetTypeHash(Body->GetFName()), GetTypeHash(Body->GetActorLocation()));
		Body->WorldScapeSeed = 10 + static_cast<int32>(IdentityHash % 999983u);
	}

	const double BodyRadiusCm = FMath::Max(Body->RadiusKM, static_cast<double>(Body->PlanetRadiusKM)) * 100000.0;
	WorldScapeRootInstance->GenerationType = EWorldScapeType::Planet;
	WorldScapeRootInstance->PlanetScale = FMath::Max(BodyRadiusCm, 100000.0);
	WorldScapeRootInstance->DistanceToFreezeGeneration = FMath::Max(
		Body->GetWorldScapeActivationRadiusCm() - WorldScapeRootInstance->PlanetScale,
		WorldScapeRootInstance->PlanetScale * 2.0);
	WorldScapeRootInstance->WorldScapeNoise = Profile.Noise;
	WorldScapeRootInstance->TerrainMaterial.DefaultMaterial = Profile.TerrainMaterial;
	WorldScapeRootInstance->OceanMaterial.DefaultMaterial = Profile.OceanMaterial;
	WorldScapeRootInstance->bOcean = Profile.bOcean && IsValid(Profile.OceanMaterial);
	WorldScapeRootInstance->OceanHeight = Profile.OceanHeight;
	WorldScapeRootInstance->NoiseScale = Profile.NoiseScale;
	WorldScapeRootInstance->NoiseIntensity = Profile.NoiseIntensity;
	WorldScapeRootInstance->Seed = Body->WorldScapeSeed;
	// WorldScape's default eight LODs cover only a small square around the pawn on
	// a full-scale planet. From low orbit that square has visible straight edges.
	// Extra coarse rings extend the mesh beyond the geometric horizon while the
	// single-active-body streaming budget keeps the cost bounded.
	WorldScapeRootInstance->MaxLod = 13;
	WorldScapeRootInstance->LodResolution = 160;
	WorldScapeRootInstance->TriangleSize = 100.0f;
	WorldScapeRootInstance->OceanMaxLod = 13;
	WorldScapeRootInstance->OceanLodResolution = 96;
	WorldScapeRootInstance->OceanTriangleSize = 160.0f;
	WorldScapeRootInstance->HeightAnchor = FMath::Clamp(
		static_cast<float>(WorldScapeRootInstance->PlanetScale * 0.00025), 50000.0f, 250000.0f);
	bSurfaceProfileApplied = true;

	// The legacy generator loads every available preset. Retain only the selected
	// profile so an unloaded family can actually release the rest through GC.
	if (MoonLikeNoise != Profile.Noise) MoonLikeNoise = nullptr;
	if (LavaWorldNoise != Profile.Noise) LavaWorldNoise = nullptr;
	if (SelenaeNoise != Profile.Noise) SelenaeNoise = nullptr;
	if (SelenaeMetalNoise != Profile.Noise) SelenaeMetalNoise = nullptr;
	if (EarthLikeNoise != Profile.Noise) EarthLikeNoise = nullptr;
	if (EarthNoise != Profile.Noise) EarthNoise = nullptr;
	if (TerraNoise != Profile.Noise) TerraNoise = nullptr;
	if (IceWorldNoise != Profile.Noise) IceWorldNoise = nullptr;
	if (TerraDesert != Profile.Noise) TerraDesert = nullptr;
	if (TerraForestNoise != Profile.Noise) TerraForestNoise = nullptr;
	if (MI_Terra != Profile.TerrainMaterial && MI_Terra != Profile.OceanMaterial) MI_Terra = nullptr;
	if (MI_Selenae != Profile.TerrainMaterial && MI_Selenae != Profile.OceanMaterial) MI_Selenae = nullptr;
	if (MI_Magma != Profile.TerrainMaterial && MI_Magma != Profile.OceanMaterial) MI_Magma = nullptr;
	if (MI_Planetary_Ocean != Profile.TerrainMaterial && MI_Planetary_Ocean != Profile.OceanMaterial)
	{
		MI_Planetary_Ocean = nullptr;
	}
	if (MI_Lava_Ocean != Profile.TerrainMaterial && MI_Lava_Ocean != Profile.OceanMaterial) MI_Lava_Ocean = nullptr;

	UE_LOG(LogTemp, Log, TEXT("[APS.WorldScape] Profile body=%s ocean=%s seed=%d noise=%s terrain=%s"),
		*Body->GetName(), WorldScapeRootInstance->bOcean ? TEXT("true") : TEXT("false"),
		Body->WorldScapeSeed, *GetNameSafe(Profile.Noise), *GetNameSafe(Profile.TerrainMaterial));
}

void APlanetarySurfaceGenerator::PreloadWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
	}
	WorldScapeRootInstance->bGenerateWorldScape = false;
	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->SetActorHiddenInGame(true);
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorEnableCollision(false);
}

void APlanetarySurfaceGenerator::FreezeWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		return;
	}
	WorldScapeRootInstance->bGenerateWorldScape = true;
	WorldScapeRootInstance->bFreezeGeneration = true;
	WorldScapeRootInstance->SetActorHiddenInGame(false);
	WorldScapeRootInstance->SetActorTickEnabled(false);
	WorldScapeRootInstance->SetActorEnableCollision(false);
}

void APlanetarySurfaceGenerator::UnloadWorldScapeRoot()
{
	if (!IsValid(WorldScapeRootInstance))
	{
		WorldScapeRootInstance = nullptr;
		return;
	}
	if (bOwnsWorldScapeRootInstance)
	{
		WorldScapeRootInstance->Destroy();
		WorldScapeRootInstance = nullptr;
		bOwnsWorldScapeRootInstance = false;
		bSurfaceProfileApplied = false;
		MoonLikeNoise = nullptr;
		LavaWorldNoise = nullptr;
		SelenaeNoise = nullptr;
		SelenaeMetalNoise = nullptr;
		EarthLikeNoise = nullptr;
		EarthNoise = nullptr;
		TerraNoise = nullptr;
		IceWorldNoise = nullptr;
		TerraDesert = nullptr;
		TerraForestNoise = nullptr;
		MI_Terra = nullptr;
		MI_Selenae = nullptr;
		MI_Magma = nullptr;
		MI_Planetary_Ocean = nullptr;
		MI_Lava_Ocean = nullptr;
	}
	else
	{
		DestroyPlanetEnvironment();
	}
}
