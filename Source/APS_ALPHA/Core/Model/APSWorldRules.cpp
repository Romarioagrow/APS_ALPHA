#include "APSWorldRules.h"

#include "APS_ALPHA/Core/Instances/MainGameplayInstance.h"
#include "APS_ALPHA/Core/Model/GeneratedWorld.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace APSWorldRulesLocal
{
	/** The master switch of the ORIGIN set (IMPLEMENTATION_PLAN §1): 0 reads every world as open reach. */
	TAutoConsoleVariable<int32> CVarEnable(TEXT("aps.Origin.Enable"), 1,
		TEXT("Rio 07.10: the ORIGIN ladder and its gates apply to worlds that ask for them (1); 0 plays every world as today."),
		ECVF_Default);

	template <typename TEnum>
	TEnum Byte(const uint8 Value, const uint8 Count)
	{
		return static_cast<TEnum>(Value < Count ? Value : 0);
	}
}

namespace APSWorldRules
{
	FRules Of(const UGeneratedWorld* World)
	{
		FRules Rules;
		if (!IsValid(World))
		{
			return Rules;
		}
		Rules.Mode = APSWorldRulesLocal::Byte<EMode>(World->RulesMode, 4);
		Rules.Origin = APSWorldRulesLocal::Byte<EOrigin>(World->RulesOrigin, 5);
		Rules.Reach = APSWorldRulesLocal::Byte<EReach>(World->RulesReach, 2);
		Rules.Knowledge = APSWorldRulesLocal::Byte<EKnowledge>(World->RulesKnowledge, 3);
		Rules.Goals = APSWorldRulesLocal::Byte<EGoals>(World->RulesGoals, 2);
		Rules.Resources = APSWorldRulesLocal::Byte<EResources>(World->RulesResources, 3);
		Rules.Others = APSWorldRulesLocal::Byte<EOthers>(World->RulesOthers, 4);
		Rules.Sky = APSWorldRulesLocal::Byte<ESky>(World->RulesSky, 2);
		Rules.AuthoredWorldId = World->AuthoredWorldId;
		Rules.AuthoredLocks = World->AuthoredLocks;
		return Rules;
	}

	FRules OfGame(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		const UMainGameplayInstance* Gameplay = GameInstance ? GameInstance->GetSubsystem<UMainGameplayInstance>() : nullptr;
		if (!Gameplay)
		{
			return FRules();
		}
		const UGeneratedWorld* Model = Gameplay->NewGeneratedWorld;
		return Of(Model);
	}

	void Write(UGeneratedWorld& World, const FRules& Rules)
	{
		World.RulesMode = static_cast<uint8>(Rules.Mode);
		World.RulesOrigin = static_cast<uint8>(Rules.Origin);
		World.RulesReach = static_cast<uint8>(Rules.Reach);
		World.RulesKnowledge = static_cast<uint8>(Rules.Knowledge);
		World.RulesGoals = static_cast<uint8>(Rules.Goals);
		World.RulesResources = static_cast<uint8>(Rules.Resources);
		World.RulesOthers = static_cast<uint8>(Rules.Others);
		World.RulesSky = static_cast<uint8>(Rules.Sky);
		World.AuthoredWorldId = Rules.AuthoredWorldId;
		World.AuthoredLocks = Rules.AuthoredLocks;
	}

