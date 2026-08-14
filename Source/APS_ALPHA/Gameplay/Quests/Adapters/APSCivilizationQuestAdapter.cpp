#include "APSCivilizationQuestAdapter.h"

#include "Misc/SecureHash.h"

const FName FAPSCivilizationQuestAdapter::ReadyVerb(
	TEXT("APS.Civilization.Manifest.Ready"));

namespace
{
	bool IsReadyProvenance(const EAPSCivilizationMaterializationState State)
	{
		return State == EAPSCivilizationMaterializationState::Materialized
			|| State == EAPSCivilizationMaterializationState::LoadedFromSave;
	}

	FAPSQuestEntityRef GuidEntity(const EAPSQuestEntityKind Kind, const FGuid& Guid)
	{
		FAPSQuestEntityRef Result;
		Result.Kind = Kind;
		Result.Guid = Guid;
		return Result;
	}
}

bool FAPSCivilizationQuestAdapter::NormalizeReadyManifest(
	const FAPSCivilizationRuntimeManifest& Source,
	FAPSCivilizationQuestReadySnapshot& OutSnapshot,
	FString& OutReason)
{
	OutReason.Reset();
	if (!IsReadyProvenance(Source.MaterializationState))
	{
		OutReason = TEXT("Civilization manifest is not in an actor-ready provenance state");
		return false;
	}

	// The owner delegate passes a const reference. Copy before validation so no reference
	// can escape the callback and schema migration never mutates Civilization state.
	FAPSCivilizationRuntimeManifest Manifest = Source;
	if (!Manifest.MigrateToLatest(&OutReason))
	{
		return false;
	}

	auto CountRole = [&Manifest](const EAPSCivilizationEntityRole Role)
	{
		return Manifest.Entities.CountByPredicate(
			[Role](const FAPSCivilizationManifestEntity& Entity)
			{
				return Entity.Role == Role;
			});
	};
	if (CountRole(EAPSCivilizationEntityRole::BaseModule) != 1
		|| CountRole(EAPSCivilizationEntityRole::LandingPad) != 1
		|| CountRole(EAPSCivilizationEntityRole::SelectedShip) != 1)
	{
		OutReason = TEXT("Actor-ready manifest requires exactly one base, pad and selected ship");
		return false;
	}

	const FAPSCivilizationManifestEntity* Base =
		Manifest.FindEntity(EAPSCivilizationEntityRole::BaseModule);
	const FAPSCivilizationManifestEntity* Pad =
		Manifest.FindEntity(EAPSCivilizationEntityRole::LandingPad);
	const FAPSCivilizationManifestEntity* Ship =
		Manifest.FindEntity(EAPSCivilizationEntityRole::SelectedShip);
	if (!Base || !Pad || !Ship
		|| Base->ParentStableId != Manifest.ManifestId
		|| Pad->ParentStableId != Base->StableId
		|| Ship->ParentStableId != Pad->StableId)
	{
		OutReason = TEXT("Actor-ready manifest parent chain must be manifest -> base -> pad -> ship");
		return false;
	}

	FAPSCivilizationQuestReadySnapshot Normalized;
	Normalized.ManifestId = Manifest.ManifestId;
	Normalized.Provenance = Manifest.MaterializationState;
	Normalized.Base = GuidEntity(EAPSQuestEntityKind::CivilizationEntity, Base->StableId);
	Normalized.LandingPad = GuidEntity(
		EAPSQuestEntityKind::CivilizationEntity, Pad->StableId);
	Normalized.SelectedShip = GuidEntity(
		EAPSQuestEntityKind::CivilizationEntity, Ship->StableId);
	Normalized.HomeSystem = GuidEntity(EAPSQuestEntityKind::StarSystem, Manifest.HomeSystemId);

	const FGuid ReadyFactId = MakeReadyFactId(Manifest.ManifestId);
	Normalized.ReadyEvent.StreamId = Manifest.ManifestId;
	Normalized.ReadyEvent.EventId = ReadyFactId;
	Normalized.ReadyEvent.CorrelationId = ReadyFactId;
	Normalized.ReadyEvent.Sequence = 1;
	Normalized.ReadyEvent.Verb = ReadyVerb;
	Normalized.ReadyEvent.Result = EAPSQuestEventResult::Succeeded;
	Normalized.ReadyEvent.Target = Normalized.Base;
	Normalized.ReadyEvent.ContextLabels.Add(Manifest.MaterializationState
		== EAPSCivilizationMaterializationState::LoadedFromSave
		? FName(TEXT("APS.Civilization.Provenance.LoadedFromSave"))
		: FName(TEXT("APS.Civilization.Provenance.Materialized")));
	if (!Normalized.ReadyEvent.IsStructurallyValid(&OutReason))
	{
		return false;
	}

	OutSnapshot = MoveTemp(Normalized);
	return true;
}

FGuid FAPSCivilizationQuestAdapter::MakeReadyFactId(const FGuid& ManifestId)
{
	const FString Canonical = FString::Printf(TEXT("%s|%s"),
		*ManifestId.ToString(EGuidFormats::DigitsWithHyphensLower), *ReadyVerb.ToString());
	FTCHARToUTF8 Utf8(*Canonical);
	FMD5 Md5;
	Md5.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
	uint8 Digest[16];
	Md5.Final(Digest);
	auto ReadWord = [&Digest](const int32 Offset)
	{
		return static_cast<uint32>(Digest[Offset]) << 24
			| static_cast<uint32>(Digest[Offset + 1]) << 16
			| static_cast<uint32>(Digest[Offset + 2]) << 8
			| static_cast<uint32>(Digest[Offset + 3]);
	};
	return FGuid(ReadWord(0), ReadWord(4), ReadWord(8), ReadWord(12));
}
