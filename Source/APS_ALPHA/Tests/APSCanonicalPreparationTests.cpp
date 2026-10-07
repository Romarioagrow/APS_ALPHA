#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Generation/APSCanonicalFilteredHeight.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSCanonicalPreparationContracts,
    "APS.Contracts.PlanetSurface.CanonicalPreparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSCanonicalPreparationContracts::RunTest(const FString& Parameters)
{
    using namespace APSCanonicalFilteredHeight;
    APSCanonicalCoverageLayout::FLayout Layout;
    std::string LayoutError;
    const APSCanonicalCoverageLayout::FFrame Anchor{{0,0,1},{1,0,0},{0,1,0}};
    if (!TestTrue(TEXT("Canonical layout"), APSCanonicalCoverageLayout::Build(
        128000000.0,Anchor,Layout,LayoutError))) return false;
    APSClosedGlobeMesh::FSamplingFrame Frame;
    Frame.Radius=Layout.RadiusCm; Frame.NoiseScale=683; Frame.NoiseIntensity=908291;
    Frame.PresentationScale=1;
    FOptions Options; Options.InputSignature=123;
    FData Output; FString Error;
    std::atomic_bool Cancelled{true}; Options.Cancellation=&Cancelled;
    Options.MaxWorkers=2;
    TestFalse(TEXT("Already cancelled build rejected before sampling"),
        BuildLevel(Frame,Layout,0,Options,Output,Error));
    TestTrue(TEXT("Cancellation has explicit reason"),Error.Contains(TEXT("cancelled")));
    TestTrue(TEXT("Cancellation leaves caller output untouched"),
        Output.Mips.IsEmpty() && Output.ByteCount==0 && Output.Metadata.Level==-1);
    Cancelled.store(false);
    for (int32 InvalidWorkers : {-1,9})
    {
        Options.MaxWorkers=InvalidWorkers;
        TestFalse(TEXT("Unbounded worker policy rejected"),BuildLevel(Frame,Layout,0,Options,Output,Error));
        TestTrue(TEXT("Rejected policy leaves output untouched"),Output.Mips.IsEmpty() && Output.Metadata.Level==-1);
    }
    Options.MaxWorkers=2; Options.HeightFilter=EHeightFilter::SharedLattice;
    for (int32 InvalidOversample : {0,1,3,5})
    {
        Options.Oversample=InvalidOversample;
        TestFalse(TEXT("Unsupported shared lattice density rejected"),BuildLevel(Frame,Layout,0,Options,Output,Error));
        TestTrue(TEXT("Rejected lattice leaves output untouched"),Output.Mips.IsEmpty() && Output.Metadata.Level==-1);
    }
    Options.Oversample=4; Cancelled.store(true);
    TestFalse(TEXT("Shared lattice respects pre-cancellation"),BuildLevel(Frame,Layout,0,Options,Output,Error));
    TestTrue(TEXT("Shared lattice cancellation has explicit reason"),Error.Contains(TEXT("cancelled")));
    TestTrue(TEXT("Runtime extraction preserves analytic gradient contracts"),GradientContractsPass(Layout,0));
    return true; // Lifecycle/render/readiness are separate integration evidence.
}
#endif
