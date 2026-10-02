#pragma once
#include <cstdint>
#include <cmath>

// Shared verbatim by the staged native-worker integration and CPU regression.
// Planet-local sector identity, never camera/world-origin/pointer/frame time.
namespace APSBoundedFoliageSeed
{
inline std::uint32_t Mix(std::uint32_t X)
{
    X ^= X >> 16; X *= 0x7feb352du; X ^= X >> 15;
    X *= 0x846ca68bu; return X ^ (X >> 16);
}
inline std::uint32_t Fold(std::uint32_t H, std::int64_t V)
{
    const auto U = static_cast<std::uint64_t>(V);
    return Mix(H ^ Mix(static_cast<std::uint32_t>(U)))
        ^ Mix(static_cast<std::uint32_t>(U >> 32) + 0x9e3779b9u);
}
inline std::uint32_t Make(std::int64_t X, std::int64_t Y, std::int64_t Z,
    std::int32_t PlanetSeed, std::int32_t Collection, std::int32_t Species, std::int32_t Layer)
{
    auto H = Mix(static_cast<std::uint32_t>(PlanetSeed) ^ 0x41505332u);
    H = Fold(H, X); H = Fold(H ^ 0x9e3779b9u, Y); H = Fold(H ^ 0x85ebca6bu, Z);
    H = Fold(H, Collection); H = Fold(H, Species); H = Fold(H, Layer);
    return Mix(H) & 0x7fffffffu;
}
inline std::uint32_t ForSector(double X, double Y, double Z, double Size,
    std::int32_t PlanetSeed, std::int32_t Collection, std::int32_t Species, std::int32_t Layer)
{
    if (!std::isfinite(Size) || Size <= 0 || !std::isfinite(X) || !std::isfinite(Y) || !std::isfinite(Z)) return 0;
    const double GX = X / Size, GY = Y / Size, GZ = Z / Size;
    // Explicit bound before llround: invalid presentation cannot cause UB.
    constexpr double Limit = 4.e18;
    if (std::abs(GX) > Limit || std::abs(GY) > Limit || std::abs(GZ) > Limit) return 0;
    return Make(std::llround(GX), std::llround(GY), std::llround(GZ), PlanetSeed, Collection, Species, Layer);
}
}
