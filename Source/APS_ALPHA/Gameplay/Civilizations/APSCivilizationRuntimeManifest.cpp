#include "APSCivilizationRuntimeManifest.h"

#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Pawns/Spaceships/Spaceship.h"
#include "Misc/Crc.h"

namespace
{
	FString NormalizeToken(FString Value)
	{
		Value.TrimStartAndEndInline();
		Value.ReplaceInline(TEXT("\r"), TEXT(""));
		Value.ReplaceInline(TEXT("\n"), TEXT(" "));
		while (Value.Contains(TEXT("  ")))
		{
			Value.ReplaceInline(TEXT("  "), TEXT(" "));
		}
		return Value.ToUpper();
	}

	FString StableRoleToken(const EAPSCivilizationEntityRole Role)
	{
		switch (Role)
		{
		case EAPSCivilizationEntityRole::BaseModule: return TEXT("BASE");
		case EAPSCivilizationEntityRole::LandingPad: return TEXT("PAD");
		case EAPSCivilizationEntityRole::SelectedShip: return TEXT("SHIP");
		default: return TEXT("ENTITY");
		}
	}
}

FName FAPSCivilizationManifestEntity::MakeDeterministicActorName() const
{
	const FString GuidToken = StableId.ToString(EGuidFormats::Digits).Left(12).ToUpper();
	return FName(*FString::Printf(TEXT("APS_CIV_%s_%s"), *GuidToken, *StableRoleToken(Role)));
}

bool FAPSCivilizationRuntimeManifest::IsStructurallyValid(FString* OutReason) const
{
	auto Fail = [OutReason](const TCHAR* Reason)
	{
		if (OutReason)
		{
			*OutReason = Reason;
		}
		return false;
	};

	if (SchemaVersion <= 0 || SchemaVersion > LatestSchemaVersion)
	{
		return Fail(TEXT("unsupported schema version"));
	}
	if (!ManifestId.IsValid() || !CivilizationId.IsValid() || !FactionId.IsValid())
	{
		return Fail(TEXT("missing manifest, civilization, or faction id"));
	}
	if (CivilizationOrdinal < 0 || WorldGenerationSeed == 0
		|| !HomeSystemId.IsValid() || HomeBodyKey.IsEmpty())
	{
		return Fail(TEXT("missing canonical civilization or home-world identity"));
	}
	if (!SelectedShipClass.IsValid())
	{
		return Fail(TEXT("selected ship class is not committed"));
	}
	if (Entities.Num() != 3)
	{
		return Fail(TEXT("starter manifest must contain base, pad, and selected ship"));
	}

	TSet<FGuid> UniqueIds;
	for (const FAPSCivilizationManifestEntity& Entity : Entities)
	{
		if (!Entity.StableId.IsValid() || Entity.OwnerCivilizationId != CivilizationId
			|| Entity.FactionId != FactionId || !Entity.ActorClass.IsValid()
			|| UniqueIds.Contains(Entity.StableId))
		{
			return Fail(TEXT("invalid or duplicate starter entity"));
		}
		UniqueIds.Add(Entity.StableId);
	}

	for (const EAPSCivilizationEntityRole RequiredRole : {
		EAPSCivilizationEntityRole::BaseModule,
		EAPSCivilizationEntityRole::LandingPad,
		EAPSCivilizationEntityRole::SelectedShip})
	{
		if (!FindEntity(RequiredRole))
		{
			return Fail(TEXT("starter entity role is missing"));
		}
	}
	return true;
}

bool FAPSCivilizationRuntimeManifest::MigrateToLatest(FString* OutReason)
{
	if (SchemaVersion == 0)
	{
		SchemaVersion = 1;
		CivilizationOrdinal = FMath::Max(0, CivilizationOrdinal);
		for (FAPSCivilizationManifestEntity& Entity : Entities)
		{
			Entity.OwnerCivilizationId = CivilizationId;
			Entity.FactionId = FactionId;
		}
	}
	return IsStructurallyValid(OutReason);
}

FAPSCivilizationManifestEntity* FAPSCivilizationRuntimeManifest::FindEntity(
	const EAPSCivilizationEntityRole Role)
{
	return Entities.FindByPredicate([Role](const FAPSCivilizationManifestEntity& Entity)
	{
		return Entity.Role == Role;
	});
}

const FAPSCivilizationManifestEntity* FAPSCivilizationRuntimeManifest::FindEntity(
	const EAPSCivilizationEntityRole Role) const
{
	return Entities.FindByPredicate([Role](const FAPSCivilizationManifestEntity& Entity)
	{
		return Entity.Role == Role;
	});
}

