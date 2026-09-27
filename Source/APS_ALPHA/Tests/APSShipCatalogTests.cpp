#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Pawns/Spaceships/APSShipCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSShipCatalogPickTest,
	"APS.Gameplay.Vehicle.ShipCatalogPick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSShipCatalogPickTest::RunTest(const FString& Parameters)
{
	UAPSShipCatalog* Catalog = NewObject<UAPSShipCatalog>();
	Catalog->StartingFleetMinSizeClass = ESpaceshipSizeClass::XXS;
	Catalog->StartingFleetMaxSizeClass = ESpaceshipSizeClass::M;

	FAPSShipCatalogEntry Small;
	Small.ShipClass = ASpaceship::StaticClass();
	Small.SizeClass = ESpaceshipSizeClass::S;

	FAPSShipCatalogEntry TooLarge = Small;
	TooLarge.SizeClass = ESpaceshipSizeClass::XL;

	FAPSShipCatalogEntry NotForFleet = Small;
	NotForFleet.bAllowInStartingFleet = false;

	FAPSShipCatalogEntry Medium = Small;
	Medium.SizeClass = ESpaceshipSizeClass::M;
	Medium.Weight = 3.0f;

	Catalog->Ships = {Small, TooLarge, NotForFleet, Medium};

	bool bSawSmall = false;
	bool bSawMedium = false;
	for (int32 Seed = 0; Seed < 200; ++Seed)
	{
		FRandomStream First(Seed);
		FRandomStream Second(Seed);
		const int32 Picked = Catalog->PickStartingFleetIndex(First);
		TestEqual(TEXT("The same seed always picks the same ship"), Catalog->PickStartingFleetIndex(Second), Picked);
		TestTrue(TEXT("Ships above the escort size limit are never picked"), Picked != 1);
		TestTrue(TEXT("Ships excluded from the starting fleet are never picked"), Picked != 2);
		bSawSmall |= Picked == 0;
		bSawMedium |= Picked == 3;
	}
	TestTrue(TEXT("Every eligible ship can be picked"), bSawSmall && bSawMedium);

	Catalog->StartingFleetMaxSizeClass = ESpaceshipSizeClass::XS;
	FRandomStream Stream(7);
	TestEqual(TEXT("No eligible ship gives INDEX_NONE"), Catalog->PickStartingFleetIndex(Stream), static_cast<int32>(INDEX_NONE));
	TestNull(TEXT("No eligible ship gives a null class"), Catalog->PickStartingFleetShip(Stream).Get());
	return true;
}

#endif
