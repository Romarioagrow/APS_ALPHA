#include "APSArrivalCurtain.h"

#include "APS_ALPHA/Core/Enums/CharSpawnPlace.h"
#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/SpawnParameters.h"
#include "APS_ALPHA/Core/World/APSPlaceholderGlobe.h"
#include "APS_ALPHA/Gameplay/Civilizations/APSCivilizationMaterializationSubsystem.h"
#include "APS_ALPHA/Gameplay/Colony/APSColonyConstructionSubsystem.h"
#include "APS_ALPHA/Gameplay/Vehicles/APSGroundVehicles.h"
#include "APS_ALPHA/Generation/AstroGenerator.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/DelayedAutoRegister.h"
#include "Styling/CoreStyle.h"
#include "Tickable.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "APSArrivalCurtain"

namespace APSArrivalCurtainLocal
{
	TAutoConsoleVariable<int32> CVarCurtain(TEXT("aps.Loading.Curtain"), 1,
		TEXT("1: a loading curtain over the generated world until the start has settled (Rio 02.10). 0: off."));
	TAutoConsoleVariable<float> CVarMaxSeconds(TEXT("aps.Loading.CurtainMaxSeconds"), 90.0f,
		TEXT("The curtain lifts after this long whatever is still missing (logged)."));

	constexpr double FadeSeconds = 0.6;
	/** The pilot stands still (no faster than this, no jump) for this long before the reveal. */
	constexpr double StillSpeedCmPerSecond = 120.0;
	constexpr double StillSeconds = 1.0;
	constexpr double TeleportCm = 200.0;
	/** And this many frames in a row came under this long. */
	constexpr int32 SmoothFramesNeeded = 20;
	constexpr double SmoothFrameSeconds = 0.05;

	const FLinearColor Night(0.004f, 0.008f, 0.014f, 1.0f);
	const FLinearColor Title(0.78f, 0.92f, 1.0f, 1.0f);
	const FLinearColor Muted(0.42f, 0.55f, 0.64f, 1.0f);
	const FLinearColor Track(0.42f, 0.55f, 0.64f, 0.25f);
	const FLinearColor Accent(0.30f, 0.82f, 1.0f, 1.0f);

