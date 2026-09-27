#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Editor/APSSharedTerrainDetailPrecisionUpdate.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSSharedTerrainPrecisionPreflightTest,
    "APS.Materials.SharedTerrain.PrecisionPositionPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPSSharedTerrainPrecisionPreflightTest::RunTest(const FString& Parameters)
{
    using namespace APSSharedTerrainDetailPrecisionUpdate;
    TStrongObjectPtr<UMaterialFunction> Owner(NewObject<UMaterialFunction>(GetTransientPackage()));
    const auto Frame = [&]()
    {
        auto* World = NewObject<UMaterialExpressionWorldPosition>(Owner.Get());
        World->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
        auto* XY = NewObject<UMaterialExpressionComponentMask>(Owner.Get());
        XY->Input.Connect(0, World); XY->R = XY->G = true; XY->B = XY->A = false;
        auto* Z = NewObject<UMaterialExpressionComponentMask>(Owner.Get());
        Z->Input.Connect(0, World); Z->B = true; Z->R = Z->G = Z->A = false;
        auto* Result = NewObject<UMaterialExpressionAppendVector>(Owner.Get());
        Result->A.Connect(0, XY); Result->B.Connect(0, Z);
        return Result;
    };
    FExpressionInput Default;
    Default.Connect(0, Frame());
    auto* Position = Frame();
    auto* Zero = NewObject<UMaterialExpressionConstant3Vector>(Owner.Get());
    Zero->Constant = FLinearColor::Black;
    auto* Explicit = NewObject<UMaterialExpressionSubtract>(Owner.Get());
    Explicit->A.Connect(0, Position); Explicit->A.Mask = Explicit->A.MaskR = Explicit->A.MaskG = Explicit->A.MaskB = 1;
    Explicit->B.Connect(0, Zero);
    FExpressionInput Binding; Binding.Connect(0, Explicit);
    TestTrue(TEXT("Independent equivalent XYZ frame minus zero is accepted"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    Zero->Constant.R = 1.0f;
    TestFalse(TEXT("Authored nonzero offset is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    Zero->Constant.R = 0.0f;
    Explicit->A.MaskB = 0;
    TestFalse(TEXT("Dropped coordinate channel is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    Explicit->A.MaskB = 1;
    auto* XY = CastChecked<UMaterialExpressionComponentMask>(Position->A.Expression);
    auto* World = CastChecked<UMaterialExpressionWorldPosition>(XY->Input.Expression);
    World->WorldPositionShaderOffset = WPT_Default;
    TestFalse(TEXT("Different world position offset convention is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    World->WorldPositionShaderOffset = WPT_ExcludeAllShaderOffsets;
    XY->R = false;
    TestFalse(TEXT("Different projection is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    XY->R = true;
    Binding.OutputIndex = 1;
    TestFalse(TEXT("Different output is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    Binding.OutputIndex = 0;
    TestFalse(TEXT("Different owner is rejected"), IsEquivalentMacroPosition(Binding, Default, GetTransientPackage()));
    // A cyclic graph cannot consume unbounded preflight time.
    Position->A.Connect(0, Position);
    TestFalse(TEXT("Cyclic or differently shaped graph is rejected"), IsEquivalentMacroPosition(Binding, Default, Owner.Get()));
    return true;
}
#endif
