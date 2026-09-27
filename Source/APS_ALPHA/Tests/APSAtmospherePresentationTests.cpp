#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "PlanetaryAtmosphere.h"

namespace APSAtmospherePresentationTests
{
UWorld* CreateWorld()
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues()
		.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	return UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false, ERHIFeatureLevel::Num, &Values);
}

void SetOpticalInputs(AAtmoScape* Atmosphere, const float RadiusKm, const float PresentedRadius)
{
	Atmosphere->PlanetRadius = RadiusKm;
	Atmosphere->AtmosphereHeight = 72.0f;
	Atmosphere->LastPlanetRadius = Atmosphere->PlanetRadius;
	Atmosphere->LastAtmosphereHeight = Atmosphere->AtmosphereHeight;
	Atmosphere->bKeepRelativeScale = true;
	Atmosphere->PresentationPlanetRadiusCm = PresentedRadius;
	Atmosphere->PresentationAtmosphereRadiusCm = PresentedRadius > 0.0f ? PresentedRadius * 1.012f : 0.0f;
	Atmosphere->PresentationOpacityScale = 0.075f;
	Atmosphere->PresentationLightIntensity = 1.0f;
	Atmosphere->AtmosphereOpacity = 8.3f;
	Atmosphere->MultiScatering = 1.6f;
	Atmosphere->RayleighHeight = 7.2f;
	Atmosphere->MieHeight = 1.4f;
	Atmosphere->RayleighScattering = FLinearColor(4.2f, 10.7f, 21.0f, 0.0f);
	Atmosphere->MieScattering = FLinearColor(0.4f, 0.7f, 0.9f, 1.0f);
	Atmosphere->Absorption = FLinearColor(3.1f, 7.2f, 0.5f, 1.0f);
	Atmosphere->MiePhase = 0.28f;
	Atmosphere->OzoneContribution = 0.32f;
	Atmosphere->AtmosphereParticulatesDensity = 6.0f;
	Atmosphere->CameraSamplesCount = 8;
	Atmosphere->LightSamplesCount = 4;
	Atmosphere->SkylightIntensity = 13.0f;
	Atmosphere->SkylightShadow = 0.7f;
	Atmosphere->StartDistanceAO = 0.8f;
	Atmosphere->StepsNumAO = 8.0f;
	Atmosphere->AirGlowIntensity = 0.03f;
	Atmosphere->OutterColor = FLinearColor(0.3f, 0.8f, 0.7f, 1.0f);
	Atmosphere->InsideColor = FLinearColor(0.8f, 0.5f, 0.2f, 1.0f);
	Atmosphere->LightSource = nullptr; // Same directional branch used by PLANET.
}

