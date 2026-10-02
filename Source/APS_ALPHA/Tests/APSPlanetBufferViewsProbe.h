#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "BufferVisualizationData.h"
#include "Engine/GameViewportClient.h"
#include "HAL/IConsoleManager.h"
#include "Tests/AutomationCommon.h"

// A causal inspection of the bound production material, not a replacement
// material or an acceptance test. Restore the viewport even on an early failure.
namespace APSPlanetBufferViews
{
    inline bool Requested() { return FParse::Param(FCommandLine::Get(), TEXT("APSProbeTerrainBuffers")); }
    class FScope
    {
        TWeakObjectPtr<UGameViewportClient> Client;
        FEngineShowFlags SavedFlags{ESFIM_Game};
        int32 SavedViewMode = VMI_Lit;
        FString SavedTarget;
        bool bActive = false;
    public:
        static constexpr int32 Count = 5;
        static const TCHAR* Label(int32 Index)
        {
            static const TCHAR* Names[] = {TEXT("lit"), TEXT("base-color"), TEXT("world-normal"), TEXT("roughness"), TEXT("lit-restored")};
            return Index >= 0 && Index < Count ? Names[Index] : TEXT("invalid");
        }
        ~FScope() { Restore(); }
        bool Begin(FString& Error)
        {
            if (bActive) return true;
            auto* V = AutomationCommon::GetAnyGameViewportClient();
            auto* Target = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
            if (!V || !Target || V->ViewModeIndex != VMI_Lit)
            { Error = TEXT("Buffer isolation needs the normal lit game viewport and target cvar"); return false; }
            for (const TCHAR* Name : {TEXT("BaseColor"), TEXT("WorldNormal"), TEXT("Roughness")})
                if (!GetBufferVisualizationData().GetMaterial(FName(Name)))
                { Error = FString(TEXT("Missing engine buffer visualization material: ")) + Name; return false; }
            Client = V; SavedFlags = V->EngineShowFlags; SavedViewMode = V->ViewModeIndex;
            SavedTarget = Target->GetString();
            bActive = true;
            return true;
        }
        bool Apply(int32 Index, FString& Error)
        {
            auto* V = Client.Get();
            auto* Target = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
            if (!bActive || !V || !Target || Index < 0 || Index >= Count)
            { Error = TEXT("Buffer isolation lost its viewport or has an invalid index"); return false; }
            if (Index == 0 || Index == Count - 1)
            {
                V->ViewModeIndex = SavedViewMode; V->EngineShowFlags = SavedFlags;
                Target->Set(*SavedTarget, ECVF_SetByCode);
            }
            else
            {
                static const TCHAR* Targets[] = {TEXT(""), TEXT("BaseColor"), TEXT("WorldNormal"), TEXT("Roughness")};
                V->ViewModeIndex = VMI_VisualizeBuffer;
                ApplyViewMode(VMI_VisualizeBuffer, true, V->EngineShowFlags);
                Target->Set(Targets[Index], ECVF_SetByCode);
            }
            UE_LOG(LogTemp, Display, TEXT("PLANET_BUFFER_ISOLATION stage=%s materialUnchanged=1 viewMode=%d target=%s"),
                Label(Index), V->ViewModeIndex, *Target->GetString());
            return true;
        }
        bool Validate(int32 Index, FString& Error) const
        {
            const auto* V = Client.Get();
            const int32 Expected = Index == 0 || Index == Count - 1 ? SavedViewMode : VMI_VisualizeBuffer;
            if (!bActive || !V || V->ViewModeIndex != Expected)
            { Error = TEXT("Buffer isolation view mode drifted"); return false; }
            if (Index > 0 && Index < Count - 1)
            {
                static const TCHAR* Targets[] = {TEXT(""), TEXT("BaseColor"), TEXT("WorldNormal"), TEXT("Roughness")};
                const auto* Target = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationTarget"));
                // GameViewportClient::Draw consumes this CVar every frame. Its
                // cached buffer mode is protected; do not bypass the public API.
                if (!V->EngineShowFlags.VisualizeBuffer || !Target || Target->GetString() != Targets[Index])
                { Error = TEXT("Requested buffer target drifted; reject mislabeled capture"); return false; }
            }
            return true;
        }
        void Restore()
        {
            if (!bActive) return;
            FString Error;
            if (!Apply(Count - 1, Error)) UE_LOG(LogTemp, Warning, TEXT("%s"), *Error);
            bActive = false;
        }
    };
}
#endif
