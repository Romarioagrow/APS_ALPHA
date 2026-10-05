#pragma once

#include "APSCanonicalCoverageLayout.h"
#include "APS_ALPHA/Generation/APSClosedGlobeMesh.h"
#include "APS_ALPHA/Generation/APSWorldScapePlanetNoise.h"
#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include "Async/ParallelFor.h"
#include "HAL/PlatformTime.h"
#include "Math/Float16Color.h"
#include <atomic>

// Shared CPU producer. No UObject access in workers, texture upload, material
// binding, camera policy or changes to the authoritative noise implementation.
// Optional shared-lattice prefilter attenuates height before decimation and
// differentiation. Neither finite filter is a strict spectral cutoff or a
// convergence certificate. Compare independent sampling densities and frames.
namespace APSCanonicalFilteredHeight
{
    constexpr int32 MaximumResolution = 1024, MaximumLevels = 9;
    constexpr double SigmaInLocalTexels = .75;
    enum class EQuadrature : uint8 { Three = 3, Five = 5 };
    enum class EHeightFilter : uint8 { Quadrature, SharedLattice };
    struct FOptions
    {
        EQuadrature Quadrature = EQuadrature::Three;
        EHeightFilter HeightFilter = EHeightFilter::Quadrature;
        int32 Oversample = 4;
        uint32 InputSignature = 0;
        // Borrowed only for the synchronous call; the async owner retains it.
        const std::atomic_bool* Cancellation = nullptr;
        // 0 preserves the diagnostic scheduler. Runtime uses a bounded lane count.
        int32 MaxWorkers = 0;
    };
    struct FMetadata
    {
        APSCanonicalCoverageLayout::FLayout Layout;
        int32 Level = -1, QuadratureOrder = 0;
        EHeightFilter HeightFilter = EHeightFilter::Quadrature;
        int32 Oversample = 0;
        uint32 InputSignature = 0;
        EPlanetType PlanetType = EPlanetType::Unknown;
        int32 TerrainSeed = 0, BiomeSeed = 0;
        double NoiseScale = 0, NoiseIntensity = 0, EnvelopeSeaHeightCm = 0;
        double WidthCm = 0, CenterTexelSpacingCm = 0, SigmaTexels = SigmaInLocalTexels;
        double MinimumLocalSpacingCm = 0, MaximumLocalSpacingCm = 0;
        double MinimumFilteredHeightCm = 0, MaximumFilteredHeightCm = 0, MaximumSlope = 0;
        double PreparationWallSeconds = 0;
        int64 MaximumHeightSamples = 0, ActualHeightSamples = 0;
        bool bCoastalReliefCandidate = false, bNativeLavaEnvelope = false;
        bool bProvenBandLimited = false, bConvergenceValidated = false;
    };
    struct FMip
    {
        int32 Size = 0;
        // RGB signed dimensionless planet-space tangent-slope residual, A=1.
        // NOT a unit normal; never normalise RGB when generating/filtering mips.
        // Shader: g-=D*dot(D,g); normal=normalize(D-g), then rotate to world.
        TArray<FFloat16Color> Pixels;
    };
    struct FData { FMetadata Metadata; TArray<FMip> Mips; int64 ByteCount = 0; };
    struct FComparison
    {
        int64 Samples = 0;
        double MeanDegrees = 0, RmsDegrees = 0, P95Degrees = 0, MaximumDegrees = 0;
        // These are observed differences between sampled filters, not bounds against
        // the exact continuous integral. No automatic quality acceptance.
    };