	/** The curtain: night, the game's name, the stage being waited for and a thin progress line. */
	class SCurtain final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SCurtain) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&)
		{
			FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 30);
			TitleFont.LetterSpacing = 600;
			FSlateFontInfo StageFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
			StageFont.LetterSpacing = 200;
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
			ChildSlot
			[
				SNew(SBorder).BorderImage(White).BorderBackgroundColor(Night)
				.HAlign(HAlign_Center).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock).Text(LOCTEXT("Title", "APOSFERA")).Font(TitleFont).ColorAndOpacity(Title)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 18.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text_Lambda([this]() { return Stage; }).Font(StageFont).ColorAndOpacity(Muted)
					]
					+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 14.0f, 0.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(320.0f).HeightOverride(2.0f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SBorder).BorderImage(White).BorderBackgroundColor(Track)
							]
							+ SOverlay::Slot().HAlign(HAlign_Left)
							[
								SNew(SBox).WidthOverride_Lambda([this]() { return FOptionalSize(320.0f * ShownProgress); })
								[
									SNew(SBorder).BorderImage(White).BorderBackgroundColor(Accent)
								]
							]
						]
					]
				]
			];
		}

		void Update(const FText& InStage, const float Progress, const float DeltaSeconds)
		{
			Stage = InStage;
			// The line eases toward the stages met, so a stage met at once does not jump.
			ShownProgress = FMath::FInterpTo(ShownProgress, FMath::Clamp(Progress, 0.0f, 1.0f), DeltaSeconds, 3.0f);
		}

	private:
		FText Stage;
		float ShownProgress{0.0f};
	};

	/** One world's curtain: up at its BeginPlay, ticked until it has faded. */
	class FCurtain final : public FTickableGameObject
	{
	public:
		FCurtain(UWorld& InWorld, const bool bInNewCivilization, const bool bInSurfaceStart)
			: World(&InWorld), bNewCivilization(bInNewCivilization), bSurfaceStart(bInSurfaceStart)
		{
			UGameViewportClient* Viewport = InWorld.GetGameViewport();
			if (!Viewport)
			{
				return;
			}
			Widget = SNew(SCurtain);
			Viewport->AddViewportWidgetContent(Widget.ToSharedRef(), 10000);
			Phase = EPhase::Waiting;
			StartSeconds = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Log, TEXT("[APS.Loading] curtain up (%s%s)"),
				bNewCivilization ? TEXT("new game") : TEXT("load"), bSurfaceStart ? TEXT(", surface start") : TEXT(""));
		}

		virtual ~FCurtain() override
		{
			RemoveWidget();
		}

		bool IsUp() const { return Phase != EPhase::Off; }
		const UWorld* GetWorld() const { return World.Get(); }

		virtual TStatId GetStatId() const override
		{
			RETURN_QUICK_DECLARE_CYCLE_STAT(FAPSArrivalCurtain, STATGROUP_Tickables);
		}
		virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
		virtual bool IsTickable() const override { return Phase != EPhase::Off; }
		virtual bool IsTickableWhenPaused() const override { return true; }
		virtual UWorld* GetTickableGameObjectWorld() const override { return World.Get(); }

		virtual void Tick(float DeltaTime) override
		{
			const double Now = FPlatformTime::Seconds();
			if (Phase == EPhase::Waiting)
			{
				int32 Met = 0;
				int32 Total = 0;
				FString Missing;
				const FText Stage = CheckReady(Met, Total, Missing);
				if (Widget.IsValid())
				{
					Widget->Update(Stage, Total > 0 ? static_cast<float>(Met) / Total : 1.0f,
						static_cast<float>(FApp::GetDeltaTime()));
				}
				if (Stage.IsEmpty())
				{
					Lift(TEXT("all ready"));
				}
				else if (Now - StartSeconds > CVarMaxSeconds.GetValueOnGameThread())
				{
					UE_LOG(LogTemp, Warning, TEXT("[APS.Loading] curtain deadline after %.0f s, still missing: %s"),
						Now - StartSeconds, *Missing);
					Lift(TEXT("deadline"));
				}
				return;
			}
			const double Alpha = 1.0 - (Now - FadeStartSeconds) / FadeSeconds;
			if (Alpha <= 0.0)
			{
				RemoveWidget();
				Phase = EPhase::Off;
				return;
			}
			if (Widget.IsValid())
			{
				Widget->SetRenderOpacity(static_cast<float>(Alpha));
			}
		}

	private:
		enum class EPhase : uint8 { Off, Waiting, Fading };

		/** The stage line of the first condition still unmet, empty when all are met; Met of Total, Missing by name. */
		FText CheckReady(int32& OutMet, int32& OutTotal, FString& OutMissing)
		{
			UWorld* GameWorld = World.Get();
			FText Stage;
			OutMet = 0;
			OutTotal = 0;
			const auto Need = [&](const bool bMet, const FText& WaitingFor, const TCHAR* Name)
			{
				++OutTotal;
				if (bMet)
				{
					++OutMet;
					return;
				}
				if (Stage.IsEmpty())
				{
					Stage = WaitingFor;
				}
				if (!OutMissing.IsEmpty())
				{
					OutMissing += TEXT(", ");
				}
				OutMissing += Name;
			};
			if (!Generator.IsValid() && GameWorld)
			{
				for (TActorIterator<AAstroGenerator> It(GameWorld); It; ++It)
				{
					Generator = *It;
					break;
				}
			}
			const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			Need(Pawn != nullptr, LOCTEXT("StagePilot", "PREPARING THE PILOT"), TEXT("pilot"));
			Need(APSPlaceholderGlobe::HasInitialCoverage(GameWorld),
				LOCTEXT("StageNativeSurfaces", "PREPARING PLANET SURFACES"), TEXT("native planet surfaces"));
			if (bNewCivilization)
			{
				Need(Generator.IsValid() && Generator->IsStarterHierarchySpawned(),
					LOCTEXT("StageStarter", "ASSEMBLING THE HOME SYSTEM"), TEXT("starter set"));
			}
			if (bSurfaceStart)
			{
				using EState = AAstroGenerator::ESurfaceSpawnState;
				const EState Landing = Generator.IsValid() ? Generator->GetSurfaceSpawnState() : EState::None;
				// A failed handoff has already released the pilot; waiting on would only hold the curtain to its deadline.
				Need(Landing == EState::Final || Landing == EState::Failed,
					LOCTEXT("StageLanding", "LANDING ON THE SURFACE"), TEXT("surface landing"));
				const UAPSColonyConstructionSubsystem* Colony = GameWorld
					? GameWorld->GetSubsystem<UAPSColonyConstructionSubsystem>() : nullptr;
				Need(Colony && Colony->IsColonyGrounded(), LOCTEXT("StageColony", "RAISING THE COLONY"), TEXT("colony grounded"));
				Need(APSGroundVehicles::AreParked(GameWorld), LOCTEXT("StageVehicles", "PARKING THE VEHICLES"),
					TEXT("vehicles parked"));
				// Rio 03.10: the first frame is inside the headquarters, not the landing site or the hop there.
				const UAPSCivilizationMaterializationSubsystem* Materialization = GameWorld
					? GameWorld->GetSubsystem<UAPSCivilizationMaterializationSubsystem>() : nullptr;
				Need(!(Materialization && Materialization->IsPilotArrivalPending()),
					LOCTEXT("StageArrival", "ENTERING THE HEADQUARTERS"), TEXT("pilot in the headquarters"));
			}

			// Still and smooth: the pilot neither moves nor jumps (offsets from the generator, which a world rebase moves
			// alike), and the last frames came quickly.
			const double Now = FPlatformTime::Seconds();
			bool bStill = false;
			if (Pawn && Generator.IsValid())
			{
				const FVector Offset = Pawn->GetActorLocation() - Generator->GetActorLocation();
				const bool bJumped = bHasLastOffset && FVector::Distance(Offset, LastOffset) > TeleportCm;
				LastOffset = Offset;
				bHasLastOffset = true;
				if (bJumped || Pawn->GetVelocity().Size() > StillSpeedCmPerSecond)
				{
					StillSinceSeconds = -1.0;
				}
				else if (StillSinceSeconds < 0.0)
				{
					StillSinceSeconds = Now;
				}
				bStill = StillSinceSeconds >= 0.0 && Now - StillSinceSeconds >= StillSeconds;
			}
			SmoothFrames = FApp::GetDeltaTime() < SmoothFrameSeconds ? SmoothFrames + 1 : 0;
			Need(bStill && SmoothFrames >= SmoothFramesNeeded, LOCTEXT("StageSettle", "SETTLING IN"), TEXT("still and smooth"));
			return Stage;
		}

		void Lift(const TCHAR* Reason)
		{
			Phase = EPhase::Fading;
			FadeStartSeconds = FPlatformTime::Seconds();
			UE_LOG(LogTemp, Log, TEXT("[APS.Loading] curtain lifting after %.1f s (%s)"), FadeStartSeconds - StartSeconds, Reason);
		}

		void RemoveWidget()
		{
			if (Widget.IsValid())
			{
				UGameViewportClient* Viewport = World.IsValid() ? World->GetGameViewport() : nullptr;
				if (Viewport)
				{
					Viewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
				}
				Widget.Reset();
			}
		}

		TWeakObjectPtr<UWorld> World;
		bool bNewCivilization{false};
		bool bSurfaceStart{false};
		EPhase Phase{EPhase::Off};
		double StartSeconds{0.0};
		double FadeStartSeconds{0.0};
		double StillSinceSeconds{-1.0};
		int32 SmoothFrames{0};
		FVector LastOffset{FVector::ZeroVector};
		bool bHasLastOffset{false};
		TWeakObjectPtr<AAstroGenerator> Generator;
		TSharedPtr<SCurtain> Widget;
	};

	TUniquePtr<FCurtain> GCurtain;

	void OnBeginPlay(TWeakObjectPtr<UWorld> WeakWorld)
	{
		UWorld* World = WeakWorld.Get();
		// Only the generated gameplay route: the menu, the preview and the authored start keep their own flow.
		if (!World || CVarCurtain.GetValueOnGameThread() == 0 || !World->GetMapName().Contains(TEXT("L_WorldGeneration")))
		{
			return;
		}
		const UGameInstance* GameInstance = World->GetGameInstance();
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		const bool bLoading = Gameplay && (Gameplay->bIsLoadingMode || Gameplay->bPendingSavedWorldReplay);
		const bool bNewCivilization = Gameplay && Gameplay->bSpawnGeneratedCivilization && !bLoading;
		const USpawnParameters* Spawn = Gameplay ? Gameplay->SpawnParameters : nullptr;
		const bool bSurfaceStart = bNewCivilization && IsValid(Spawn)
			&& (Spawn->CharacterSpawnPlace == ECharSpawnPlace::PlanetSurface || Spawn->CharacterSpawnPlace == ECharSpawnPlace::MoonSurface);
		GCurtain = MakeUnique<FCurtain>(*World, bNewCivilization, bSurfaceStart);
	}

	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues)
	{
		if (World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE))
		{
			World->OnWorldBeginPlay.AddStatic(&OnBeginPlay, TWeakObjectPtr<UWorld>(World));
		}
	}

	void OnWorldCleanup(UWorld* World, bool, bool)
	{
		if (GCurtain && GCurtain->GetWorld() == World)
		{
			GCurtain.Reset();
		}
	}

	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		FWorldDelegates::OnPostWorldInitialization.AddStatic(&OnWorldInitialized);
		FWorldDelegates::OnWorldCleanup.AddStatic(&OnWorldCleanup);
	});
}

bool APSArrivalCurtain::IsUp(const UWorld* World)
{
	using namespace APSArrivalCurtainLocal;
	return GCurtain && World && GCurtain->GetWorld() == World && GCurtain->IsUp();
}

#undef LOCTEXT_NAMESPACE
