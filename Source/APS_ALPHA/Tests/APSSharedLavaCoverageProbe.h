#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS
class FAutomationTestBase;
class IAutomationLatentCommand;
class UWorld;
class APlanet;

// Test-only continuation of the existing generated/natural-landing fixture.
// The command restores its lease before the owning fixture drains WorldScape.
TSharedPtr<IAutomationLatentCommand> APSCreateSharedLavaCoverageProbe(
    FAutomationTestBase* Test, UWorld* World, APlanet* Planet);

// Same actual-WorldScape 2x2 coverage probe, opt-in Water/Ammonia only.
// These candidates inherit their saved shared MIC parameters, not legacy MIDs.
TSharedPtr<IAutomationLatentCommand> APSCreateSharedLiquidCoverageProbe(
    FAutomationTestBase* Test, UWorld* World, APlanet* Planet);
#endif