    namespace Private
    {
        inline FVector3d Vector(const APSCanonicalCoverageLayout::FVector3& V) { return {V.X,V.Y,V.Z}; }
        inline bool LayoutValid(const APSCanonicalCoverageLayout::FLayout& L, int32 Level)
        {
            if (!FMath::IsFinite(L.RadiusCm) || L.RadiusCm <= 0 || L.Resolution < 2
                || L.Resolution > MaximumResolution || !FMath::IsPowerOfTwo(L.Resolution)
                || L.Count < 1 || L.Count > MaximumLevels || Level < 0 || Level >= L.Count) return false;
            const FVector3d C=Vector(L.Frame.Center), U=Vector(L.Frame.AxisU), V=Vector(L.Frame.AxisV);
            if (C.ContainsNaN() || U.ContainsNaN() || V.ContainsNaN()
                || !FMath::IsNearlyEqual(C.SizeSquared(),1.0,1.e-8)
                || !FMath::IsNearlyEqual(U.SizeSquared(),1.0,1.e-8)
                || !FMath::IsNearlyEqual(V.SizeSquared(),1.0,1.e-8)
                || !FVector3d::CrossProduct(U,V).Equals(C,1.e-8)) return false;
            const auto& S=L.Levels[Level];
            return FMath::IsFinite(S.WidthCm) && S.WidthCm > 0 && S.WidthCm <= 4.0*L.RadiusCm
                && S.CenterTexelSpacingCm == S.WidthCm/L.Resolution;
        }
        inline bool Geometry(const APSCanonicalCoverageLayout::FLayout& L, int32 Level,
            double UvX, double UvY, FVector3d& D, FVector3d& Tx, FVector3d& Ty, double& Q, double& Step)
        {
            APSCanonicalCoverageLayout::FVector3 Raw;
            if (!APSCanonicalCoverageLayout::Direction(L,Level,UvX,UvY,Raw)
                || !APSCanonicalCoverageLayout::LocalTexelSpacing(L,Level,UvX,UvY,Step)) return false;
            D=Vector(Raw);
            const double X=(UvX-.5)*L.Levels[Level].WidthCm/(2.0*L.RadiusCm);
            const double Y=(UvY-.5)*L.Levels[Level].WidthCm/(2.0*L.RadiusCm);
            Q=1.0+X*X+Y*Y;
            const FVector3d C=Vector(L.Frame.Center);
            // Orthonormal stereo tangent frame: Dx=(2/Q)Tx, Dy=(2/Q)Ty.
            Tx=Vector(L.Frame.AxisU)-X*(C+D); Ty=Vector(L.Frame.AxisV)-Y*(C+D);
            return !D.ContainsNaN() && !Tx.ContainsNaN() && !Ty.ContainsNaN()
                && FMath::IsFinite(Q) && Q > 0 && FMath::IsFinite(Step) && Step > 0
                && FMath::IsNearlyEqual(D.SizeSquared(),1.0,1.e-8)
                && FMath::IsNearlyEqual(Tx.SizeSquared(),1.0,1.e-8)
                && FMath::IsNearlyEqual(Ty.SizeSquared(),1.0,1.e-8)
                && FMath::Abs(FVector3d::DotProduct(Tx,Ty)) < 1.e-8;
        }
        struct FKernel { int32 Count=0; double Nodes[5]={},Weights[5]={}; };
        inline bool Kernel(EQuadrature Rule, FKernel& K)
        {
            K={};
            if (Rule==EQuadrature::Three)
            {
                K.Count=3; K.Nodes[0]=-FMath::Sqrt(3.0); K.Nodes[2]=-K.Nodes[0];
                K.Weights[0]=K.Weights[2]=1.0/6.0; K.Weights[1]=2.0/3.0;
            }
            else if (Rule==EQuadrature::Five)
            {
                const double S=FMath::Sqrt(10.0);
                K.Count=5; K.Nodes[0]=-FMath::Sqrt(5.0+S); K.Nodes[1]=-FMath::Sqrt(5.0-S);
                K.Nodes[3]=-K.Nodes[1]; K.Nodes[4]=-K.Nodes[0];
                K.Weights[0]=K.Weights[4]=(7.0-2.0*S)/60.0;
                K.Weights[1]=K.Weights[3]=(7.0+2.0*S)/60.0; K.Weights[2]=8.0/15.0;
            }
            else return false;
            double Sum=0; for (int32 I=0; I<K.Count; ++I) Sum+=K.Weights[I];
            return FMath::IsNearlyEqual(Sum,1.0,1.e-14);
        }

