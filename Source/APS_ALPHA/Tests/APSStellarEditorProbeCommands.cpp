// Explicit, editor-only entry point for unattended stellar rendering checks.
// No scene generation, material mutation or persistent editor setting changes.
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "PlayInEditorDataTypes.h"
#include "UObject/Package.h"

namespace APSStellarEditorProbe
{
	static void StartPIE()
	{
		if (!GEditor || GEditor->PlayWorld)
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.StellarProbe] Refusing to replace an active play session."));
			return;
		}
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		if (!EditorWorld || EditorWorld->GetOutermost()->IsDirty()
			|| EditorWorld->GetOutermost()->GetName()
				!= TEXT("/Game/APS/APS_ALPHA/Menu/L_APS_MainMenu_Alpha"))
		{
			UE_LOG(LogTemp, Warning, TEXT("[APS.StellarProbe] Requires the saved main-menu map; no map is opened or saved automatically."));
			return;
		}
		FRequestPlaySessionParams Params;
		Params.WorldType = EPlaySessionWorldType::PlayInEditor;
		Params.SessionDestination = EPlaySessionDestinationType::InProcess;
		// No destination viewport: UE opens its normal standalone PIE viewport.
		// The play settings are duplicated by RequestPlaySession, not saved.
		GEditor->RequestPlaySession(Params);
		UE_LOG(LogTemp, Display, TEXT("[APS.StellarProbe] Normal PIE requested; simulation-only mode is not used."));
	}

	static FAutoConsoleCommand StartPIECommand(
		TEXT("APS.Diagnostics.StartStellarPIE"),
		TEXT("Start normal PIE from the saved APS main menu for unattended render diagnostics. Editor only; refuses an active game."),
		FConsoleCommandDelegate::CreateStatic(&StartPIE),
		ECVF_Cheat);
}
#endif

