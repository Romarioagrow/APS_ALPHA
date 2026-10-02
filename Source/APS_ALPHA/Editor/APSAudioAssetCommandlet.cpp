#include "APSAudioAssetCommandlet.h"

#if WITH_EDITOR
#include "APS_ALPHA/Core/Audio/APSAudioBank.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace APSAudioAssets
{
	const FString Root = TEXT("/Game/APS/APS_ALPHA/Audio/");

	struct FCueSpec
	{
		FString Name;
		FString Source;
		int32 ClassIndex;
		float Gain;
		bool bLoop;
	};

	TArray<FCueSpec> Specs()
	{
		TArray<FCueSpec> Result {
			{TEXT("SC_UI_Click"), TEXT("/Game/Interface_And_Item_Sounds/WAV/Futuristic_Click_10"), 4, 0.60f, false},
			{TEXT("SC_UI_Hover"), TEXT("/Game/Interface_And_Item_Sounds/WAV/Click_03"), 4, 0.25f, false},
			{TEXT("SC_UI_Confirm"), TEXT("/Game/EnergyFieldsSFX/WAV/A_PF_UI_apply"), 4, 0.25f, false},
			{TEXT("SC_UI_Back"), TEXT("/Game/Interface_And_Item_Sounds/WAV/Back_Click_03"), 4, 0.55f, false},
			{TEXT("SC_Music_Menu"), TEXT("/Game/APS/APS_ALPHA/Audio/Waves/SW_Menu_MelodicPlaylist"), 1, 0.55f, true},
			{TEXT("SC_Music_Exploration"), TEXT("/Game/APS/APS_ALPHA/Audio/Waves/SW_Exploration_CalmSuite"), 1, 0.65f, true},
			{TEXT("SC_Ambience_Space"), TEXT("/Game/SpaceAmbBundle/wavs/Space_1/Soundscapes/space_amb_v1_low1_loop"), 2, 0.14f, true},
			{TEXT("SC_Ambience_Interior"), TEXT("/Game/SpaceAmbBundle/wavs/Space_1/Soundscapes/space_amb_v1_base1_loop"), 2, 0.14f, true},
			{TEXT("SC_Ship_Idle"), TEXT("/Game/EnergyFieldsSFX/WAV/A_PF_LowField_Loop"), 3, 0.40f, true},
			{TEXT("SC_Ship_Thrust"), TEXT("/Game/EnergyFieldsSFX/WAV/A_PF_ElectricMachine_Loop"), 3, 0.30f, true},
			{TEXT("SC_Ship_Warp"), TEXT("/Game/EnergyFieldsSFX/WAV/A_PF_MovingForces_Loop"), 3, 0.25f, true},
			{TEXT("SC_Ship_Start"), TEXT("/Game/SciFiGameSounds/waves/Spaceship13"), 3, 0.20f, false},
			{TEXT("SC_Ship_Stop"), TEXT("/Game/SciFiGameSounds/waves/BlackOut7"), 3, 0.40f, false},
			{TEXT("SC_Ship_Mode"), TEXT("/Game/EnergyFieldsSFX/WAV/A_PF_UI_NextPage"), 3, 0.18f, false},
			{TEXT("SC_Landing"), TEXT("/Game/FootstepsMiniPack/SoundWav/DirtRoad_Mono_03"), 3, 2.2f, false}
		};
		for (int32 I = 1; I <= 5; ++I)
		{
			Result.Add({FString::Printf(TEXT("SC_Step_Dirt_%02d"), I),
				FString::Printf(TEXT("/Game/FootstepsMiniPack/SoundWav/DirtRoad_Mono_%02d"), I), 3, 2.2f, false});
			Result.Add({FString::Printf(TEXT("SC_Step_Metal_%02d"), I),
				FString::Printf(TEXT("/Game/FootstepsMiniPack/SoundWav/MetalSteps_%02d"), I), 3, 0.50f, false});
		}
		return Result;
	}

	FString ObjectPath(const FString& Package)
	{
		return Package + TEXT(".") + FPackageName::GetShortName(Package);
	}

	template<typename T>
	T* Make(const FString& Name, TArray<UObject*>& Assets)
	{
		UPackage* Package = CreatePackage(*(Root + Name));
		T* Asset = NewObject<T>(Package, *Name, RF_Public | RF_Standalone);
		Assets.Add(Asset);
		return Asset;
	}

	bool Save(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		Asset->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}

	int32 Validate()
	{
		const UAPSAudioBank* Bank = LoadObject<UAPSAudioBank>(nullptr, *ObjectPath(Root + TEXT("DA_APSAudioBank")));
		if (!Bank || !Bank->Mix || !Bank->MasterClass || !Bank->MusicClass ||
			!Bank->AmbienceClass || !Bank->EffectsClass || !Bank->UIClass) return 1;
		for (const FCueSpec& Spec : Specs())
		{
			const USoundCue* Cue = LoadObject<USoundCue>(nullptr, *ObjectPath(Root + Spec.Name));
			const USoundNodeWavePlayer* Player = Cue ? Cast<USoundNodeWavePlayer>(Cue->FirstNode) : nullptr;
			if (!Player || !Player->GetSoundWave() || !Cue->SoundClassObject || Cue->IsLooping() != Spec.bLoop
				|| Player->GetSoundWave()->GetPathName() != ObjectPath(Spec.Source))
			{
				UE_LOG(LogTemp, Error, TEXT("[APS.Audio] Invalid cue: %s"), *Spec.Name);
				return 1;
			}
		}
		if (Bank->DefaultFootsteps.Sounds.Num() != 5 || Bank->MetalFootsteps.Sounds.Num() != 5) return 1;
		UE_LOG(LogTemp, Display, TEXT("[APS.Audio] Bank validation passed: 25 cues, 5 classes, music/ambience/ship/steps/UI."));
		return 0;
	}
}
#endif