void CompareMaterial(FAutomationTestBase& Test, UMaterialInstanceDynamic* Physical,
	UMaterialInstanceDynamic* Presented, const FString& Label)
{
	if (!Test.TestNotNull(Label + TEXT(" physical MID"), Physical)
		|| !Test.TestNotNull(Label + TEXT(" presentation MID"), Presented)) return;
	Test.TestTrue(Label + TEXT(" same material parent"), Physical->Parent == Presented->Parent);
	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Physical->GetAllScalarParameterInfo(Infos, Ids);
	Test.TestTrue(Label + TEXT(" scalar inputs exist"), !Infos.IsEmpty());
	for (const FMaterialParameterInfo& Info : Infos)
	{
		float A = 0.0f, B = 0.0f;
		const FString Key = Label + TEXT(" scalar ") + Info.Name.ToString();
		const bool bA = Physical->GetScalarParameterValue(Info, A);
		const bool bB = Presented->GetScalarParameterValue(Info, B);
		Test.TestTrue(Key + TEXT(" readable in both paths"), bA && bB);
		Test.TestTrue(Key + TEXT(" finite"), FMath::IsFinite(A) && FMath::IsFinite(B));
		Test.TestEqual(Key + TEXT(" identical formula result"), A, B);
	}
	Infos.Reset(); Ids.Reset();
	Physical->GetAllVectorParameterInfo(Infos, Ids);
	Test.TestTrue(Label + TEXT(" vector inputs exist"), !Infos.IsEmpty());
	for (const FMaterialParameterInfo& Info : Infos)
	{
		FLinearColor A, B;
		const FString Key = Label + TEXT(" vector ") + Info.Name.ToString();
		const bool bA = Physical->GetVectorParameterValue(Info, A);
		const bool bB = Presented->GetVectorParameterValue(Info, B);
		Test.TestTrue(Key + TEXT(" readable in both paths"), bA && bB);
		const auto Finite = [](const FLinearColor& C)
		{
			return FMath::IsFinite(C.R) && FMath::IsFinite(C.G)
				&& FMath::IsFinite(C.B) && FMath::IsFinite(C.A);
		};
		Test.TestTrue(Key + TEXT(" finite"), Finite(A) && Finite(B));
		Test.TestEqual(Key + TEXT(" identical formula result"), A, B);
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSAtmospherePresentationOpticsTest,
	"APS.Gameplay.Generation.AtmospherePresentation.OpticalParityAndTransformIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSAtmospherePresentationOpticsTest::RunTest(const FString& Parameters)
{
	using namespace APSAtmospherePresentationTests;
	UWorld* World = CreateWorld();
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	AAtmoScape* Physical = World->SpawnActor<AAtmoScape>();
	AAtmoScape* Presented = World->SpawnActor<AAtmoScape>();
	if (!Physical || !Presented)
	{
		AddError(TEXT("Both AtmoScape actors must spawn with their normal construction MIDs"));
		World->DestroyWorld(false);
		return false;
	}
	TInlineComponentArray<UStaticMeshComponent*> PhysicalMeshes, PresentedMeshes;
	Physical->GetComponents(PhysicalMeshes);
	Presented->GetComponents(PresentedMeshes);
	TestEqual(TEXT("Five physical shells"), PhysicalMeshes.Num(), 5);
	TestEqual(TEXT("Five presentation shells"), PresentedMeshes.Num(), 5);
	for (const float RadiusKm : {461.0f, 848.4f, 6450.0f})
	for (const float PresentedRadius : {0.0f, 1200000.0f})
	{
		SetOpticalInputs(Physical, RadiusKm, PresentedRadius);
		SetOpticalInputs(Presented, RadiusKm, PresentedRadius);
		Presented->SetActorTransform(FTransform(FRotator(13.0, 47.0, -8.0),
			FVector(72345.0, -98651.0, 23456.0), FVector(1.2, 0.9, 1.1)));
		TArray<FTransform> BeforeTransforms;
		TArray<uint8> BeforeVisibility;
		for (int32 Index = 0; Index < PresentedMeshes.Num(); ++Index)
		{
			UStaticMeshComponent* Mesh = PresentedMeshes[Index];
			Mesh->SetRelativeTransform(FTransform(FRotator(Index * 5.0, 9.0, -3.0),
				FVector(17.0, Index * 23.0, -11.0), FVector(2.0 + Index, 3.0, 4.0)));
			Mesh->SetVisibility(Index % 2 == 0);
			Mesh->SetHiddenInGame(Index % 3 == 0);
			BeforeTransforms.Add(Mesh->GetComponentTransform());
			BeforeVisibility.Add(uint8(Mesh->IsVisible()) | (uint8(Mesh->bHiddenInGame) << 1));
		}
		const FTransform BeforeActor = Presented->GetActorTransform();
		for (int32 Edit = 0; Edit < 4; ++Edit)
		{
			if (Edit == 1) { Physical->AtmosphereHeight *= 1.7f; Presented->AtmosphereHeight *= 1.7f; }
			if (Edit == 2) { Physical->PlanetRadius *= 2.0f; Presented->PlanetRadius *= 2.0f; }
			Physical->UpdateScale();
			Presented->UpdatePresentationScale();
			const FString Label = FString::Printf(TEXT("radius=%.1f presented=%.1f edit=%d"), RadiusKm, PresentedRadius, Edit);
			TestTrue(Label + TEXT(" actor transform untouched"), Presented->GetActorTransform().Equals(BeforeActor, 0.0));
			TestEqual(Label + TEXT(" relative height parity"), Physical->AtmosphereHeight, Presented->AtmosphereHeight);
			TestEqual(Label + TEXT(" relative Rayleigh parity"), Physical->RayleighHeight, Presented->RayleighHeight);
			TestEqual(Label + TEXT(" relative Mie parity"), Physical->MieHeight, Presented->MieHeight);
			for (int32 Index = 0; Index < PresentedMeshes.Num(); ++Index)
			{
				UStaticMeshComponent* Mesh = PresentedMeshes[Index];
				const FString MeshLabel = Label + TEXT(" ") + Mesh->GetName();
				TestTrue(MeshLabel + TEXT(" transform untouched"), Mesh->GetComponentTransform().Equals(BeforeTransforms[Index], 0.0));
				TestEqual(MeshLabel + TEXT(" visibility untouched"),
					uint8(uint8(Mesh->IsVisible()) | (uint8(Mesh->bHiddenInGame) << 1)), BeforeVisibility[Index]);
				UStaticMeshComponent** Other = PhysicalMeshes.FindByPredicate([Mesh](const UStaticMeshComponent* Candidate)
				{
					return Candidate->GetFName() == Mesh->GetFName();
				});
				if (!Other) { AddError(MeshLabel + TEXT(" missing matching physical component")); continue; }
				CompareMaterial(*this, Cast<UMaterialInstanceDynamic>((*Other)->GetMaterial(0)),
					Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)), MeshLabel);
				const double PhysicalRadiusCm = double(Physical->PlanetRadius) * 100000.0;
				const double AtmosphereRadiusCm = PhysicalRadiusCm + Physical->AtmosphereHeight * 100000.0f;
				const FVector ExpectedPhysicalScale = FVector(Physical->PlanetRadius * 2000)
					* (AtmosphereRadiusCm / PhysicalRadiusCm);
				TestTrue(MeshLabel + TEXT(" gameplay still updates physical scale"),
					(*Other)->GetRelativeScale3D().Equals(ExpectedPhysicalScale, 0.01));
			}
		}
	}
	World->DestroyWorld(false);
	return true;
}

#endif