        template<typename THeight>
        bool FilterHeight(const APSCanonicalCoverageLayout::FLayout& L, int32 Level,
            double X, double Y, const FKernel& K, THeight Height, double& Out, double& Step)
        {
            FVector3d D,Tx,Ty; double Q=0;
            if (!Geometry(L,Level,X,Y,D,Tx,Ty,Q,Step)) return false;
            const double Sigma=SigmaInLocalTexels*Step;
            double H=0;
            for (int32 J=0; J<K.Count; ++J) for (int32 I=0; I<K.Count; ++I)
            {
                // Physical tangent offsets lifted by the spherical exponential
                // map. The kernel never changes terrain frequency/amplitude.
                const FVector3d AngleVector=(Tx*K.Nodes[I]+Ty*K.Nodes[J])*(Sigma/L.RadiusCm);
                const double Angle=AngleVector.Size();
                const double Sinc=Angle>1.e-12 ? FMath::Sin(Angle)/Angle : 1.0;
                FVector3d SampleD=D*FMath::Cos(Angle)+AngleVector*Sinc;
                if (SampleD.ContainsNaN() || !SampleD.Normalize()) return false;
                double SampleH=0;
                if (!Height(SampleD,SampleH) || !FMath::IsFinite(SampleH) || L.RadiusCm+SampleH <= 0) return false;
                H+=K.Weights[I]*K.Weights[J]*SampleH;
            }
            if (!FMath::IsFinite(H) || L.RadiusCm+H <= 0) return false;
            Out=H; return true;
        }
    }

    // Exact differential of the filtered radial surface in stereo coordinates,
    // with finite differences only for H. Constant height gives EXACT zero g,
    // unlike cross products of finite displaced neighbours on a curved sphere.
    inline bool GradientResidual(const APSCanonicalCoverageLayout::FLayout& Layout, int32 Level,
        double U, double V, double Center, double Left, double Right, double Up, double Down,
        FVector3d& Out)
    {
        if (!Private::LayoutValid(Layout,Level) || !FMath::IsFinite(Center) || !FMath::IsFinite(Left)
            || !FMath::IsFinite(Right) || !FMath::IsFinite(Up) || !FMath::IsFinite(Down)
            || Layout.RadiusCm+Center <= 0) return false;
        FVector3d D,Tx,Ty; double Q=0,Step=0;
        if (!Private::Geometry(Layout,Level,U,V,D,Tx,Ty,Q,Step)) return false;
        const double DeltaS=Layout.Levels[Level].WidthCm/(2.0*Layout.RadiusCm*Layout.Resolution);
        const double Hx=(Right-Left)/(2.0*DeltaS), Hy=(Down-Up)/(2.0*DeltaS);
        const double Metric=4.0/(Q*Q);
        const FVector3d G=(Tx*Hx+Ty*Hy)*(2.0/Q)/((Layout.RadiusCm+Center)*Metric);
        if (G.ContainsNaN() || FMath::Abs(G.X)>65000 || FMath::Abs(G.Y)>65000 || FMath::Abs(G.Z)>65000
            || FMath::Abs(FVector3d::DotProduct(G,D))>1.e-7*FMath::Max(1.0,G.Size())) return false;
        Out=G; return true;
    }

    inline bool GradientContractsPass(const APSCanonicalCoverageLayout::FLayout& Layout, int32 Level)
    {
        if (!Private::LayoutValid(Layout,Level)) return false;
        for (double U : {.01,.5,.99}) for (double V : {.01,.5,.99})
        {
            FVector3d G;
            if (!GradientResidual(Layout,Level,U,V,0,0,0,0,0,G) || G!=FVector3d::ZeroVector
                || !GradientResidual(Layout,Level,U,V,12345,12345,12345,12345,12345,G)
                || G!=FVector3d::ZeroVector) return false;
            FVector3d D,Tx,Ty; double Q=0,Step=0;
            if (!Private::Geometry(Layout,Level,U,V,D,Tx,Ty,Q,Step)) return false;
            const double DeltaS=Layout.Levels[Level].WidthCm/(2.0*Layout.RadiusCm*Layout.Resolution);
            const double Hx=Layout.RadiusCm*.002, Hy=-Layout.RadiusCm*.001;
            if (!GradientResidual(Layout,Level,U,V,0,-Hx*DeltaS,Hx*DeltaS,-Hy*DeltaS,Hy*DeltaS,G)
                || !G.Equals((Tx*Hx+Ty*Hy)*(Q/(2.0*Layout.RadiusCm)),1.e-10)) return false;
        }
        return true;
    }

