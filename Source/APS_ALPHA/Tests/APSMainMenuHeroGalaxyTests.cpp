#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

#include "APS_ALPHA/Core/Controllers/MainMenuController.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "APS_ALPHA/UI/MainMenu/WorldGenerationViewModel.h"
#include "EngineUtils.h"

namespace APSMainMenuHeroGalaxyTests
{
	constexpr double TimeoutSeconds = 180.0;
	constexpr int32 ExpectedInstanceCount = 9200;

	AAstroGenerator* FindPreviewGenerator(UWorld* World)
	{
		for (TActorIterator<AAstroGenerator> It(World); It; ++It)
		{
			if (IsValid(*It) && It->ActorHasTag(TEXT("WorldGenerationPreview")))
			{
				return *It;
			}
		}
		return nullptr;
	}

	class FHeroGalaxySwitchCommand final : public IAutomationLatentCommand
	{
	public:
		explicit FHeroGalaxySwitchCommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			const double Now = FPlatformTime::Seconds();
			if (StartSeconds <= 0.0)
			{
				StartSeconds = Now;
			}
			if (Now - StartSeconds > TimeoutSeconds)
			{
				Test->AddError(TEXT("Main-menu hero galaxy switch timed out"));
				return true;
			}

			UWorld* World = AutomationCommon::GetAnyGameWorld();
			AMainMenuController* Controller = World
				? Cast<AMainMenuController>(World->GetFirstPlayerController()) : nullptr;
			UWorldGenerationViewModel* ViewModel = Controller
				? Controller->GetWorldGenerationViewModel() : nullptr;
			AAstroGenerator* Generator = World ? FindPreviewGenerator(World) : nullptr;
			if (!Controller || !ViewModel || !Generator)
			{
				return false;
			}

			if (Phase == 0)
			{
				if (!Generator->IsMainMenuHeroGalaxyActive())
				{
					return false;
				}
				Test->TestEqual(TEXT("Landing hero uses its fixed HISM budget"),
					Generator->GetMainMenuHeroGalaxyInstanceCount(), ExpectedInstanceCount);
				Test->TestEqual(TEXT("Landing camera is owned by the preview generator"),
					Controller->GetViewTarget(), static_cast<AActor*>(Generator));
				if (!Controller->OpenAstronomicalGenerationForAutomation(
					EAstroPreviewFocus::StarCluster, EAPSGenerationRoute::Space))
				{
					Test->AddError(TEXT("Could not open Astronomical Generation from landing"));
					return true;
				}
				Phase = 1;
				return false;
			}

			if (!ViewModel->bPreviewReady)
			{
				return false;
			}
			Test->TestFalse(TEXT("Astronomical Generation releases the decorative HISM"),
				Generator->IsMainMenuHeroGalaxyActive());
			Test->TestEqual(TEXT("Released decorative HISM owns no instances"),
				Generator->GetMainMenuHeroGalaxyInstanceCount(), 0);
			Test->TestTrue(TEXT("Astronomical Generation resumes the normal preview contract"),
				Generator->IsPreviewFocusAvailable(EAstroPreviewFocus::StarCluster));
			return true;
		}

	private:
		FAutomationTestBase* Test{nullptr};
		double StartSeconds{0.0};
		int32 Phase{0};
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAPSMainMenuHeroGalaxySwitchTest,
	"APS.Rendered.MainMenu.HeroGalaxySwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSMainMenuHeroGalaxySwitchTest::RunTest(const FString& Parameters)
{
	if (!AutomationOpenMap(TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"), true))
	{
		AddError(TEXT("Could not open the MainMenu map for hero galaxy smoke"));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(
		APSMainMenuHeroGalaxyTests::FHeroGalaxySwitchCommand(this));
	return true;
}

#endif
