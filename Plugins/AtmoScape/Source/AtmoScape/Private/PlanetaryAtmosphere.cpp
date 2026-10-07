// Copyright 2021 IOLACORP STUDIO. All Rights Reserved

#include "PlanetaryAtmosphere.h"

bool AAtmoScape::ShouldTickIfViewportsOnly() const
{
	return true;
}

// Sets default values
AAtmoScape::AAtmoScape()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("PlanetaryAtmoRoot"));
	RootComponent = Root;
	PlanetaryAtmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetaryAtmoMesh"));
	PlanetaryAtmoMesh->SetupAttachment(Root);
	PlanetaryAtmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpacePlanetaryAtmoMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpacePlanetaryAtmoMesh"));
	SpacePlanetaryAtmoMesh->SetupAttachment(Root);
	SpacePlanetaryAtmoMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanetarySkylightMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetarySkylightMesh"));
	PlanetarySkylightMesh->SetupAttachment(Root);
	PlanetarySkylightMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanetaryAbsorptionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetaryAbsorptionMesh"));
	PlanetaryAbsorptionMesh->SetupAttachment(Root);
	PlanetaryAbsorptionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlanetaryOutterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlanetarOutterMesh"));
	PlanetaryOutterMesh->SetupAttachment(Root);
	PlanetaryOutterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	bool HDAtmosMesh = false;

	if (HDAtmosMesh)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> AtmosphereStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_AtmosphereMeshHD.SM_AtmosphereMeshHD'"));
		if (AtmosphereStaticMesh.Succeeded())
		{
			PlanetaryAtmoMesh->SetStaticMesh(AtmosphereStaticMesh.Object);
		}
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> AtmosphereStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_AtmosphereMesh.SM_AtmosphereMesh'"));
		if (AtmosphereStaticMesh.Succeeded())
		{
			PlanetaryAtmoMesh->SetStaticMesh(AtmosphereStaticMesh.Object);
		}
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SpaceAtmosphereStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_SpaceAtmosphereMesh.SM_SpaceAtmosphereMesh'"));
	if (SpaceAtmosphereStaticMesh.Succeeded())
	{
		SpacePlanetaryAtmoMesh->SetStaticMesh(SpaceAtmosphereStaticMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlanetarySkylightStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_AtmosphereMesh.SM_AtmosphereMesh'"));
	if (SpaceAtmosphereStaticMesh.Succeeded())
	{
		PlanetarySkylightMesh->SetStaticMesh(PlanetarySkylightStaticMesh.Object);
	}
	
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlanetaryAbsorptionStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_AtmosphereMesh.SM_AtmosphereMesh'"));
	if (SpaceAtmosphereStaticMesh.Succeeded())
	{
		PlanetaryAbsorptionMesh->SetStaticMesh(PlanetaryAbsorptionStaticMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> OutterAtmosphereStaticMesh(TEXT("StaticMesh'/AtmoScape/Mesh/SM_OutterAtmosphereMesh.SM_OutterAtmosphereMesh'"));
	if (OutterAtmosphereStaticMesh.Succeeded())
	{
		PlanetaryOutterMesh->SetStaticMesh(OutterAtmosphereStaticMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> AtmosphereDynMat(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_PlanetaryAtmo.MM_PlanetaryAtmo'"));
	if (AtmosphereDynMat.Succeeded())
	{
		Atmo_Material = AtmosphereDynMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SpaceAtmosphereDynMat(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_PlanetaryAtmo.MM_PlanetaryAtmo'"));
	if (SpaceAtmosphereDynMat.Succeeded())
	{
		SpaceAtmo_Material = SpaceAtmosphereDynMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlanetarySkylightDynMat(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_PlanetarySkylight.MM_PlanetarySkylight'"));
	if (PlanetarySkylightDynMat.Succeeded())
	{
		PlanetarySkylight_Material = PlanetarySkylightDynMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlanetaryAbsorbtionDynMat(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_Absorbtion.MM_Absorbtion'")); 
	if (PlanetaryAbsorbtionDynMat.Succeeded()) 
	{
		Absorbtion_Material = PlanetaryAbsorbtionDynMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlanetaryOutterDynMat(TEXT("MaterialInterface'/AtmoScape/Materials/Master/MM_PlanetaryOutterAtmo.MM_PlanetaryOutterAtmo'")); 
	if (PlanetaryOutterDynMat.Succeeded())
	{
		Outter_Material = PlanetaryOutterDynMat.Object;
	}

	PlanetaryAtmoMesh->bCastDynamicShadow = false;
	PlanetaryAtmoMesh->CastShadow = false;
	PlanetaryAtmoMesh->bCastStaticShadow = false;

	SpacePlanetaryAtmoMesh->bCastDynamicShadow = false;
	SpacePlanetaryAtmoMesh->CastShadow = false;
	SpacePlanetaryAtmoMesh->bCastStaticShadow = false;

	PlanetarySkylightMesh->bCastDynamicShadow = false;
	PlanetarySkylightMesh->CastShadow = false;
	PlanetarySkylightMesh->bCastStaticShadow = false;

	PlanetaryAbsorptionMesh->bCastDynamicShadow = false;
	PlanetaryAbsorptionMesh->CastShadow = false;
	PlanetaryAbsorptionMesh->bCastStaticShadow = false;
	PlanetaryAbsorptionMesh->TranslucencySortPriority = -2;

	PlanetaryOutterMesh->bCastDynamicShadow = false;
	PlanetaryOutterMesh->CastShadow = false;
	PlanetaryOutterMesh->bCastStaticShadow = false;
	//PlanetaryOutterMesh->TranslucencySortPriority = -2;
}

// Called when the game starts or when spawned
void AAtmoScape::BeginPlay()
{
	Super::BeginPlay();
	AtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	SpaceAtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	PlanetarySkylightMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));

	PlanetaryAtmoMesh->SetVisibility(false);
	SpacePlanetaryAtmoMesh->SetVisibility(true);
	PlanetarySkylightMesh->SetVisibility(false);
	PlanetaryOutterMesh->SetVisibility(true);
}

// Called every frame
void AAtmoScape::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	SpaceAtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	PlanetarySkylightMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	PlanetaryOutterMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));

	UpdateScale();
}

// Called every frame
void AAtmoScape::OnConstruction(const FTransform& Transform)
{
	AtmosphereMaterial = PlanetaryAtmoMesh->CreateDynamicMaterialInstance(0, Atmo_Material);
	SpaceAtmosphereMaterial = SpacePlanetaryAtmoMesh->CreateDynamicMaterialInstance(0, SpaceAtmo_Material);
	PlanetarySkylightMaterial = PlanetarySkylightMesh->CreateDynamicMaterialInstance(0, PlanetarySkylight_Material);
	PlanetaryAbsorptionMaterial = PlanetaryAbsorptionMesh->CreateDynamicMaterialInstance(0, Absorbtion_Material);
	PlanetaryOutterMaterial = PlanetaryOutterMesh->CreateDynamicMaterialInstance(0, Outter_Material);

	AtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	SpaceAtmosphereMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	PlanetarySkylightMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));
	PlanetaryOutterMaterial->SetVectorParameterValue("SunlightIntensity", FLinearColor(0, 0, 0, 1.f));

	LastPlanetRadius = PlanetRadius;
	LastAtmosphereHeight = AtmosphereHeight;

	UpdateScale();

}



void AAtmoScape::UpdateMaterialParameters(UMaterialInstanceDynamic* Material, float AtmosRadius, float PlanetScale,
    float LightSegLength, float ScaleHeightR, float ScaleHeightM, float ScaleSkylightIntensity, float ScaleSkylightShadow, float ScaleStartDistanceAO, float ScaleStepsNumAO, float ScaleAirGlowIntensity,
    FLinearColor MieScatteringCoef, FLinearColor RayleighScatteringCoef, FLinearColor OutterColorCoef, FLinearColor InsideColorCoef,
    FLinearColor RayleighOzoneScateringCoef, FVector Vec_PlanetScale,
    FLinearColor AtmoBackHSV)
{
	if (!IsValid(Material)) return;

    Material->SetScalarParameterValue("AtmosRadius", AtmosRadius);
    Material->SetScalarParameterValue("EarthRadius", PlanetScale);
    Material->SetScalarParameterValue("LightSegLength", LightSegLength);
    Material->SetScalarParameterValue("ScaleHeight_R", ScaleHeightR);
    Material->SetScalarParameterValue("ScaleHeight_M", ScaleHeightM);
    Material->SetVectorParameterValue("coef_M", MieScatteringCoef);
    Material->SetVectorParameterValue("coef_R", RayleighScatteringCoef);
    Material->SetVectorParameterValue("coef_RO", RayleighOzoneScateringCoef);
    Material->SetScalarParameterValue("ActorScale", Vec_PlanetScale.Y);
    Material->SetVectorParameterValue("AtmoBack", AtmoBackHSV.HSVToLinearRGB());
    Material->SetScalarParameterValue("AtmosOpacity",
		AtmosphereOpacity * FMath::Max(PresentationOpacityScale, 0.0f));
	const int32 SafeCameraSamples = FMath::Clamp(CameraSamplesCount, 1, 128);
	const int32 SafeLightSamples = FMath::Clamp(LightSamplesCount, 1, 128);
    Material->SetScalarParameterValue("CameraSamples", SafeCameraSamples);
	Material->SetScalarParameterValue("ParticulateIntensity", AtmosphereParticulatesDensity);
    Material->SetScalarParameterValue("CameraSamples_Recip", 1.0f / static_cast<float>(SafeCameraSamples));
    Material->SetScalarParameterValue("LightSamples", SafeLightSamples);
    Material->SetScalarParameterValue("Mie_G", (MiePhase * 1.55) - (MiePhase * MiePhase * MiePhase * 0.55));

    // Ajout du paramиtre Skylight
    Material->SetScalarParameterValue("SkylightIntensity", ScaleSkylightIntensity);
	Material->SetScalarParameterValue("SkylightShadow", ScaleSkylightShadow);
	Material->SetScalarParameterValue("StartDistanceAO", ScaleStartDistanceAO);
	Material->SetScalarParameterValue("StepsNumAO", ScaleStepsNumAO);

	// Ajout du paramиtre OutterAtmo
	Material->SetScalarParameterValue("AirGlowIntensity", ScaleAirGlowIntensity);
	Material->SetVectorParameterValue("OutterColor", OutterColor);
	Material->SetVectorParameterValue("insideColor", InsideColor);
	// MM_PlanetaryAtmo consumes this scalar for both its directional-light and
	// custom-position branches.
	Material->SetScalarParameterValue("LightIntensity",
		FMath::Max(PresentationLightIntensity, 0.0f));

    if (IsValid(LightSource))
    {
        Material->SetScalarParameterValue("CustomLightSource", 1);
        Material->SetScalarParameterValue("SampleDistance", static_cast<int>(bSampleDistance));
        Material->SetVectorParameterValue("LightPosition", LightSource->GetActorLocation());
        //Material->SetScalarParameterValue("LightDistanceScale", 647800000 * LightDistanceScale);
    }
    else
    {
        Material->SetScalarParameterValue("CustomLightSource", 0);
    }

}

void AAtmoScape::UpdateScale()
{
	UpdateScaleInternal(true);
}

void AAtmoScape::UpdatePresentationScale()
{
	UpdateScaleInternal(false);
}

void AAtmoScape::UpdateScaleInternal(const bool bUpdatePhysicalTransforms)
{

	// Zero height means airless. Never divide by its previous value or destroy
	// finite density heights when switching the atmosphere off and back on.
	if (bKeepRelativeScale) {

		if (LastPlanetRadius != PlanetRadius && FMath::IsFinite(LastPlanetRadius)
			&& LastPlanetRadius > 0.0f && FMath::IsFinite(PlanetRadius) && PlanetRadius > 0.0f)
		{
			double SizeCoef = PlanetRadius / LastPlanetRadius;
			RayleighHeight *= SizeCoef;
			MieHeight *= SizeCoef;
			AtmosphereHeight *= SizeCoef;

			LastPlanetRadius = PlanetRadius;
			LastAtmosphereHeight = AtmosphereHeight;
		}
		else if (LastAtmosphereHeight != AtmosphereHeight && FMath::IsFinite(LastAtmosphereHeight)
			&& LastAtmosphereHeight > 0.0f && FMath::IsFinite(AtmosphereHeight) && AtmosphereHeight > 0.0f)
		{
			double SizeCoef = AtmosphereHeight / LastAtmosphereHeight;
			MieHeight *= SizeCoef;
			RayleighHeight *= SizeCoef;

			LastAtmosphereHeight = AtmosphereHeight;
		}
	}

	LastPlanetRadius = PlanetRadius;
	LastAtmosphereHeight = AtmosphereHeight;
	const bool bHasAtmosphere = FMath::IsFinite(PlanetRadius) && PlanetRadius > 0.0f
		&& FMath::IsFinite(AtmosphereHeight) && AtmosphereHeight > 0.0f
		&& FMath::IsFinite(AtmosphereOpacity) && AtmosphereOpacity > 0.0f;
	if (!bHasAtmosphere)
	{
		if (bUpdatePhysicalTransforms)
		{
			for (UStaticMeshComponent* Mesh : {PlanetaryAtmoMesh, SpacePlanetaryAtmoMesh,
				PlanetarySkylightMesh, PlanetaryAbsorptionMesh, PlanetaryOutterMesh})
				if (IsValid(Mesh)) Mesh->SetVisibility(false);
		}
		return;
	}
	const float UnitConversion = 100000; // Km to Cm;

	const double PlanetScale = FMath::Max(
		static_cast<double>(PlanetRadius) * UnitConversion, UE_DOUBLE_SMALL_NUMBER);

	const FVector Vec_PlanetScale = FVector(PlanetRadius * 2000);

	const double AtmosRadius = PlanetScale + (AtmosphereHeight * UnitConversion);
	const double MaterialPlanetScale = PresentationPlanetRadiusCm > 0.0f
		? static_cast<double>(PresentationPlanetRadiusCm) : PlanetScale;
	const double MaterialLengthScale = MaterialPlanetScale / PlanetScale;
	const double MaterialAtmosRadius = PresentationAtmosphereRadiusCm > 0.0f
		? FMath::Max(static_cast<double>(PresentationAtmosphereRadiusCm), MaterialPlanetScale)
		: AtmosRadius * MaterialLengthScale;
	const FVector MaterialVecPlanetScale = Vec_PlanetScale * MaterialLengthScale;

	if (bUpdatePhysicalTransforms)
	{
	FVector CamLocation = FVector(0, 0, 0);

	//Check camera position, if position is outside the sphere, use the spacemesh instead
	if (IsValid(GetWorld()) && IsValid(GetWorld()->GetFirstPlayerController()) && IsValid(GetWorld()->GetFirstPlayerController()->PlayerCameraManager))
	{
		CamLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();
	}

#if WITH_EDITOR
	if (AtmoScapeEditor::IsInViewPort()) {

		CamLocation = AtmoScapeEditor::GetViewPortCameraPosition();
	}
#endif

	// Select the outside shell as soon as the observer actually leaves the authored
	// atmosphere.  The old `* 91` threshold kept the inside skylight/outer meshes
	// active for dozens of planetary radii, which presented as a second giant blue
	// sphere in orbital preview and flickered while the camera crossed LOD contexts.
	// A small tolerance prevents boundary chatter without changing physical scale.
	const double ShellToleranceCm = FMath::Max(100.0, (MaterialAtmosRadius - MaterialPlanetScale) * 0.0001);
	const bool bOutside = FVector::Distance(CamLocation, GetActorLocation()) > MaterialAtmosRadius + ShellToleranceCm;
	// Both shells can be off after preview or a zero-height edit. Never require
	// the opposite shell to be visible before restoring the intended one.
	PlanetaryAtmoMesh->SetVisibility(!bOutside);
	SpacePlanetaryAtmoMesh->SetVisibility(bOutside);
	PlanetarySkylightMesh->SetVisibility(!bOutside);
	PlanetaryOutterMesh->SetVisibility(!bOutside);
	PlanetaryAbsorptionMesh->SetVisibility(true);

	//	UE_LOG(LogTemp, Warning, TEXT("Planet Radius : %f"), PlanetScale);

	const double AtmoScale = AtmosRadius / PlanetScale;

	PlanetaryAtmoMesh->SetRelativeScale3D(AtmoScale * Vec_PlanetScale);
	SpacePlanetaryAtmoMesh->SetRelativeScale3D(AtmoScale * Vec_PlanetScale);
	PlanetarySkylightMesh->SetRelativeScale3D(AtmoScale * Vec_PlanetScale);
	PlanetaryAbsorptionMesh->SetRelativeScale3D(AtmoScale * Vec_PlanetScale);
	PlanetaryOutterMesh->SetRelativeScale3D(AtmoScale * Vec_PlanetScale);
	}

	const double MaterialAtmoScale = MaterialAtmosRadius / MaterialPlanetScale;
	float NomalizeOpticalDepth = MaterialAtmoScale * MaterialAtmoScale;
	NomalizeOpticalDepth = NomalizeOpticalDepth - 1;
	NomalizeOpticalDepth = FMath::Sqrt(FMath::Max(NomalizeOpticalDepth, 0.0f));
	NomalizeOpticalDepth = NomalizeOpticalDepth * 2;
	NomalizeOpticalDepth = NomalizeOpticalDepth * MaterialPlanetScale;

	const float LightSegLength = NomalizeOpticalDepth / FMath::Clamp(LightSamplesCount, 1, 128);

	const double ScaleHeightR = FMath::Max(FMath::IsFinite(RayleighHeight) ? RayleighHeight : 8.0f, 0.00001f)
		* UnitConversion * MaterialLengthScale;

	const double ScaleHeightM = FMath::Max(FMath::IsFinite(MieHeight) ? MieHeight : 1.2f, 0.00001f)
		* UnitConversion * MaterialLengthScale;

	const double ScaleSkylightIntensity = SkylightIntensity;

	const double ScaleSkylightShadow = SkylightShadow;

	const double ScaleStartDistanceAO = StartDistanceAO;

	const double ScaleStepsNumAO = StepsNumAO;

	const double ScaleAirGlowIntensity = AirGlowIntensity;

	float MultiScateringScaled = (MultiScatering * 0.05f)
		/ FMath::Max(MaterialVecPlanetScale.Y, UE_DOUBLE_SMALL_NUMBER);

	FLinearColor MieScatteringCoef = MieScattering * MultiScateringScaled;

	FLinearColor RayleighScatteringCoef = RayleighScattering * MultiScateringScaled;

	FLinearColor OutterColorCoef = OutterColor;

	FLinearColor InsideColorCoef = InsideColor;

	FLinearColor RayleighOzoneScateringCoef = ((Absorption * OzoneContribution) + RayleighScattering) * MultiScateringScaled;

	FLinearColor AtmoBackHSV = RayleighScattering.LinearRGBToHSV();
	AtmoBackHSV.B = 0;

	UpdateMaterialParameters(AtmosphereMaterial, MaterialAtmosRadius, MaterialPlanetScale,
		LightSegLength, ScaleHeightR, ScaleHeightM, ScaleSkylightIntensity, ScaleSkylightShadow, ScaleStartDistanceAO, ScaleStepsNumAO,  ScaleAirGlowIntensity,
		MieScatteringCoef, RayleighScatteringCoef, OutterColorCoef, InsideColorCoef,
		RayleighOzoneScateringCoef, MaterialVecPlanetScale,
		AtmoBackHSV);

	UpdateMaterialParameters(SpaceAtmosphereMaterial, MaterialAtmosRadius, MaterialPlanetScale,
		LightSegLength, ScaleHeightR, ScaleHeightM, ScaleSkylightIntensity, ScaleSkylightShadow, ScaleStartDistanceAO, ScaleStepsNumAO, ScaleAirGlowIntensity,
		MieScatteringCoef, RayleighScatteringCoef, OutterColorCoef, InsideColorCoef, 
		RayleighOzoneScateringCoef, MaterialVecPlanetScale,
		AtmoBackHSV);

	UpdateMaterialParameters(PlanetarySkylightMaterial, MaterialAtmosRadius, MaterialPlanetScale,
		LightSegLength, ScaleHeightR, ScaleHeightM, ScaleSkylightIntensity, ScaleSkylightShadow, ScaleStartDistanceAO, ScaleStepsNumAO, ScaleAirGlowIntensity,
		MieScatteringCoef, RayleighScatteringCoef, OutterColorCoef, InsideColorCoef,
		RayleighOzoneScateringCoef, MaterialVecPlanetScale,
		AtmoBackHSV);

	UpdateMaterialParameters(PlanetaryAbsorptionMaterial, MaterialAtmosRadius, MaterialPlanetScale,
		LightSegLength, ScaleHeightR, ScaleHeightM, ScaleSkylightIntensity, ScaleSkylightShadow, ScaleStartDistanceAO, ScaleStepsNumAO, ScaleAirGlowIntensity,
		MieScatteringCoef, RayleighScatteringCoef, OutterColorCoef, InsideColorCoef,
		RayleighOzoneScateringCoef, MaterialVecPlanetScale,
		AtmoBackHSV);

	UpdateMaterialParameters(PlanetaryOutterMaterial, MaterialAtmosRadius, MaterialPlanetScale,
		LightSegLength, ScaleHeightR, ScaleHeightM, ScaleSkylightIntensity, ScaleSkylightShadow, ScaleStartDistanceAO, ScaleStepsNumAO, ScaleAirGlowIntensity,
		MieScatteringCoef, RayleighScatteringCoef, OutterColorCoef, InsideColorCoef,
		RayleighOzoneScateringCoef, MaterialVecPlanetScale,
		AtmoBackHSV);

}