	void ApplyPreset(UGeneratedWorld& World, const EMode Mode)
	{
		FRules Rules = Of(&World);
		Rules.Mode = Mode;
		switch (Mode)
		{
		case EMode::Sandbox:
			Rules.Origin = EOrigin::None;
			Rules.Reach = EReach::Open;
			Rules.Knowledge = EKnowledge::AsToday;
			Rules.Goals = EGoals::Civilization;
			Rules.Resources = EResources::Normal;
			Rules.Others = EOthers::Alone;
			break;
		case EMode::Origin:
			if (Rules.Origin == EOrigin::None)
			{
				Rules.Origin = EOrigin::Ark;
			}
			Rules.Reach = EReach::Ladder;
			Rules.Knowledge = EKnowledge::Unknown;
			Rules.Goals = EGoals::Civilization;
			Rules.Resources = EResources::Normal;
			Rules.Others = EOthers::Traces;
			break;
		case EMode::SpaceTrips:
			Rules.Origin = EOrigin::None;
			Rules.Reach = EReach::Open;
			Rules.Knowledge = EKnowledge::AllKnown;
			Rules.Goals = EGoals::None;
			Rules.Resources = EResources::Normal;
			Rules.Others = EOthers::Traces;
			break;
		case EMode::Custom:
		default:
			break;
		}
		Write(World, Rules);
	}

	bool IsLadder(const UWorld* World)
	{
		return APSWorldRulesLocal::CVarEnable.GetValueOnGameThread() != 0 && OfGame(World).Reach == EReach::Ladder;
	}

	bool IsTrip(const UWorld* World)
	{
		return OfGame(World).Goals == EGoals::None;
	}

	const TCHAR* ModeName(const EMode Mode)
	{
		switch (Mode)
		{
		case EMode::Origin: return TEXT("ORIGIN");
		case EMode::SpaceTrips: return TEXT("SPACE TRIPS");
		case EMode::Custom: return TEXT("CUSTOM");
		case EMode::Sandbox:
		default: return TEXT("SANDBOX");
		}
	}

	const TCHAR* OriginName(const EOrigin Origin)
	{
		switch (Origin)
		{
		case EOrigin::Ark: return TEXT("ARK");
		case EOrigin::Adrift: return TEXT("ADRIFT");
		case EOrigin::UnderTheRing: return TEXT("UNDER THE RING");
		case EOrigin::Exodus: return TEXT("EXODUS");
		case EOrigin::None:
		default: return TEXT("NONE");
		}
	}

	const TCHAR* KnowledgeName(const EKnowledge Knowledge)
	{
		switch (Knowledge)
		{
		case EKnowledge::AllKnown: return TEXT("ALL KNOWN");
		case EKnowledge::Unknown: return TEXT("UNKNOWN");
		case EKnowledge::AsToday:
		default: return TEXT("HOME KNOWN");
		}
	}

	const TCHAR* ResourcesName(const EResources Resources)
	{
		switch (Resources)
		{
		case EResources::Unlimited: return TEXT("UNLIMITED");
		case EResources::Scarce: return TEXT("SCARCE");
		case EResources::Normal:
		default: return TEXT("NORMAL");
		}
	}

	const TCHAR* OthersName(const EOthers Others)
	{
		switch (Others)
		{
		case EOthers::Traces: return TEXT("TRACES");
		case EOthers::Neighbours: return TEXT("NEIGHBOURS");
		case EOthers::Unknown: return TEXT("UNKNOWN");
		case EOthers::Alone:
		default: return TEXT("ALONE");
		}
	}

	FString Describe(const FRules& Rules)
	{
		if (Rules.IsToday() && Rules.AuthoredWorldId.IsEmpty())
		{
			return FString();
		}
		FString Text = FString::Printf(TEXT("MODE=%d|ORIGIN=%d|REACH=%d|KNOW=%d|GOALS=%d|RES=%d|OTHERS=%d|SKY=%d"),
			static_cast<int32>(Rules.Mode), static_cast<int32>(Rules.Origin), static_cast<int32>(Rules.Reach),
			static_cast<int32>(Rules.Knowledge), static_cast<int32>(Rules.Goals), static_cast<int32>(Rules.Resources),
			static_cast<int32>(Rules.Others), static_cast<int32>(Rules.Sky));
		if (!Rules.AuthoredWorldId.IsEmpty())
		{
			Text += FString::Printf(TEXT("|WORLD=%s|LOCKS=%d"), *Rules.AuthoredWorldId, Rules.AuthoredLocks);
		}
		return Text;
	}
}