    inline bool BuildLevel(const APSClosedGlobeMesh::FSamplingFrame& Frame,
        const APSCanonicalCoverageLayout::FLayout& Layout, int32 Level,
        const FOptions& Options, FData& Out, FString& Error)
    {
        Error.Reset(); const double Started=FPlatformTime::Seconds();
        const auto Cancelled=[&]() { return Options.Cancellation && Options.Cancellation->load(std::memory_order_relaxed); };
        const auto Cancel=[&]() { Error=TEXT("Filtered height preparation cancelled; no partial publication"); return false; };
        Private::FKernel K;
        const double Sea=double(Frame.Profile.OceanLevel)*Frame.NoiseIntensity;
        if (!Out.Mips.IsEmpty() || Out.ByteCount!=0 || Out.Metadata.Level!=-1)
        { Error=TEXT("Filtered height output must be empty"); return false; }
        if (Cancelled()) return Cancel();
        if (Options.MaxWorkers<0 || Options.MaxWorkers>8)
        { Error=TEXT("Filtered height worker count must be0..8"); return false; }
        const bool bDense=Options.HeightFilter==EHeightFilter::SharedLattice;
        if ((!bDense && Options.HeightFilter!=EHeightFilter::Quadrature)
            || (bDense && Options.Oversample!=2 && Options.Oversample!=4))
        { Error=TEXT("Filtered height mode needs Quadrature or SharedLattice2/4"); return false; }
        if (!APSCanonicalCoverageLayout::ValidLayout(Layout) || !Private::LayoutValid(Layout,Level)
            || (!bDense && !Private::Kernel(Options.Quadrature,K))
            || !GradientContractsPass(Layout,Level) || Options.InputSignature==0
            || Frame.Radius!=Layout.RadiusCm || Frame.PresentationScale!=1.0
            || !FMath::IsFinite(Frame.NoiseScale) || Frame.NoiseScale<1
            || !FMath::IsFinite(Frame.NoiseIntensity) || Frame.NoiseIntensity<0
            || !FMath::IsFinite(Frame.OceanHeight) || !FMath::IsFinite(Sea))
        { Error=TEXT("Filtered height needs exact full-scale frame/layout, nonzero signature, valid3/5 kernel and stereo gradient"); return false; }
        const int32 N=Layout.Resolution, Pitch=N+2;
        TArray<double> Heights; Heights.SetNumUninitialized(Pitch*Pitch);
        TArray<uint8> Valid; Valid.Init(0,Pitch);
        TArray<int64> Calls; Calls.Init(0,Pitch);
        TArray<double> RowMinStep,RowMaxStep;
        RowMinStep.Init(DBL_MAX,Pitch); RowMaxStep.Init(0,Pitch);
        const APSClosedGlobeMesh::FSamplingFrame Snapshot=Frame;
        const APSCanonicalCoverageLayout::FLayout Chart=Layout;
        const auto RunRows=[&](int32 Count, const auto& Function)
        {
            if (Options.MaxWorkers==0) ParallelFor(Count,Function);
            else
            {
                const int32 Lanes=FMath::Min(Options.MaxWorkers,Count);
                ParallelFor(Lanes,[&](int32 Lane)
                {
                    for (int32 Y=Lane; Y<Count && !Cancelled(); Y+=Lanes) Function(Y);
                },EParallelForFlags::BackgroundPriority);
            }
        };
        const auto SampleHeight=[&](CustomNoise& Noise, const FVector3d& D, double& H)
        {
            H=UAPSWorldScapePlanetNoise::SampleHeightResolvedProfile(Snapshot.Profile,Noise,
                DVector(D*Snapshot.Radius),DVector(0,0,0),Snapshot.NoiseScale,Snapshot.NoiseIntensity,
                Snapshot.Radius,D.Z,Snapshot.bCoastalReliefCandidate);
            H=APSWorldScapeSurfaceEnvelope::Height(H,Sea,Snapshot.bApplyNativeLavaEnvelope);
            return FMath::IsFinite(H) && Snapshot.Radius+H>0;
        };
        int64 ExpectedCalls=0, DenseCalls=0;
        const auto BuildRow=[&](int32 Y)
        {
            if (Cancelled()) { Valid[Y]=0; return; }
            CustomNoise Noise=Snapshot.SeededNoise;
            const auto Height=[&](const FVector3d& D,double& H)
            {
                ++Calls[Y];
                return SampleHeight(Noise,D,H);
            };
            for (int32 X=0; X<Pitch; ++X)
            {
                if ((X&31)==0 && Cancelled()) return;
                double H=0,Step=0;
                if (!Private::FilterHeight(Chart,Level,(X-.5)/N,(Y-.5)/N,K,Height,H,Step))
                { return; }
                Heights[Y*Pitch+X]=H;
                RowMinStep[Y]=FMath::Min(RowMinStep[Y],Step); RowMaxStep[Y]=FMath::Max(RowMaxStep[Y],Step);
            }
            Valid[Y]=1; // A skipped/interrupted lane never validates uninitialised heights.
        };
        if (!bDense)
        {
            ExpectedCalls=int64(Pitch)*Pitch*K.Count*K.Count;
            RunRows(Pitch,BuildRow);
        }
        else
        {
            // Shared oversampled HEIGHT lattice; filter before decimating and
            // differentiating. Same authoritative noise/envelope, no slope gain.
            // Gaussian is separable in stereo chart coordinates, not an exact
            // spherical convolution. Finite sampling is NOT alias-free above
            // the fine lattice Nyquist; never advertise strict band limitation.
            const int32 S=Options.Oversample, Radius=3*S;
            const int32 FinePitch=(Pitch-1)*S+2*Radius+1;
            const double Sigma=SigmaInLocalTexels*S;
            TArray<double> Weights; Weights.SetNumUninitialized(2*Radius+1);
            double WeightSum=0;
            for (int32 Kx=-Radius; Kx<=Radius; ++Kx)
            {
                const double W=FMath::Exp(-.5*FMath::Square(Kx/Sigma));
                Weights[Kx+Radius]=W; WeightSum+=W;
            }
            for (double& W:Weights) W/=WeightSum;
            // Only horizontally decimated rows are retained (~32.3MiB at4x),
            // not FinePitch squared heights. Scratch is per executing row lane.
            TArray<double> Horizontal; Horizontal.SetNumUninitialized(FinePitch*Pitch);
            TArray<uint8> FineValid; FineValid.Init(0,FinePitch);
            TArray<int64> FineCalls; FineCalls.Init(0,FinePitch);
            ExpectedCalls=int64(FinePitch)*FinePitch;
            RunRows(FinePitch,[&](int32 Y)
            {
                if (Cancelled()) return;
                CustomNoise Noise=Snapshot.SeededNoise;
                TArray<double> FineRow; FineRow.SetNumUninitialized(FinePitch);
                const double V=(double(Y-Radius)/S-.5)/N;
                for (int32 X=0; X<FinePitch; ++X)
                {
                    if ((X&31)==0 && Cancelled()) return;
                    APSCanonicalCoverageLayout::FVector3 Raw;
                    if (!APSCanonicalCoverageLayout::Direction(Chart,Level,(double(X-Radius)/S-.5)/N,V,Raw)) return;
                    ++FineCalls[Y];
                    if (!SampleHeight(Noise,Private::Vector(Raw),FineRow[X])) return;
                }
                for (int32 X=0; X<Pitch; ++X)
                {
                    if ((X&31)==0 && Cancelled()) return;
                    double H=0;
                    for (int32 Kx=-Radius; Kx<=Radius; ++Kx)
                        H+=Weights[Kx+Radius]*FineRow[Radius+X*S+Kx];
                    Horizontal[Y*Pitch+X]=H;
                }
                FineValid[Y]=1;
            });
            if (Cancelled()) return Cancel();
            for (int32 Y=0; Y<FinePitch; ++Y)
            {
                if (!FineValid[Y]) { Error=TEXT("Shared height lattice contains invalid/unwritten row"); return false; }
                DenseCalls+=FineCalls[Y];
            }
            RunRows(Pitch,[&](int32 Y)
            {
                for (int32 X=0; X<Pitch; ++X)
                {
                    if ((X&31)==0 && Cancelled()) return;
                    double H=0,Step=0;
                    for (int32 Ky=-Radius; Ky<=Radius; ++Ky)
                        H+=Weights[Ky+Radius]*Horizontal[(Radius+Y*S+Ky)*Pitch+X];
                    if (!FMath::IsFinite(H) || Chart.RadiusCm+H<=0
                        || !APSCanonicalCoverageLayout::LocalTexelSpacing(Chart,Level,(X-.5)/N,(Y-.5)/N,Step)) return;
                    Heights[Y*Pitch+X]=H;
                    RowMinStep[Y]=FMath::Min(RowMinStep[Y],Step); RowMaxStep[Y]=FMath::Max(RowMaxStep[Y],Step);
                }
                Valid[Y]=1;
            });
        }
        if (Cancelled()) return Cancel();
        FData Result; auto& M=Result.Metadata;
        M.Layout=Layout; M.Level=Level; M.QuadratureOrder=K.Count; M.InputSignature=Options.InputSignature;
        M.HeightFilter=Options.HeightFilter; M.Oversample=bDense ? Options.Oversample : 0;
        M.PlanetType=Frame.Profile.PlanetType; M.TerrainSeed=Frame.Profile.TerrainSeed; M.BiomeSeed=Frame.Profile.BiomeSeed;
        M.NoiseScale=Frame.NoiseScale; M.NoiseIntensity=Frame.NoiseIntensity; M.EnvelopeSeaHeightCm=Sea;
        M.WidthCm=Layout.Levels[Level].WidthCm; M.CenterTexelSpacingCm=Layout.Levels[Level].CenterTexelSpacingCm;
        M.MaximumHeightSamples=ExpectedCalls; M.ActualHeightSamples=DenseCalls;
        M.MinimumLocalSpacingCm=DBL_MAX; M.MinimumFilteredHeightCm=DBL_MAX; M.MaximumFilteredHeightCm=-DBL_MAX;
        M.bCoastalReliefCandidate=Frame.bCoastalReliefCandidate; M.bNativeLavaEnvelope=Frame.bApplyNativeLavaEnvelope;
        for (int32 Y=0; Y<Pitch; ++Y)
        {
            if (!Valid[Y]) { Error=FString::Printf(TEXT("Filtered height invalid kernel sample in row%d; output not published"),Y); return false; }
            M.ActualHeightSamples+=Calls[Y];
            M.MinimumLocalSpacingCm=FMath::Min(M.MinimumLocalSpacingCm,RowMinStep[Y]);
            M.MaximumLocalSpacingCm=FMath::Max(M.MaximumLocalSpacingCm,RowMaxStep[Y]);
        }
        if (M.ActualHeightSamples!=M.MaximumHeightSamples)
        { Error=TEXT("Filtered height-call count differs from fixed sampling lattice budget"); return false; }
        // XYZ stores slope*solid-angle weight; W retains that weight. Never
        // average repeatedly normalised normals or slopes across mip levels.
        TArray<FVector4d> Sums; Sums.SetNumUninitialized(N*N);
        for (int32 Y=0; Y<N; ++Y) for (int32 X=0; X<N; ++X)
        {
            if (X==0 && Cancelled()) return Cancel();
            const int32 I=(Y+1)*Pitch+X+1; const double U=(X+.5)/N,V=(Y+.5)/N;
            FVector3d G,D,Tx,Ty; double Q=0,Step=0;
            if (!GradientResidual(Layout,Level,U,V,Heights[I],Heights[I-1],Heights[I+1],Heights[I-Pitch],Heights[I+Pitch],G)
                || !Private::Geometry(Layout,Level,U,V,D,Tx,Ty,Q,Step))
            { Error=TEXT("Filtered height gradient is nonfinite/degenerate/unrepresentable"); return false; }
            const double Weight=1.0/(Q*Q); // Common stereo4*deltaS^2 cancels in the mean.
            Sums[Y*N+X]=FVector4d(G.X*Weight,G.Y*Weight,G.Z*Weight,Weight);
            M.MinimumFilteredHeightCm=FMath::Min(M.MinimumFilteredHeightCm,Heights[I]);
            M.MaximumFilteredHeightCm=FMath::Max(M.MaximumFilteredHeightCm,Heights[I]);
            M.MaximumSlope=FMath::Max(M.MaximumSlope,G.Size());
        }
        Heights.Reset();
        for (int32 Size=N;; Size/=2)
        {
            if (Cancelled()) return Cancel();
            FMip Mip; Mip.Size=Size; Mip.Pixels.Reserve(Size*Size);
            int32 Packed=0;
            for (const FVector4d& Sum:Sums)
            {
                if ((Packed++&1023)==0 && Cancelled()) return Cancel();
                if (Sum.ContainsNaN() || Sum.W<=0)
                { Error=TEXT("Filtered residual mip weight is invalid"); return false; }
                const FVector3d G(Sum.X/Sum.W,Sum.Y/Sum.W,Sum.Z/Sum.W);
                if (G.ContainsNaN() || FMath::Abs(G.X)>65000 || FMath::Abs(G.Y)>65000 || FMath::Abs(G.Z)>65000)
                { Error=TEXT("Filtered residual mip cannot be represented as signed half floats"); return false; }
                Mip.Pixels.Emplace(FLinearColor(float(G.X),float(G.Y),float(G.Z),1));
            }
            Result.ByteCount+=int64(Mip.Pixels.Num())*sizeof(FFloat16Color); Result.Mips.Add(MoveTemp(Mip));
            if (Size==1) break;
            const int32 Next=Size/2; TArray<FVector4d> NextSums; NextSums.SetNumZeroed(Next*Next);
            for (int32 Y=0; Y<Next; ++Y) for (int32 X=0; X<Next; ++X)
                for (int32 DY=0; DY<2; ++DY) for (int32 DX=0; DX<2; ++DX)
                    NextSums[Y*Next+X]+=Sums[(2*Y+DY)*Size+2*X+DX];
            Sums=MoveTemp(NextSums);
        }
        if (Cancelled()) return Cancel();
        M.PreparationWallSeconds=FPlatformTime::Seconds()-Started; Out=MoveTemp(Result); return true;
    }

