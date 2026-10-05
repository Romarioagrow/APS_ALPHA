#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Audio/APSAudioBank.h"
#include "APS_ALPHA/Core/Audio/APSAudioSurfaceResolver.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ProceduralMeshComponent.h"
#include "Sound/SoundWave.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAudioSurfaceTest, "APS.Audio.FootstepSurfaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAudioSurfaceTest::RunTest(const FString& Parameters)
{
	using namespace APSAudioPlayback;
	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None,
		nullptr, false, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("Isolated surface test world"), World)) return false;

	UAPSAudioBank* Bank = NewObject<UAPSAudioBank>();
	Bank->DefaultFootsteps.Sounds.Add(NewObject<USoundWave>(Bank));
	Bank->MetalFootsteps.Sounds.Add(NewObject<USoundWave>(Bank));
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Terrain = World->SpawnActor<AActor>(Spawn);
	AAPSCivilizationLandingPad* Pad = World->SpawnActor<AAPSCivilizationLandingPad>(Spawn);
	if (!TestNotNull(TEXT("Terrain"), Terrain) || !TestNotNull(TEXT("Landing pad"), Pad))
	{
		World->DestroyWorld(false);
		return false;
	}
	UStaticMeshComponent* Ground = NewObject<UStaticMeshComponent>(Terrain);
	const FHitResult GroundHit(Terrain, Ground, FVector::ZeroVector, FVector::UpVector);
	FHitResult DeckHit(Pad, Pad->Deck, FVector::ZeroVector, FVector::UpVector);
	const auto Uses = [&Bank](const FHitResult& Hit, const FAPSAudioFootsteps& Expected)
	{
		return &ResolveFootsteps(Hit, *Bank) == &Expected;
	};
	TestTrue(TEXT("Ground without a physical material retains terrain steps"), Uses(GroundHit, Bank->DefaultFootsteps));
	TestTrue(TEXT("Planetary deck without a physical material selects metal"), Uses(DeckHit, Bank->MetalFootsteps));
	TestTrue(TEXT("Legacy access ramp selects metal"),
		Uses(FHitResult(Pad, Pad->AccessRamp, FVector::ZeroVector, FVector::UpVector), Bank->MetalFootsteps));
	UProceduralMeshComponent* Ramp = NewObject<UProceduralMeshComponent>(Pad);
	TestTrue(TEXT("Generated procedural ramp selects metal"),
		Uses(FHitResult(Pad, Ramp, FVector::ZeroVector, FVector::UpVector), Bank->MetalFootsteps));
	TestTrue(TEXT("Stepping off the pad immediately restores terrain steps"), Uses(GroundHit, Bank->DefaultFootsteps));

	for (UClass* Class : {AAPSCivilizationBaseModule::StaticClass(), AAPSColonyModule::StaticClass(),
		AColony::StaticClass(), ASpaceship::StaticClass(), ASpaceStation::StaticClass()})
	{
		AActor* Structure = World->SpawnActor<AActor>(Class, Spawn);
		if (!TestNotNull(*Class->GetName(), Structure)) continue;
		UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(Structure);
		TestTrue(*FString::Printf(TEXT("%s walkable mesh selects metal"), *Class->GetName()),
			Uses(FHitResult(Structure, Floor, FVector::ZeroVector, FVector::UpVector), Bank->MetalFootsteps));
	}

	FAPSAudioFootsteps& Authored = Bank->SurfaceFootsteps.Add(SurfaceType1);
	Authored.Sounds.Add(NewObject<USoundWave>(Bank));
	UPhysicalMaterial* Material = NewObject<UPhysicalMaterial>();
	Material->SurfaceType = SurfaceType1;
	DeckHit.PhysMaterial = Material;
	TestTrue(TEXT("Authored physical surface overrides a structure's fallback"), Uses(DeckHit, Authored));
	DeckHit.PhysMaterial.Reset();
	Pad->Deck->SetPhysMaterialOverride(Material);
	TestTrue(TEXT("Body material is honored when movement omitted hit material"), Uses(DeckHit, Authored));
	Pad->Deck->SetPhysMaterialOverride(nullptr);
	Authored.Sounds.Reset();
	DeckHit.PhysMaterial = Material;
	TestTrue(TEXT("An empty authored set falls back to the known deck"), Uses(DeckHit, Bank->MetalFootsteps));
	DeckHit.PhysMaterial.Reset();
	FAPSAudioFootsteps& Untyped = Bank->SurfaceFootsteps.Add(SurfaceType_Default);
	Untyped.Sounds.Add(NewObject<USoundWave>(Bank));
	TestTrue(TEXT("Generic default mapping does not hide metal deck detection"), Uses(DeckHit, Bank->MetalFootsteps));
	TestTrue(TEXT("Generic default mapping still applies to untyped ground"), Uses(GroundHit, Untyped));
	// Explicit surface tags also work on meshes without authored physical materials.
	const TCHAR* SurfaceTags[] = {TEXT("APS.Surface.Plastic"), TEXT("APS.Surface.Stone"),
		TEXT("APS.Surface.Wood"), TEXT("APS.Surface.Snow")};
	for (int32 I = 0; I < UE_ARRAY_COUNT(SurfaceTags); ++I)
	{
		FAPSAudioFootsteps& Added = Bank->SurfaceFootsteps.Add(static_cast<EPhysicalSurface>(I + 3));
		Added.Sounds.Add(NewObject<USoundWave>(Bank));
		Pad->Deck->ComponentTags.Add(SurfaceTags[I]);
		TestTrue(TEXT("Explicit surface tag overrides a known metal structure"), Uses(DeckHit, Added));
		Pad->Deck->ComponentTags.Remove(SurfaceTags[I]);
		TestTrue(TEXT("Leaving a tagged surface restores the deck"), Uses(DeckHit, Bank->MetalFootsteps));
	}
	Pad->Deck->ComponentTags.Add(TEXT("APS.Surface.Plastic"));
	FAPSAudioFootsteps& Explicit = Bank->SurfaceFootsteps.FindChecked(SurfaceType1);
	Explicit.Sounds.Add(NewObject<USoundWave>(Bank));
	DeckHit.PhysMaterial = Material;
	TestTrue(TEXT("Physical material takes priority over surface tags"), Uses(DeckHit, Explicit));
	DeckHit.PhysMaterial.Reset();
	Pad->Deck->ComponentTags.Reset();
	Bank->SurfaceFootsteps.Reset();
	Bank->MetalFootsteps.Sounds.Reset();
	TestTrue(TEXT("A missing metal bank safely falls back to terrain"), Uses(DeckHit, Bank->DefaultFootsteps));
	TestTrue(TEXT("An empty hit is safe"), Uses(FHitResult(), Bank->DefaultFootsteps));
	World->DestroyWorld(false);
	return true;
}
#endif