UAPSAudioAssetCommandlet::UAPSAudioAssetCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UAPSAudioAssetCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	using namespace APSAudioAssets;
	if (FParse::Param(*Params, TEXT("Validate"))) return Validate();
	const TArray<FCueSpec> Recipe = Specs();
	const TArray<FString> ClassNames { TEXT("SCL_Master"), TEXT("SCL_Music"), TEXT("SCL_Ambience"), TEXT("SCL_Effects"), TEXT("SCL_UI") };
	TArray<FString> Targets = ClassNames;
	Targets.Append({TEXT("SM_APS"), TEXT("Concurrency_UI"), TEXT("Concurrency_Steps"), TEXT("DA_APSAudioBank")});
	for (const FCueSpec& Spec : Recipe) Targets.Add(Spec.Name);
	// Preflight every output before creating anything; reruns cannot overwrite tuned assets.
	for (const FString& Target : Targets)
	{
		if (FPackageName::DoesPackageExist(Root + Target))
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Audio] Target already exists: %s. Use -Validate to check an installed bank."), *Target);
			return 1;
		}
	}
	TArray<USoundWave*> Waves;
	for (const FCueSpec& Spec : Recipe)
	{
		USoundWave* Wave = LoadObject<USoundWave>(nullptr, *ObjectPath(Spec.Source));
		if (!Wave || Wave->GetDuration() <= 0.f)
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Audio] Missing/unreadable source: %s. No outputs saved."), *Spec.Source);
			return 1;
		}
		Waves.Add(Wave);
	}
	if (FParse::Param(*Params, TEXT("DryRun")))
	{
		UE_LOG(LogTemp, Display, TEXT("[APS.Audio] Preflight passed: %d cues, %d outputs; sources untouched."), Recipe.Num(), Targets.Num());
		return 0;
	}
	TArray<UObject*> Assets;
	TArray<USoundClass*> Classes;
	for (const FString& Name : ClassNames) Classes.Add(Make<USoundClass>(Name, Assets));
	for (int32 I = 1; I < Classes.Num(); ++I) Classes[I]->SetParentClass(Classes[0]);
	Classes[4]->Properties.bIsUISound = true;
	USoundMix* Mix = Make<USoundMix>(TEXT("SM_APS"), Assets);
	Mix->Duration = -1.f;
	Mix->FadeInTime = 0.25f;
	Mix->FadeOutTime = 0.25f;
	USoundConcurrency* UIConcurrency = Make<USoundConcurrency>(TEXT("Concurrency_UI"), Assets);
	UIConcurrency->Concurrency.MaxCount = 4;
	UIConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
	UIConcurrency->Concurrency.RetriggerTime = 0.035f;
	USoundConcurrency* StepConcurrency = Make<USoundConcurrency>(TEXT("Concurrency_Steps"), Assets);
	StepConcurrency->Concurrency.MaxCount = 3;
	StepConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;

	TMap<FString, USoundCue*> Cues;
	for (int32 I = 0; I < Recipe.Num(); ++I)
	{
		const FCueSpec& Spec = Recipe[I];
		USoundCue* Cue = Make<USoundCue>(Spec.Name, Assets);
		USoundNodeWavePlayer* Node = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
		Node->SetSoundWave(Waves[I]);
		Node->bLooping = Spec.bLoop;
		Cue->FirstNode = Node;
		Cue->SoundClassObject = Classes[Spec.ClassIndex];
		Cue->VolumeMultiplier = Spec.Gain;
		Cue->bOverrideAttenuation = false;
		// A muted music/engine layer retains its playback position and resumes smoothly.
		Cue->VirtualizationMode = Spec.bLoop ? EVirtualizationMode::PlayWhenSilent : EVirtualizationMode::Disabled;
		if (Spec.ClassIndex == 4) Cue->ConcurrencySet.Add(UIConcurrency);
		else if (Spec.Name.StartsWith(TEXT("SC_Step_")) || Spec.Name == TEXT("SC_Landing")) Cue->ConcurrencySet.Add(StepConcurrency);
		Cue->LinkGraphNodesFromSoundNodes();
		if (Cue->IsLooping() != Spec.bLoop)
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Audio] Loop metadata failed: %s; no outputs saved."), *Spec.Name);
			return 1;
		}
		Cues.Add(Spec.Name, Cue);
	}
	UAPSAudioBank* Bank = Make<UAPSAudioBank>(TEXT("DA_APSAudioBank"), Assets);
	Bank->Mix = Mix;
	Bank->MasterClass = Classes[0];
	Bank->MusicClass = Classes[1];
	Bank->AmbienceClass = Classes[2];
	Bank->EffectsClass = Classes[3];
	Bank->UIClass = Classes[4];
	Bank->UIClick = Cues[TEXT("SC_UI_Click")];
	Bank->UIHover = Cues[TEXT("SC_UI_Hover")];
	Bank->UIConfirm = Cues[TEXT("SC_UI_Confirm")];
	Bank->UIBack = Cues[TEXT("SC_UI_Back")];
	Bank->MenuMusic = Cues[TEXT("SC_Music_Menu")];
	Bank->ExplorationMusic = Cues[TEXT("SC_Music_Exploration")];
	// Exploration already supplies the background; avoid stacking continuous drones.
	Bank->SpaceAmbience = nullptr;
	Bank->InteriorAmbience = nullptr;
	Bank->ShipIdleLoop = Cues[TEXT("SC_Ship_Idle")];
	Bank->ShipThrustLoop = Cues[TEXT("SC_Ship_Thrust")];
	Bank->ShipWarpLoop = Cues[TEXT("SC_Ship_Warp")];
	Bank->EngineStart = Cues[TEXT("SC_Ship_Start")];
	Bank->EngineStop = Cues[TEXT("SC_Ship_Stop")];
	Bank->FlightModeChange = Cues[TEXT("SC_Ship_Mode")];
	Bank->Landing = Cues[TEXT("SC_Landing")];
	for (int32 I = 1; I <= 5; ++I)
	{
		Bank->DefaultFootsteps.Sounds.Add(Cues[FString::Printf(TEXT("SC_Step_Dirt_%02d"), I)]);
		Bank->MetalFootsteps.Sounds.Add(Cues[FString::Printf(TEXT("SC_Step_Metal_%02d"), I)]);
	}
	for (UObject* Asset : Assets)
	{
		if (!Save(Asset))
		{
			UE_LOG(LogTemp, Error, TEXT("[APS.Audio] Save failed: %s. Bank is saved last; inspect partial outputs."), *Asset->GetPathName());
			return 1;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("[APS.Audio] Created %d assets in %s; no source waves modified."), Assets.Num(), *Root);
	return Validate();
#else
	return 1;
#endif
}
