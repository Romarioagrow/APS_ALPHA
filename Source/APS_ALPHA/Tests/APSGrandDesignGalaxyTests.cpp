#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "APS_ALPHA/Generation/APSGrandDesignGalaxy.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSGrandDesignGalaxyMorphologyTest,
	"APS.UI.MainMenu.GrandDesignMorphology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSGrandDesignGalaxyMorphologyTest::RunTest(const FString& Parameters)
{
	constexpr int32 Seed = 0x41A05F3;

	int32 SpiralCount = 0;
	int32 BulgeCount = 0;
	int32 HaloCount = 0;
	for (int64 CatalogIndex = 0; CatalogIndex < 4096; ++CatalogIndex)
	{
		const APSGrandDesignGalaxy::FSample First =
			APSGrandDesignGalaxy::SampleCatalog(Seed, CatalogIndex);
		const APSGrandDesignGalaxy::FSample Second =
			APSGrandDesignGalaxy::SampleCatalog(Seed, CatalogIndex);
		TestEqual(TEXT("Menu morphology is deterministic"),
			First.UnitPosition, Second.UnitPosition);
		TestTrue(TEXT("Grand-design sample remains inside its conservative envelope"),
			First.UnitPosition.GetAbsMax() <= 1.13);

		switch (First.Population)
		{
		case APSGrandDesignGalaxy::EPopulation::SpiralArm: ++SpiralCount; break;
		case APSGrandDesignGalaxy::EPopulation::Bulge: ++BulgeCount; break;
		case APSGrandDesignGalaxy::EPopulation::Halo: ++HaloCount; break;
		}
	}

	TestTrue(TEXT("Grand-design sample is arm-dominant"), SpiralCount > 2800);
	TestTrue(TEXT("Grand-design sample retains a luminous bulge"), BulgeCount > 450);
	TestTrue(TEXT("Grand-design sample retains a sparse halo"), HaloCount > 100);
	TestEqual(TEXT("Every sample belongs to one morphology population"),
		SpiralCount + BulgeCount + HaloCount, 4096);
	return true;
}

#endif
