#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Experimental topology only: no runtime enablement, positions, sampling or UE types.
namespace APSCoastalMeshRefinement
{
using FIndex = std::uint32_t;
enum class EStatus { Applied, NoOp, BudgetExceeded, InvalidInput };

struct FMidpoint
{
    FIndex A;
    FIndex B;
};

struct FPlan
{
    EStatus Status = EStatus::InvalidInput;
    // Midpoints[i] creates vertex InputWeldIds.size() + i; endpoints are input
    // indices, always A < B. Distinct raw seam edges retain distinct vertices.
    std::vector<FMidpoint> Midpoints;
    std::vector<FIndex> Indices;
    std::vector<FIndex> WeldIds;
};

namespace Detail
{
inline std::uint64_t EdgeKey(FIndex A, FIndex B)
{
    if (B < A) std::swap(A, B);
    return (std::uint64_t(A) << 32) | B;
}

inline FPlan Rejected(EStatus Status)
{
    FPlan Result;
    Result.Status = Status;
    return Result;
}
}

// One conforming red/green pass; callers may explicitly request a second pass
// after sampling every new vertex. Any nonzero selection byte selects a triangle.
// All selected geometric edges propagate through weld IDs, including cube seams.
// Deterministic first triangle/edge encounter order; hash iteration is never used.
// NoOp copies the original topology; rejection returns empty outputs and leaves
// inputs untouched. Budgets count RAW vertices, including duplicated seam vertices.
// Counts are bounded by FIndex::max. Weld IDs may be sparse, but new IDs must fit.
// Geometric zero area cannot be checked without positions; callers must validate
// projected/displaced geometry after sampling. Repeated index/weld IDs are rejected.
// Input must already be conforming; equal weld IDs must sample the same position.
inline FPlan PlanPass(const std::vector<FIndex>& InputIndices,
    const std::vector<FIndex>& InputWeldIds,
    const std::vector<std::uint8_t>& CoastTriangles,
    std::size_t MaxAddedVertices)
{
    const std::size_t Limit = std::numeric_limits<FIndex>::max();
    const std::size_t VertexCount = InputWeldIds.size();
    if (VertexCount > Limit || InputIndices.size() > Limit
        || InputIndices.size() % 3 != 0
        || CoastTriangles.size() != InputIndices.size() / 3)
        return Detail::Rejected(EStatus::InvalidInput);

    FIndex MaximumWeldId = 0;
    for (FIndex WeldId : InputWeldIds)
        MaximumWeldId = std::max(MaximumWeldId, WeldId);

    std::unordered_set<std::uint64_t> MarkedEdges;
    for (std::size_t T = 0; T < CoastTriangles.size(); ++T)
    {
        const FIndex A = InputIndices[3 * T];
        const FIndex B = InputIndices[3 * T + 1];
        const FIndex C = InputIndices[3 * T + 2];
        if (A >= VertexCount || B >= VertexCount || C >= VertexCount
            || A == B || B == C || C == A)
            return Detail::Rejected(EStatus::InvalidInput);
        const FIndex WA = InputWeldIds[A], WB = InputWeldIds[B], WC = InputWeldIds[C];
        if (WA == WB || WB == WC || WC == WA)
            return Detail::Rejected(EStatus::InvalidInput);
        if (CoastTriangles[T])
        {
            MarkedEdges.insert(Detail::EdgeKey(WA, WB));
            MarkedEdges.insert(Detail::EdgeKey(WB, WC));
            MarkedEdges.insert(Detail::EdgeKey(WC, WA));
        }
    }

    if (MarkedEdges.empty())
    {
        FPlan Result;
        Result.Status = EStatus::NoOp;
        Result.Indices = InputIndices;
        Result.WeldIds = InputWeldIds;
        return Result;
    }

    // Preflight the WHOLE pass before publishing any topology. In particular,
    // exhausting the budget may not leave only one side of a seam subdivided.
    std::unordered_map<std::uint64_t, FIndex> RawMidpoints;
    std::unordered_map<std::uint64_t, FIndex> GeometricMidpointWelds;
    std::vector<FIndex> AddedWeldIds;
    FPlan Result;
    std::size_t OutputIndexCount = 0;
    for (std::size_t T = 0; T < CoastTriangles.size(); ++T)
    {
        const FIndex V[3] = {InputIndices[3 * T], InputIndices[3 * T + 1], InputIndices[3 * T + 2]};
        std::size_t ChildCount = 1;
        for (unsigned E = 0; E < 3; ++E)
        {
            const FIndex A = V[E], B = V[(E + 1) % 3];
            const std::uint64_t GeometricEdge = Detail::EdgeKey(InputWeldIds[A], InputWeldIds[B]);
            if (!MarkedEdges.count(GeometricEdge)) continue;
            ++ChildCount;
            const std::uint64_t RawEdge = Detail::EdgeKey(A, B);
            if (RawMidpoints.count(RawEdge)) continue;
            if (Result.Midpoints.size() >= MaxAddedVertices)
                return Detail::Rejected(EStatus::BudgetExceeded);
            if (Result.Midpoints.size() >= Limit - VertexCount)
                return Detail::Rejected(EStatus::InvalidInput);

            auto Weld = GeometricMidpointWelds.find(GeometricEdge);
            if (Weld == GeometricMidpointWelds.end())
            {
                if (GeometricMidpointWelds.size() >= Limit - MaximumWeldId)
                    return Detail::Rejected(EStatus::InvalidInput);
                const FIndex Id = static_cast<FIndex>(
                    std::size_t(MaximumWeldId) + GeometricMidpointWelds.size() + 1);
                Weld = GeometricMidpointWelds.emplace(GeometricEdge, Id).first;
            }
            RawMidpoints.emplace(RawEdge, static_cast<FIndex>(VertexCount + Result.Midpoints.size()));
            Result.Midpoints.push_back({std::min(A, B), std::max(A, B)});
            AddedWeldIds.push_back(Weld->second);
        }
        if (OutputIndexCount > Limit - 3 * ChildCount)
            return Detail::Rejected(EStatus::InvalidInput);
        OutputIndexCount += 3 * ChildCount;
    }

    Result.Indices.reserve(OutputIndexCount);
    Result.WeldIds = InputWeldIds;
    Result.WeldIds.insert(Result.WeldIds.end(), AddedWeldIds.begin(), AddedWeldIds.end());
    const auto Add = [&Result](FIndex A, FIndex B, FIndex C)
    { Result.Indices.push_back(A); Result.Indices.push_back(B); Result.Indices.push_back(C); };
    for (std::size_t T = 0; T < CoastTriangles.size(); ++T)
    {
        const FIndex V[3] = {InputIndices[3 * T], InputIndices[3 * T + 1], InputIndices[3 * T + 2]};
        FIndex M[3] = {};
        unsigned Mask = 0;
        for (unsigned E = 0; E < 3; ++E)
        {
            const auto Found = RawMidpoints.find(Detail::EdgeKey(V[E], V[(E + 1) % 3]));
            if (Found != RawMidpoints.end()) { Mask |= 1u << E; M[E] = Found->second; }
        }
        const FIndex A = V[0], B = V[1], C = V[2], X = M[0], Y = M[1], Z = M[2];
        switch (Mask)
        {
        case 0: Add(A, B, C); break;
        case 1: Add(A, X, C); Add(X, B, C); break;
        case 2: Add(A, B, Y); Add(A, Y, C); break;
        case 4: Add(A, B, Z); Add(Z, B, C); break;
        case 3: Add(X, B, Y); Add(A, X, C); Add(X, Y, C); break;
        case 6: Add(Y, C, Z); Add(A, B, Z); Add(B, Y, Z); break;
        case 5: Add(A, X, Z); Add(X, B, C); Add(X, C, Z); break;
        case 7: Add(A, X, Z); Add(X, B, Y); Add(Z, Y, C); Add(X, Y, Z); break;
        }
    }
    Result.Status = EStatus::Applied;
    return Result;
}
}
