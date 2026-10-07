#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Expansion/APSStarSystems.h"

class AActor;

/**
 * Shared vocabulary of the F10 strategic map v2 (Rio 02.10: "the main screen for expansion, management and development:
 * every real object labelled, selectable, with types, links, orbits and lines; a star list on the right"). The scene
 * (APSStrategicMapScene) reads the live world into these records, the view (SAPSStrategicMapView) projects and paints
 * them over the real 3D world, the panel (SAPSStrategicMapPanel) lists and acts on them.
 */
namespace APSStrategicMap
{
	/** What a live object is on the map. Star systems of the catalogue are FSystemMark, not objects. */
	enum class EKind : uint8
	{
		Star,
		Planet,
		Moon,
		Station,
		Outpost,
		Colony,
		Ship,
		Pilot,
		Anomaly
	};

	/** Layers the left panel switches. */
	enum class ELayer : uint8
	{
		Labels,
		Orbits,
		Routes,
		Network,
		Ships,
		Stations,
		Systems,
		Count
	};

	/** A label plate laid out once per text (the generation menu's plate: type line over the name, designation after it). */
	struct FPlate
	{
		FVector2D Size{0.0, 0.0};
		float TypeTop{0.0f};
		float NameTop{0.0f};
		float NameWidth{0.0f};
		float DesignationTop{0.0f};
	};

	/** One live object of the world: a body, a structure, a ship, an anomaly site or the pilot. */
	struct FObject
	{
		TWeakObjectPtr<AActor> Actor;
		EKind Kind{EKind::Planet};
		FText Name;
		/** The plate's first line: "ROCKY  /  6,371 KM", "G2V  /  MAIN SEQUENCE", "SHIP  /  SCIENCE  /  CLASS S". */
		FText Type;
		/** A, A1, A5.04 for bodies (APSBodyDesignation); empty for the rest. */
		FText Designation;
		FLinearColor Colour{FLinearColor::White};
		/** Physical radius: the surface of a body, the hull or the structure's meshes (cm). */
		double RadiusCm{0.0};
		/** The body it circles (a planet's star, a moon's planet) and the orbit plane; unset for the rest. */
		TWeakObjectPtr<AActor> OrbitCentre;
		FVector OrbitAxisX{FVector::ForwardVector};
		FVector OrbitAxisY{FVector::RightVector};
		double OrbitRadiusCm{0.0};
		/** Lower first: who keeps a label when the screen is crowded. */
		uint8 Priority{9};
		/** The civilization's own (structures, ships, the colony). */
		bool bOwn{false};
		/** The pilot or the ship the pilot flies. */
		bool bPlayer{false};
		FPlate Plate;
	};

	/** A star system of the cluster catalogue drawn on the map (FAPSStarSystems index). */
	struct FSystemMark
	{
		int32 Index{INDEX_NONE};
		FText Name;
		FText Type;
		FLinearColor Colour{FLinearColor::White};
		double RoomCm{0.0};
		/** Relay reach of a claimed system (cm); 0 when it holds none. */
		double ReachCm{0.0};
		APSStars::EKnowledge Knowledge{APSStars::EKnowledge::Catalogued};
		int32 StarCount{1};
		/** Its deep-space anomaly as the civilization knows it: 0 none known, 1 detected, 2 located, 3 investigated. */
		uint8 Anomaly{0};
		bool bHome{false};
		bool bClaimed{false};
		FPlate Plate;
	};

	/** What the map has selected: a live actor or a catalogue system (its anchor is spawned only for the object page). */
	struct FSelection
	{
		TWeakObjectPtr<AActor> Actor;
		int32 SystemIndex{INDEX_NONE};

		bool IsSet() const { return Actor.IsValid() || SystemIndex != INDEX_NONE; }
		bool IsSystem() const { return SystemIndex != INDEX_NONE; }
		bool operator==(const FSelection& Other) const
		{
			return SystemIndex == Other.SystemIndex && Actor.Get() == Other.Actor.Get();
		}
		bool operator!=(const FSelection& Other) const { return !(*this == Other); }
		static FSelection OfActor(AActor* InActor)
		{
			FSelection Selection;
			Selection.Actor = InActor;
			return Selection;
		}
		static FSelection OfSystem(const int32 Index)
		{
			FSelection Selection;
			Selection.SystemIndex = Index;
			return Selection;
		}
	};
}