FGuid FAPSCivilizationRuntimeManifestFactory::MakeStableId(
	const FString& Namespace, const FString& CanonicalValue)
{
	const FString Base = NormalizeToken(Namespace) + TEXT("|") + NormalizeToken(CanonicalValue);
	return FGuid(
		FCrc::StrCrc32(*(Base + TEXT("|0"))),
		FCrc::StrCrc32(*(Base + TEXT("|1"))),
		FCrc::StrCrc32(*(Base + TEXT("|2"))),
		FCrc::StrCrc32(*(Base + TEXT("|3"))));
}

FString FAPSCivilizationRuntimeManifestFactory::BuildCivilizationDescriptor(
	const USpawnParameters* Parameters)
{
	if (!Parameters)
	{
		return FString();
	}
	return NormalizeToken(FString::Printf(
		TEXT("NAME=%s|ARCH=%d|GOV=%d|ECO=%d|SOC=%d|POP=%d|CREDITS=%lld|TECH=%d|")
		TEXT("FLEET=%d|INFRA=%d,%d,%d,%d|DIV=%d,%d,%d,%d,%d,%d|SHIP=%s"),
		*Parameters->CivilizationName,
		static_cast<int32>(Parameters->CivilizationArchetype),
		static_cast<int32>(Parameters->GovernmentType),
		static_cast<int32>(Parameters->EconomicSystem),
		static_cast<int32>(Parameters->SocietyType),
		Parameters->FoundingPopulation,
		static_cast<long long>(Parameters->StartingCredits),
		Parameters->TechnologyLevel,
		Parameters->StartingFleetSize,
		Parameters->StarOutposts,
		Parameters->PlanetOutposts,
		Parameters->OrbitalOutposts,
		Parameters->GroundOutposts,
		Parameters->ExplorationDivisionLevel,
		Parameters->IndustryDivisionLevel,
		Parameters->ScienceDivisionLevel,
		Parameters->CivilAffairsDivisionLevel,
		Parameters->MilitaryDivisionLevel,
		Parameters->FleetDivisionLevel,
		*GetPathNameSafe(Parameters->BP_HomeSpaceship.Get())));
}

FAPSCivilizationRuntimeManifest FAPSCivilizationRuntimeManifestFactory::Build(
	const int32 WorldGenerationSeed,
	const FGuid& HomeSystemId,
	const FString& HomeBodyKey,
	const USpawnParameters* Parameters,
	const FSoftClassPath& BaseModuleClass,
	const FSoftClassPath& LandingPadClass,
	const int32 CivilizationOrdinal)
{
	FAPSCivilizationRuntimeManifest Result;
	Result.WorldGenerationSeed = WorldGenerationSeed;
	Result.CivilizationOrdinal = FMath::Max(0, CivilizationOrdinal);
	Result.HomeSystemId = HomeSystemId;
	Result.HomeBodyKey = NormalizeToken(HomeBodyKey);
	Result.CivilizationDescriptor = BuildCivilizationDescriptor(Parameters);
	Result.SelectedShipClass = Parameters
		? FSoftClassPath(Parameters->BP_HomeSpaceship.Get()) : FSoftClassPath();

	const FString HomeIdentity = FString::Printf(TEXT("%d|%s|%s"),
		WorldGenerationSeed, *HomeSystemId.ToString(EGuidFormats::Digits), *Result.HomeBodyKey);
	Result.CivilizationId = MakeStableId(TEXT("APS.CIVILIZATION"),
		HomeIdentity + FString::Printf(TEXT("|ORDINAL=%d"), Result.CivilizationOrdinal));
	Result.FactionId = MakeStableId(TEXT("APS.FACTION"),
		Result.CivilizationId.ToString(EGuidFormats::Digits));
	Result.ManifestId = MakeStableId(TEXT("APS.CIVILIZATION.MANIFEST.V1"),
		Result.CivilizationId.ToString(EGuidFormats::Digits) + TEXT("|") + HomeIdentity);

	auto AddEntity = [&Result](const EAPSCivilizationEntityRole Role,
		const FSoftClassPath& ActorClass, const FGuid& ParentId)
	{
		FAPSCivilizationManifestEntity& Entity = Result.Entities.AddDefaulted_GetRef();
		Entity.Role = Role;
		Entity.ActorClass = ActorClass;
		Entity.OwnerCivilizationId = Result.CivilizationId;
		Entity.FactionId = Result.FactionId;
		Entity.ParentStableId = ParentId;
		Entity.StableId = MakeStableId(TEXT("APS.CIVILIZATION.ENTITY.V1"),
			Result.ManifestId.ToString(EGuidFormats::Digits) + TEXT("|") + StableRoleToken(Role));
		return Entity.StableId;
	};

	const FGuid BaseId = AddEntity(EAPSCivilizationEntityRole::BaseModule,
		BaseModuleClass, Result.ManifestId);
	const FGuid PadId = AddEntity(EAPSCivilizationEntityRole::LandingPad,
		LandingPadClass, BaseId);
	AddEntity(EAPSCivilizationEntityRole::SelectedShip, Result.SelectedShipClass, PadId);
	return Result;
}
