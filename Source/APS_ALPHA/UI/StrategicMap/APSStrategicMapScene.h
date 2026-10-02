#pragma once

#include "CoreMinimal.h"
#include "APSStrategicMapTypes.h"
#include "Fonts/SlateFontInfo.h"

class AActor;
class AAstroGenerator;
class APawn;
class APlanet;
class ASpaceship;
class FAPSStarSystems;
class UWorld;

namespace APSStrategicMap
{
	/** The generation menu's plate fonts: the type line and the name in the readable face, the designation in the display face. */
	FSlateFontInfo PlateTypeFont();
	FSlateFontInfo PlateNameFont();
	FSlateFontInfo PlateDesignationFont();
	/** Even paddings round the capitals (as SPreviewSystemOverlay::LayoutLabel). */
	FPlate LayoutPlate(const FText& Type, const FText& Name, const FText& Designation);
	inline constexpr float PlateBar = 3.0f;
	inline constexpr float PlatePadX = 8.0f;
	inline constexpr float PlatePadY = 6.0f;
	inline constexpr float PlateLineGap = 5.0f;
	inline constexpr float PlateDesignationGap = 6.0f;
}

/**
 * What the strategic map shows, read from the live world (Rio 02.10: "every real object labelled: planets, moons,
 * surfaces, stations, outposts, ships"): stars, planets and moons with their orbits, stations, shipyards and
 * headquarters, outposts, colonies and built infrastructure, anomaly sites, the fleet's ships and the pilot, and the
 * catalogue star systems near the view plus every known or claimed one with the relay network. Re-read twice a second
 * and at once after a fleet, infrastructure or star-system revision; positions are read live by the view every frame.
 * Selection, hover and layers live here too, shared by the view and the side panels.
 */
class FAPSStrategicMapScene
{
public:
	FAPSStrategicMapScene(UWorld* InWorld, AAstroGenerator* InGenerator);

	/** Re-reads the world when due. The view focus and distance pick the catalogue systems near what is looked at. */
	void Update(float DeltaSeconds, const FVector& ViewFocus, double ViewDistance);
	void Invalidate()
	{
		bObjectsDirty = true;
		bSystemsDirty = true;
	}

	const TArray<APSStrategicMap::FObject>& GetObjects() const { return Objects; }
	const TArray<APSStrategicMap::FSystemMark>& GetSystems() const { return Systems; }
	/** The relay network: claimed systems within reach of each other, as catalogue index pairs. */
	const TArray<TPair<int32, int32>>& GetLinks() const { return Links; }
	int32 FindObject(const AActor* Actor) const;
	int32 FindSystem(int32 CatalogueIndex) const;
	/** Changes whenever the object list was re-read. */
	uint32 GetObjectsSerial() const { return ObjectsSerial; }
	/** Stars, planets and moons with the radius the camera keeps out of. */
	void GetObstacles(TArray<TPair<TWeakObjectPtr<AActor>, double>>& OutObstacles) const;
	/** A target on or just over a planet's or moon's ground (a colony, an anomaly site, a landed ship, the pilot on foot):
	 * the local up there and the body's radius. False for anything in orbit or in space. */
	bool SurfaceUp(const APSStrategicMap::FSelection& Target, FVector& OutUp, double& OutBodyRadiusCm) const;

	const APSStrategicMap::FSelection& GetSelection() const { return Selection; }
	/** A system anchor actor is stored as its catalogue system. */
	void Select(const APSStrategicMap::FSelection& NewSelection);
	uint32 GetSelectionSerial() const { return SelectionSerial; }
	APSStrategicMap::FSelection Hover;

	bool IsLayerOn(APSStrategicMap::ELayer Layer) const { return (LayerMask & (1u << static_cast<uint32>(Layer))) != 0; }
	void SetLayer(APSStrategicMap::ELayer Layer, bool bOn);
	uint32 GetLayerMask() const { return LayerMask; }
	void SetLayerMask(uint32 Mask) { LayerMask = Mask; }
	/** Rio 02.10: one switch hides every mark (labels, rings, orbits, routes) and leaves just space; the layers keep. */
	bool IsCleanView() const { return bCleanView; }
	void SetCleanView(const bool bOn) { bCleanView = bOn; }

	/** The actor that stands for a selection on the object page: the actor, or the system's anchor (spawned on demand). */
	AActor* ResolveActor(const APSStrategicMap::FSelection& Target) const;
	bool Locate(const APSStrategicMap::FSelection& Target, FVector& OutLocation) const;
	bool LocateSystem(int32 CatalogueIndex, FVector& OutLocation) const;
	/** The sphere a focus on the target frames (cm): a system's room, a planet with its near orbits, a ship's hull. */
	double FrameRadius(const APSStrategicMap::FSelection& Target) const;
	/** The radius the camera may approach to: the body itself, a small share of a system's room. */
	double FocusRadius(const APSStrategicMap::FSelection& Target) const;
	FText NameOf(const APSStrategicMap::FSelection& Target) const;

	UWorld* GetWorld() const { return World.Get(); }
	FAPSStarSystems* GetStars() const;
	/** The actor free points are kept relative to: the home star, else the home system. */
	AActor* GetReference() const;
	AActor* GetHomeStar() const;
	APlanet* GetHomePlanet() const;
	/** The ship the pilot flies, else the flagship, else the generator's home ship. */
	ASpaceship* GetMyShip() const;
	APawn* GetPilot() const;
	/** The home planets' orbit normal: the map's "up". */
	FVector GetFrameUp() const;
	/** The home system's sphere (its room in the catalogue, else its outermost orbit), cm. */
	double GetHomeRoomCm() const;
	/** A sphere round home holding its nearest few dozen catalogue neighbours, cm. */
	double GetClusterFrameRadius() const;

private:
	void RefreshObjects();
	void RefreshSystems(const FVector& Focus);
	APSStrategicMap::FPlate CachedPlate(const FText& Type, const FText& Name, const FText& Designation);

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AAstroGenerator> Generator;
	TArray<APSStrategicMap::FObject> Objects;
	TArray<APSStrategicMap::FSystemMark> Systems;
	TArray<TPair<int32, int32>> Links;
	TMap<FString, APSStrategicMap::FPlate> PlateCache;
	APSStrategicMap::FSelection Selection;
	uint32 SelectionSerial{1};
	uint32 ObjectsSerial{1};
	uint32 LayerMask{0xFFFFFFFFu};
	bool bCleanView{false};
	bool bObjectsDirty{true};
	bool bSystemsDirty{true};
	float ObjectsClock{0.0f};
	float SystemsClock{0.0f};
	FVector SystemsFocus{FVector::ZeroVector};
	uint32 SeenFleetRevision{0};
	uint32 SeenInfrastructureRevision{0};
	uint32 SeenStarsRevision{0};
};
