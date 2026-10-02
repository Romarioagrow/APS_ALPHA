#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "APS_ALPHA/Core/Audio/APSShipAudioMixPolicy.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShipAudioMixLayersTest, "APS.Audio.ShipMix.Layers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShipAudioMixLayersTest::RunTest(const FString& Parameters)
{
    using namespace APSShipAudioMix;
    FInput Input;
    Input.bRunning = true;
    const auto Idle = Resolve(Input);
    TestTrue(TEXT("Running engine retains an idle bed"), Idle.IdleGain > 0.f);
    TestEqual(TEXT("No demand cannot invent thrust from coasting"),
        Idle.ForwardGain + Idle.LiftGain + Idle.BoostGain + Idle.WarpGain, 0.f);

    Input.ForwardDemand = 1.f;
    const auto Forward = Resolve(Input);
    TestTrue(TEXT("Forward demand opens the forward layer"), Forward.ForwardGain > 0.f);
    TestEqual(TEXT("Forward does not imply lift or boost"), Forward.LiftGain + Forward.BoostGain, 0.f);
    Input.ForwardDemand = -1.f;
    const auto Reverse = Resolve(Input);
    TestTrue(TEXT("Reverse is audible but below full forward"), Reverse.ForwardGain > 0.f && Reverse.ForwardGain < Forward.ForwardGain);
    TestTrue(TEXT("Reverse has a lower pitch"), Reverse.ForwardPitch < Forward.ForwardPitch);
    Input.ForwardDemand = 0.f;
    Input.LateralDemand = 1.f;
    const auto Side = Resolve(Input);
    TestTrue(TEXT("Lateral demand adds bounded manoeuvring thrust"), Side.ForwardGain > 0.f && Side.ForwardGain < Forward.ForwardGain);
    Input.LateralDemand = -1.f;
    TestEqual(TEXT("Left and right use the same level"), Resolve(Input).ForwardGain, Side.ForwardGain);

    Input.LateralDemand = 0.f;
    Input.VerticalDemand = 1.f;
    const auto Lift = Resolve(Input);
    TestTrue(TEXT("Positive vertical demand has its own layer"), Lift.LiftGain > 0.f);
    TestEqual(TEXT("Lift does not imply forward or boost"), Lift.ForwardGain + Lift.BoostGain, 0.f);
    TestTrue(TEXT("Lift has a distinct lower pitch"), Lift.LiftPitch < Forward.ForwardPitch);
    Input.VerticalDemand = -1.f;
    const auto Down = Resolve(Input);
    TestTrue(TEXT("Downward thrust stays audible with a lower timbre"), Down.LiftGain > 0.f
        && Down.LiftGain < Lift.LiftGain && Down.LiftPitch < Lift.LiftPitch);

    Input.VerticalDemand = 0.f;
    Input.AppliedBoost = 1.f;
    const auto Boost = Resolve(Input);
    TestTrue(TEXT("Boost is distinct while held at rest"), Boost.BoostGain > 0.f);
    TestEqual(TEXT("Boost does not fake translation demand"), Boost.ForwardGain + Boost.LiftGain, 0.f);
    Input.ForwardDemand = Input.VerticalDemand = Input.LateralDemand = 1.f;
    Input.Band = 4;
    const auto Combined = Resolve(Input);
    TestTrue(TEXT("Combined demands preserve every requested layer"), Combined.IdleGain > 0.f
        && Combined.ForwardGain > 0.f && Combined.LiftGain > 0.f && Combined.BoostGain > 0.f && Combined.WarpGain > 0.f);
    TestTrue(TEXT("Combined layers respect the shared gain budget"), Combined.TotalGain() <= MaximumTotalGain);
    Input.bRunning = false;
    TestEqual(TEXT("Engine off silences every layer regardless of controls"), Resolve(Input).TotalGain(), 0.f);
    Input.bRunning = true;
    const auto Restarted = Resolve(Input);
    TestEqual(TEXT("Restart restores idle demand"), Restarted.IdleGain, Combined.IdleGain);
    TestEqual(TEXT("Restart restores forward demand"), Restarted.ForwardGain, Combined.ForwardGain);
    TestEqual(TEXT("Restart restores lift demand"), Restarted.LiftGain, Combined.LiftGain);
    TestEqual(TEXT("Restart restores boost demand"), Restarted.BoostGain, Combined.BoostGain);
    TestEqual(TEXT("Restart restores warp demand"), Restarted.WarpGain, Combined.WarpGain);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShipAudioMixIndependentDemandsTest, "APS.Audio.ShipMix.IndependentDemands",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShipAudioMixIndependentDemandsTest::RunTest(const FString& Parameters)
{
    using namespace APSShipAudioMix;
    FInput Input;
    Input.bRunning = true;
    Input.ForwardDemand = .5f;
    Input.LateralDemand = -.25f;
    Input.VerticalDemand = .4f;
    Input.AppliedBoost = .2f;
    Input.Band = 2;
    const auto Mixed = Resolve(Input);
    TestTrue(TEXT("Independent partial forward and side demands retain their mix"),
        FMath::IsNearlyEqual(Mixed.ForwardGain, .34075f, .00001f));
    TestTrue(TEXT("Partial lift is not replaced by forward demand"),
        FMath::IsNearlyEqual(Mixed.LiftGain, .185f, .00001f));
    TestTrue(TEXT("Partial boost remains separate"),
        FMath::IsNearlyEqual(Mixed.BoostGain, .116f, .00001f));
    TestTrue(TEXT("An under-budget mix is not amplified to the ceiling"),
        FMath::IsNearlyEqual(Mixed.TotalGain(), .87425f, .00001f));

    Input.ForwardDemand = 0.f;
    const auto WithoutForward = Resolve(Input);
    Input.ForwardDemand = std::numeric_limits<float>::quiet_NaN();
    const auto BadForward = Resolve(Input);
    TestEqual(TEXT("One invalid channel does not silence valid side thrust"), BadForward.ForwardGain, WithoutForward.ForwardGain);
    TestEqual(TEXT("One invalid channel does not silence lift"), BadForward.LiftGain, WithoutForward.LiftGain);
    TestEqual(TEXT("One invalid channel does not silence boost"), BadForward.BoostGain, WithoutForward.BoostGain);

    Input.ForwardDemand = -.6f;
    Input.LateralDemand = .8f;
    Input.VerticalDemand = -.9f;
    Input.AppliedBoost = .7f;
    Input.Band = 4;
    const auto Limited = Resolve(Input);
    // Pre-budget gains: idle .245, forward .438, lift .363375,
    // boost .406, warp .1606. Their relative balance must survive limiting.
    const float Scale = Limited.IdleGain / .245f;
    TestTrue(TEXT("Independent simultaneous demands exercise the shared budget"), Scale > 0.f && Scale < 1.f);
    TestTrue(TEXT("Forward shares the common gain scale"), FMath::IsNearlyEqual(Limited.ForwardGain / .438f, Scale, .00001f));
    TestTrue(TEXT("Lift shares the common gain scale"), FMath::IsNearlyEqual(Limited.LiftGain / .363375f, Scale, .00001f));
    TestTrue(TEXT("Boost shares the common gain scale"), FMath::IsNearlyEqual(Limited.BoostGain / .406f, Scale, .00001f));
    TestTrue(TEXT("Warp shares the common gain scale"), FMath::IsNearlyEqual(Limited.WarpGain / .1606f, Scale, .00001f));
    TestTrue(TEXT("Limited mixed demands remain below the ceiling"), Limited.TotalGain() <= MaximumTotalGain);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPSShipAudioMixBoundsTest, "APS.Audio.ShipMix.BoundsAndModes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAPSShipAudioMixBoundsTest::RunTest(const FString& Parameters)
{
    using namespace APSShipAudioMix;
    FInput Input;
    Input.bRunning = true;
    Input.ForwardDemand = .5f;
    float PreviousPitch = Resolve(Input).ForwardPitch;
    for (int32 Band = 0; Band <= 4; ++Band)
    {
        Input.Band = Band;
        const auto Out = Resolve(Input);
        TestTrue(TEXT("Band changes are small bounded pitch steps"), Out.ForwardPitch >= PreviousPitch
            && Out.ForwardPitch - PreviousPitch < .04f);
        TestEqual(TEXT("Only Cruise and Stellar add warp under demand"), Out.WarpGain > 0.f, Band >= 3);
        PreviousPitch = Out.ForwardPitch;
    }
    Input.Band = 0;
    const auto Low = Resolve(Input);
    Input.Band = 4;
    TestTrue(TEXT("Flight bands vary the pitch preset"), Resolve(Input).ForwardPitch > Low.ForwardPitch);
    Input.ForwardDemand = 0.f;
    TestEqual(TEXT("High band alone does not create powered thrust"),
        Resolve(Input).ForwardGain + Resolve(Input).LiftGain + Resolve(Input).BoostGain + Resolve(Input).WarpGain, 0.f);

    const float NaN = std::numeric_limits<float>::quiet_NaN();
    const float Inf = std::numeric_limits<float>::infinity();
    for (float Bad : {NaN, Inf, -Inf})
    {
        Input.ForwardDemand = Input.LateralDemand = Input.VerticalDemand = Input.AppliedBoost = Bad;
        const auto Out = Resolve(Input);
        TestEqual(TEXT("Invalid telemetry cannot command thrust"), Out.ForwardGain + Out.LiftGain + Out.BoostGain + Out.WarpGain, 0.f);
    }
    for (int32 Band : {MIN_int32, 0, 1, 2, 3, 4, MAX_int32})
        for (float Demand : {-2.f, -1.f, 0.f, .5f, 1.f, 2.f})
    {
        Input.Band = Band;
        Input.ForwardDemand = Input.LateralDemand = Input.VerticalDemand = Input.AppliedBoost = Demand;
        const auto Out = Resolve(Input);
        TestTrue(TEXT("Finite telemetry stays within the combined gain ceiling"),
            FMath::IsFinite(Out.TotalGain()) && Out.TotalGain() <= MaximumTotalGain);
        for (float Gain : {Out.IdleGain, Out.ForwardGain, Out.LiftGain, Out.BoostGain, Out.WarpGain})
            TestTrue(TEXT("Every gain is finite and bounded"), FMath::IsFinite(Gain) && Gain >= 0.f && Gain <= 1.f);
        for (float Pitch : {Out.IdlePitch, Out.ForwardPitch, Out.LiftPitch, Out.BoostPitch, Out.WarpPitch})
            TestTrue(TEXT("Every pitch is finite and bounded"), FMath::IsFinite(Pitch) && Pitch >= .65f && Pitch <= 1.5f);
        if (Demand < 0.f) TestEqual(TEXT("Negative applied boost cannot enable boost audio"), Out.BoostGain, 0.f);
    }
    return true;
}
#endif
