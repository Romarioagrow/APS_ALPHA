// Rio 08.10, packaged 0.6.0-alpha had no planet surfaces: the cook failed M_APS_SharedWorldScapeTerrain,
// M_APS_ContinuousTerrain and M_APS_UnifiedLavaSurface with "Function (...) call input with index 0 is unset".
// FFunctionExpressionInput::ExpressionInput is UPROPERTY(transient) and only exists after
// UMaterialExpressionMaterialFunctionCall::UpdateFromFunctionResource. The APS surface tools hydrated those pins in
// memory and saved, so a cold load in the cook leaves them null, while the editor keeps using DDC shader maps and
// never recompiles. In the cook only, refresh every loaded material/function's call nodes before its shaders are
// compiled, as the material editor does when it opens a graph. Source assets are never saved by this path.

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Misc/DelayedAutoRegister.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectHash.h"

namespace APSCookMaterialHydration
{
	static TAutoConsoleVariable<int32> CVarHydrate(
		TEXT("aps.Cook.HydrateMaterialFunctions"), 1,
		TEXT("1: while cooking, refresh material function call nodes after load so their transient input links exist\n")
		TEXT("before the cooked shader maps are compiled (packaged planet surfaces). 0: engine default."),
		ECVF_Default);

	int32 GHydrated = 0;

	void OnAssetLoaded(UObject* Object)
	{
		if (!IsRunningCookCommandlet() || CVarHydrate.GetValueOnAnyThread() == 0 || !IsValid(Object))
		{
			return;
		}
		if (!Object->IsA<UMaterialFunctionInterface>() && !Object->IsA<UMaterial>())
		{
			return;
		}
		// Every call node in the package, not only the graph list: the patched Slope functions wire call nodes in
		// through tool-made Custom nodes, and the compile reaches them by those wires.
		TArray<UMaterialExpressionMaterialFunctionCall*> Calls;
		ForEachObjectWithPackage(Object->GetPackage(), [&Calls](UObject* Inner)
		{
			if (UMaterialExpressionMaterialFunctionCall* Call = Cast<UMaterialExpressionMaterialFunctionCall>(Inner))
			{
				Calls.Add(Call);
			}
			return true;
		});
		for (UMaterialExpressionMaterialFunctionCall* Call : Calls)
		{
			Call->UpdateFromFunctionResource(false);
		}
		if (GHydrated++ == 0)
		{
			UE_LOG(LogTemp, Display, TEXT("[APS.CookHydrate] refreshing material function call nodes after load (first: %s)"),
				*Object->GetPathName());
		}
	}

	// Commandlets (the cook) run inside engine PreInit, before EndOfEngineInit; a passed phase runs at once.
	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::ObjectSystemReady, []
	{
		FCoreUObjectDelegates::OnAssetLoaded.AddStatic(&OnAssetLoaded);
	});
}

#endif // WITH_EDITOR