    // Convenience only: levels are joined sequentially, never launch9 nested
    // builds at once. Runtime owners should budget/cache individual BuildLevel.
    inline bool BuildCascade(const APSClosedGlobeMesh::FSamplingFrame& Frame,
        const APSCanonicalCoverageLayout::FLayout& Layout, const FOptions& Options,
        TArray<FData>& Out, FString& Error)
    {
        Error.Reset();
        if (!Out.IsEmpty() || !APSCanonicalCoverageLayout::ValidLayout(Layout)
            || Layout.Count<1 || Layout.Count>MaximumLevels)
        { Error=TEXT("Filtered cascade needs empty output and1..9 levels"); return false; }
        TArray<FData> Result; Result.Reserve(Layout.Count);
        for (int32 Level=0; Level<Layout.Count; ++Level)
        {
            FData Data;
            if (!BuildLevel(Frame,Layout,Level,Options,Data,Error)) return false;
            Result.Add(MoveTemp(Data));
        }
        Out=MoveTemp(Result); return true;
    }

    // Independent bounded diagnostic: same lattice+frame, possibly differing
    // height filters/oversampling densities (deliberately not identity gates).
    // normal-mip agreement alone does not establish height-filter convergence.
    inline bool CompareSameLattice(const FData& A, const FData& B, FComparison& Out, FString& Error)
    {
        Error.Reset(); const auto& L=A.Metadata.Layout; const int32 Level=A.Metadata.Level;
        if (Out.Samples!=0 || !APSCanonicalCoverageLayout::ValidLayout(L)
            || !APSCanonicalCoverageLayout::ValidLayout(B.Metadata.Layout)
            || !Private::LayoutValid(L,Level) || A.Mips.IsEmpty() || B.Mips.IsEmpty()
            || A.Metadata.InputSignature==0 || A.Metadata.InputSignature!=B.Metadata.InputSignature
            || A.Metadata.NoiseScale!=B.Metadata.NoiseScale || A.Metadata.NoiseIntensity!=B.Metadata.NoiseIntensity
            || A.Metadata.EnvelopeSeaHeightCm!=B.Metadata.EnvelopeSeaHeightCm
            || A.Metadata.PlanetType!=B.Metadata.PlanetType || A.Metadata.TerrainSeed!=B.Metadata.TerrainSeed
            || A.Metadata.BiomeSeed!=B.Metadata.BiomeSeed
            || A.Metadata.bCoastalReliefCandidate!=B.Metadata.bCoastalReliefCandidate
            || A.Metadata.bNativeLavaEnvelope!=B.Metadata.bNativeLavaEnvelope
            || A.Metadata.WidthCm!=B.Metadata.WidthCm || L.RadiusCm!=B.Metadata.Layout.RadiusCm
            || L.Resolution!=B.Metadata.Layout.Resolution || Level!=B.Metadata.Level
            || !Private::Vector(L.Frame.Center).Equals(Private::Vector(B.Metadata.Layout.Frame.Center),0)
            || !Private::Vector(L.Frame.AxisU).Equals(Private::Vector(B.Metadata.Layout.Frame.AxisU),0)
            || !Private::Vector(L.Frame.AxisV).Equals(Private::Vector(B.Metadata.Layout.Frame.AxisV),0)
            || A.Mips[0].Size!=L.Resolution || B.Mips[0].Size!=L.Resolution
            || A.Mips[0].Pixels.Num()!=L.Resolution*L.Resolution || B.Mips[0].Pixels.Num()!=A.Mips[0].Pixels.Num())
        { Error=TEXT("Convergence comparison requires empty output and the same immutable frame/lattice"); return false; }
        FComparison Result; TArray<double> Angles; Angles.Reserve(A.Mips[0].Pixels.Num());
        double SquareSum=0;
        for (int32 Y=0; Y<L.Resolution; ++Y) for (int32 X=0; X<L.Resolution; ++X)
        {
            APSCanonicalCoverageLayout::FVector3 Raw;
            if (!APSCanonicalCoverageLayout::Direction(L,Level,(X+.5)/L.Resolution,(Y+.5)/L.Resolution,Raw))
            { Error=TEXT("Convergence comparison has invalid direction"); return false; }
            const FVector3d D=Private::Vector(Raw);
            const auto Normal=[&](const FData& Data)
            {
                const FLinearColor C=Data.Mips[0].Pixels[Y*L.Resolution+X].GetFloats();
                FVector3d G(C.R,C.G,C.B); G-=D*FVector3d::DotProduct(D,G); return (D-G).GetSafeNormal();
            };
            const FVector3d NA=Normal(A),NB=Normal(B);
            if (NA.ContainsNaN() || NB.ContainsNaN() || NA.IsNearlyZero() || NB.IsNearlyZero())
            { Error=TEXT("Convergence comparison has invalid residual"); return false; }
            const double Angle=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector3d::DotProduct(NA,NB),-1.0,1.0)));
            Angles.Add(Angle); Result.MeanDegrees+=Angle; SquareSum+=Angle*Angle;
            Result.MaximumDegrees=FMath::Max(Result.MaximumDegrees,Angle);
        }
        Angles.Sort(); Result.Samples=Angles.Num(); Result.MeanDegrees/=Result.Samples;
        Result.RmsDegrees=FMath::Sqrt(SquareSum/Result.Samples);
        Result.P95Degrees=Angles[FMath::Clamp(FMath::CeilToInt(Angles.Num()*.95)-1,0,Angles.Num()-1)];
        Out=Result; return true;
    }
}
