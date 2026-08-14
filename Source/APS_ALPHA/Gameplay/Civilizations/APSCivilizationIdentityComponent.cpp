#include "APSCivilizationIdentityComponent.h"

UAPSCivilizationIdentityComponent::UAPSCivilizationIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAPSCivilizationIdentityComponent::InitializeFromManifest(
	const FAPSCivilizationRuntimeManifest& Manifest,
	const FAPSCivilizationManifestEntity& Entity)
{
	ManifestSchemaVersion = Manifest.SchemaVersion;
	StableEntityId = Entity.StableId;
	ParentStableId = Entity.ParentStableId;
	OwnerCivilizationId = Manifest.CivilizationId;
	FactionId = Manifest.FactionId;
	Role = Entity.Role;
	HomeBodyKey = Manifest.HomeBodyKey;
}
