#include "APSAudioSurfaceResolver.h"

#include "APSAudioBank.h"
#include "APS_ALPHA/Actors/Tech/Colony.h"
#include "APS_ALPHA/Actors/Tech/Headquarters.h"
#include "APS_ALPHA/Actors/Tech/SpaceStation.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationStarterActors.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyModule.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstance.h"
#include "PhysicsEngine/BodyInstance.h"

namespace APSAudioPlayback
{
	// IDs match DefaultEngine.ini and Import-Footsteps.py. Physical materials and
	// explicit APS.Surface.* component/actor tags are the authoring contract.
	EPhysicalSurface SurfaceFromName(const FString& Name)
	{
		const FString N = Name.ToLower();
		if (N.Contains(TEXT("plastic")) || N.Contains(TEXT("polymer")) || N.Contains(TEXT("linoleum"))
			|| N.Contains(TEXT("rubber")) || N.Contains(TEXT("floor01_soft"))) return SurfaceType3;
		if (N.Contains(TEXT("stone")) || (N.Contains(TEXT("rock")) && !N.Contains(TEXT("rocket"))) || N.Contains(TEXT("concrete"))
			|| N.Contains(TEXT("pavement")) || N.Contains(TEXT("granite"))) return SurfaceType4;
		if (N.Contains(TEXT("wood")) || N.Contains(TEXT("parquet"))) return SurfaceType5;
		if (N.Contains(TEXT("snow")) || N.Contains(TEXT("_ice")) || N.StartsWith(TEXT("ice_"))) return SurfaceType6;
		if (N.Contains(TEXT("metal")) || N.Contains(TEXT("steel"))) return SurfaceType2;
		if (N.Contains(TEXT("grass")) || N.Contains(TEXT("dirt")) || N.Contains(TEXT("soil"))) return SurfaceType1;
		return SurfaceType_Default;
	}

	const FAPSAudioFootsteps& ResolveFootsteps(const FHitResult& Floor, const UAPSAudioBank& Bank)
	{
		const UPrimitiveComponent* Component = Floor.GetComponent();
		const UPhysicalMaterial* Material = Floor.PhysMaterial.Get();
		// Movement floor sweeps do not always request a physical material.
		// Read the contacted body's material without changing collision or friction.
		if (!Material && Component)
		{
			if (const FBodyInstance* Body = Component->GetBodyInstance(Floor.BoneName))
			{
				Material = Body->GetSimplePhysicalMaterial();
			}
		}
		const EPhysicalSurface Surface = UPhysicalMaterial::DetermineSurfaceType(Material);
		const FAPSAudioFootsteps* Authored = Bank.SurfaceFootsteps.Find(Surface);
		if (Surface != SurfaceType_Default && Authored && !Authored->Sounds.IsEmpty())
		{
			return *Authored;
		}

		const AActor* Owner = Component ? Component->GetOwner() : Floor.GetActor();
		static const FName Tags[] = {TEXT("APS.Surface.Grass"), TEXT("APS.Surface.Metal"),
			TEXT("APS.Surface.Plastic"), TEXT("APS.Surface.Stone"), TEXT("APS.Surface.Wood"), TEXT("APS.Surface.Snow")};
		for (int32 I = 0; I < UE_ARRAY_COUNT(Tags); ++I)
		{
			if ((Component && Component->ComponentHasTag(Tags[I])) || (Owner && Owner->ActorHasTag(Tags[I])))
			{
				const FAPSAudioFootsteps* Tagged = Bank.SurfaceFootsteps.Find(static_cast<EPhysicalSurface>(I + 1));
				if (Tagged && !Tagged->Sounds.IsEmpty()) return *Tagged;
			}
		}
		// Inspect only the contacted material slot. Scanning every slot can turn a
		// metal deck into plastic simply because the mesh also contains a window.
		if (Component)
		{
			int32 Section = INDEX_NONE;
			const UMaterialInterface* Visual = Floor.FaceIndex != INDEX_NONE
				? Component->GetMaterialFromCollisionFaceIndex(Floor.FaceIndex, Section) : nullptr;
			if (!Visual && Component->GetNumMaterials() == 1) Visual = Component->GetMaterial(0);
			for (int32 Depth = 0; Visual && Depth < 8; ++Depth)
			{
				const EPhysicalSurface NamedSurface = SurfaceFromName(Visual->GetName());
				const FAPSAudioFootsteps* Named = NamedSurface != SurfaceType_Default ? Bank.SurfaceFootsteps.Find(NamedSurface) : nullptr;
				if (Named && !Named->Sounds.IsEmpty()) return *Named;
				// Runtime tint instances often have generic names; their authored
				// parent still identifies the actual floor material.
				const UMaterialInstance* Instance = Cast<UMaterialInstance>(Visual);
				Visual = Instance ? Instance->Parent.Get() : nullptr;
			}
		}

		// Generated decks and ramps use plain materials with no surface type.
		// Their owner still identifies the structure, including procedural ramp meshes.
		const AActor* Actor = Component ? Component->GetOwner() : Floor.GetActor();
		const bool bMetalStructure = Actor && (
			Actor->IsA<AAPSCivilizationLandingPad>() || Actor->IsA<AAPSCivilizationBaseModule>()
			|| Actor->IsA<AAPSColonyModule>() || Actor->IsA<AColony>()
			|| Actor->IsA<AHeadquarters>() || Actor->IsA<ASpaceship>() || Actor->IsA<ASpaceStation>());
		if (bMetalStructure && !Bank.MetalFootsteps.Sounds.IsEmpty())
		{
			return Bank.MetalFootsteps;
		}
		// An untyped surface override is a fallback, not an override of a known deck.
		if (Authored && !Authored->Sounds.IsEmpty()) return *Authored;
		return Bank.DefaultFootsteps;
	}
}
