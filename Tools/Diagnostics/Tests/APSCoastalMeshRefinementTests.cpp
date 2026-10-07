// Standalone CPU-only test; compile explicitly with a C++11-or-newer compiler.
// This is not a UE automation test and does not enable the preview candidate.
#include "../../../Source/APS_ALPHA/Core/Planetary/APSCoastalMeshRefinement.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace APSCoastalMeshRefinement;
struct FPoint { double X; double Y; };

void Check(bool Condition, const char* Message)
{
    if (!Condition) throw std::runtime_error(Message);
}

double TwiceArea(const FPoint& A, const FPoint& B, const FPoint& C)
{
    return (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X);
}

std::vector<FPoint> AddMidpoints(const std::vector<FPoint>& Points, const FPlan& Plan)
{
    std::vector<FPoint> Result = Points;
    for (const FMidpoint& M : Plan.Midpoints)
    {
        Check(M.A < M.B && M.B < Points.size(), "Midpoints require ordered input endpoints");
        Result.push_back({(Points[M.A].X + Points[M.B].X) * 0.5,
            (Points[M.A].Y + Points[M.B].Y) * 0.5});
    }
    Check(Result.size() == Plan.WeldIds.size(), "Output weld count differs from vertex count");
    return Result;
}

std::size_t MidpointOffset(const FPlan& Plan, FIndex A, FIndex B)
{
    if (B < A) std::swap(A, B);
    for (std::size_t I = 0; I < Plan.Midpoints.size(); ++I)
        if (Plan.Midpoints[I].A == A && Plan.Midpoints[I].B == B) return I;
    return Plan.Midpoints.size();
}

unsigned EdgeOccurrences(const std::vector<FIndex>& Indices, FIndex A, FIndex B)
{
    unsigned Count = 0;
    for (std::size_t T = 0; T < Indices.size(); T += 3)
        for (unsigned E = 0; E < 3; ++E)
        {
            const FIndex X = Indices[T + E], Y = Indices[T + (E + 1) % 3];
            if ((X == A && Y == B) || (X == B && Y == A)) ++Count;
        }
    return Count;
}

void CheckGeometry(const std::vector<FPoint>& Points,
    const std::vector<FIndex>& Input, const FPlan& Plan)
{
    const auto OutputPoints = AddMidpoints(Points, Plan);
    std::size_t OutputOffset = 0;
    for (std::size_t T = 0; T < Input.size(); T += 3)
    {
        unsigned Children = 1;
        for (unsigned E = 0; E < 3; ++E)
            Children += MidpointOffset(Plan, Input[T + E], Input[T + (E + 1) % 3]) < Plan.Midpoints.size();
        const double ParentArea = TwiceArea(Points[Input[T]], Points[Input[T + 1]], Points[Input[T + 2]]);
        double ChildrenArea = 0;
        for (unsigned Child = 0; Child < Children; ++Child)
        {
            Check(OutputOffset + 2 < Plan.Indices.size(), "Missing output triangle");
            const FIndex A = Plan.Indices[OutputOffset], B = Plan.Indices[OutputOffset + 1], C = Plan.Indices[OutputOffset + 2];
            Check(A < OutputPoints.size() && B < OutputPoints.size() && C < OutputPoints.size(), "Invalid output index");
            const double Area = TwiceArea(OutputPoints[A], OutputPoints[B], OutputPoints[C]);
            Check(Area * ParentArea > 0, "Child winding reversed or triangle collapsed");
            ChildrenArea += Area;
            OutputOffset += 3;
        }
        Check(std::abs(ChildrenArea - ParentArea) < 1.e-10, "Child areas do not cover the parent");
    }
    Check(OutputOffset == Plan.Indices.size(), "Unexpected output triangles");
    for (const FMidpoint& M : Plan.Midpoints)
        Check(EdgeOccurrences(Plan.Indices, M.A, M.B) == 0, "Unsplit edge survived beside a midpoint");
}

void AllEightMasks()
{
    const std::vector<FPoint> Points = {{0, 0}, {2, 0}, {0, 2}, {0, -2}, {2, 2}, {-2, 0}};
    const std::vector<FIndex> Welds = {10, 11, 12, 13, 14, 15};
    // Base triangle is NEVER selected; its edge masks must arrive from neighbours.
    const std::vector<FIndex> Fixture = {0, 1, 2, 1, 0, 3, 2, 1, 4, 0, 2, 5};
    for (unsigned Reverse = 0; Reverse < 2; ++Reverse)
        for (unsigned Mask = 0; Mask < 8; ++Mask)
        {
            auto Input = Fixture;
            if (Reverse) for (std::size_t T = 0; T < Input.size(); T += 3) std::swap(Input[T + 1], Input[T + 2]);
            const std::vector<std::uint8_t> Selection = {0,
                std::uint8_t(Mask & 1), std::uint8_t(Mask & 2), std::uint8_t(Mask & 4)};
            const FPlan Plan = PlanPass(Input, Welds, Selection, 100);
            Check(Plan.Status == (Mask ? EStatus::Applied : EStatus::NoOp), "Unexpected mask status");
            Check((MidpointOffset(Plan, 0, 1) < Plan.Midpoints.size()) == bool(Mask & 1), "AB mask mismatch");
            Check((MidpointOffset(Plan, 1, 2) < Plan.Midpoints.size()) == bool(Mask & 2), "BC mask mismatch");
            Check((MidpointOffset(Plan, 2, 0) < Plan.Midpoints.size()) == bool(Mask & 4), "CA mask mismatch");
            CheckGeometry(Points, Input, Plan);
        }
}

void SharedAndSeamEdges()
{
    const std::vector<FIndex> SharedInput = {0, 1, 2, 1, 0, 3};
    const auto Shared = PlanPass(SharedInput, {10, 11, 12, 13}, {1, 0}, 3);
    Check(Shared.Status == EStatus::Applied && Shared.Midpoints.size() == 3, "Shared raw edge was duplicated");
    const FIndex M = FIndex(4 + MidpointOffset(Shared, 0, 1));
    Check(EdgeOccurrences(Shared.Indices, 0, M) == 2 && EdgeOccurrences(Shared.Indices, M, 1) == 2,
        "Shared edge halves are not conforming");
    CheckGeometry({{0, 0}, {2, 0}, {0, 2}, {0, -2}}, SharedInput, Shared);

    const std::vector<FIndex> SeamInput = {0, 1, 2, 4, 3, 5};
    const std::vector<FIndex> SeamWelds = {10, 11, 12, 10, 11, 13};
    const std::vector<FPoint> SeamPoints = {{0, 0}, {2, 0}, {0, 2}, {0, 0}, {2, 0}, {0, -2}};
    const auto Seam = PlanPass(SeamInput, SeamWelds, {1, 0}, 4);
    Check(Seam.Status == EStatus::Applied && Seam.Midpoints.size() == 4, "Seam render vertices must remain duplicated");
    const auto Left = MidpointOffset(Seam, 0, 1), Right = MidpointOffset(Seam, 3, 4);
    Check(Left < Seam.Midpoints.size() && Right < Seam.Midpoints.size() && Left != Right, "Seam midpoint missing or merged");
    Check(Seam.WeldIds[6 + Left] == Seam.WeldIds[6 + Right], "Seam midpoint weld IDs differ");
    Check(Seam.Indices.size() == 18, "Unselected seam neighbour did not split");
    CheckGeometry(SeamPoints, SeamInput, Seam);
    const auto Rejected = PlanPass(SeamInput, SeamWelds, {1, 0}, 3);
    Check(Rejected.Status == EStatus::BudgetExceeded && Rejected.Indices.empty()
        && Rejected.WeldIds.empty() && Rejected.Midpoints.empty(), "Budget ignored seam duplicates or partially applied");

    // A second explicit pass consumes the first pass's topology and sampled points.
    const auto Second = PlanPass(Seam.Indices, Seam.WeldIds,
        std::vector<std::uint8_t>(Seam.Indices.size() / 3, 1), 100);
    Check(Second.Status == EStatus::Applied, "Second caller-controlled pass failed");
    CheckGeometry(AddMidpoints(SeamPoints, Seam), Seam.Indices, Second);
    std::vector<std::uint8_t> PartialSelection(Seam.Indices.size() / 3, 0);
    PartialSelection[0] = 1;
    const auto PartialSecond = PlanPass(Seam.Indices, Seam.WeldIds, PartialSelection, 100);
    Check(PartialSecond.Status == EStatus::Applied, "Partially selected second pass failed");
    CheckGeometry(AddMidpoints(SeamPoints, Seam), Seam.Indices, PartialSecond);
    std::unordered_map<std::uint64_t, FIndex> WeldByEdge;
    for (std::size_t I = 0; I < Second.Midpoints.size(); ++I)
    {
        const auto& Edge = Second.Midpoints[I];
        const auto Key = Detail::EdgeKey(Seam.WeldIds[Edge.A], Seam.WeldIds[Edge.B]);
        const FIndex WeldId = Second.WeldIds[Seam.WeldIds.size() + I];
        const auto Found = WeldByEdge.emplace(Key, WeldId);
        Check(Found.second || Found.first->second == WeldId, "Second-pass seam weld identity was lost");
    }
}

void DeterminismAndValidation()
{
    const std::vector<FIndex> Input = {0, 1, 2}, Welds = {30, 10, 20};
    const auto First = PlanPass(Input, Welds, {1}, 3);
    const auto Again = PlanPass(Input, Welds, {1}, 3);
    Check(First.Status == EStatus::Applied && First.Indices == Again.Indices && First.WeldIds == Again.WeldIds,
        "Repeated plans differ");
    Check(First.Indices == std::vector<FIndex>({0, 3, 5, 3, 1, 4, 5, 4, 2, 3, 4, 5}), "Unexpected deterministic output order");
    Check(First.WeldIds == std::vector<FIndex>({30, 10, 20, 31, 32, 33}), "Unexpected deterministic weld allocation");
    Check(First.Midpoints.size() == 3 && Again.Midpoints.size() == 3, "Unexpected midpoint count");
    for (std::size_t I = 0; I < First.Midpoints.size(); ++I)
        Check(First.Midpoints[I].A == Again.Midpoints[I].A && First.Midpoints[I].B == Again.Midpoints[I].B,
            "Repeated midpoint ordering differs");
    const auto NoOp = PlanPass(Input, Welds, {0}, 0);
    Check(NoOp.Status == EStatus::NoOp && NoOp.Indices == Input && NoOp.WeldIds == Welds && NoOp.Midpoints.empty(), "NoOp modified topology");
    Check(PlanPass({}, {}, {}, 0).Status == EStatus::NoOp, "Empty mesh should be NoOp");
    for (std::size_t Budget = 0; Budget < 3; ++Budget)
    {
        const auto Plan = PlanPass(Input, Welds, {1}, Budget);
        Check(Plan.Status == EStatus::BudgetExceeded && Plan.Indices.empty()
            && Plan.WeldIds.empty() && Plan.Midpoints.empty(), "Budget rejection was not atomic");
    }
    const FIndex Maximum = std::numeric_limits<FIndex>::max();
    const std::vector<FPlan> Bad = {
        PlanPass({0, 1}, Welds, {}, 100),
        PlanPass(Input, Welds, {}, 100),
        PlanPass({0, 1, 3}, Welds, {1}, 100),
        PlanPass({0, 0, 2}, Welds, {1}, 100),
        PlanPass(Input, {10, 10, 20}, {1}, 100),
        PlanPass(Input, {10, 20, Maximum}, {1}, 100),
        PlanPass({0, 1, 3}, Welds, {0}, 0),
        PlanPass({}, {}, {1}, 100)
    };
    for (const auto& Plan : Bad)
        Check(Plan.Status == EStatus::InvalidInput && Plan.Midpoints.empty()
            && Plan.Indices.empty() && Plan.WeldIds.empty(), "Invalid input accepted or partially applied");
    Check(PlanPass(Input, {10, 20, Maximum}, {0}, 0).Status == EStatus::NoOp,
        "NoOp must not allocate overflowing weld IDs");
    const auto LastIds = PlanPass(Input, {10, 20, Maximum - 3}, {1}, 3);
    Check(LastIds.Status == EStatus::Applied && LastIds.WeldIds.back() == Maximum,
        "Last representable weld ID should be usable");
    const auto LateOverflow = PlanPass(Input, {10, 20, Maximum - 2}, {1}, 3);
    Check(LateOverflow.Status == EStatus::InvalidInput && LateOverflow.Midpoints.empty()
        && LateOverflow.Indices.empty() && LateOverflow.WeldIds.empty(),
        "Weld overflow after two allocations must reject the whole pass");
    Check(Input == std::vector<FIndex>({0, 1, 2}) && Welds == std::vector<FIndex>({30, 10, 20}), "Inputs mutated");
}

void ExactProductionBudget()
{
    const std::size_t Cap = 32768;
    std::vector<FIndex> Input, Welds;
    std::vector<std::uint8_t> Selection;
    // 10922 independent selected triangles require 32766 raw midpoints.
    for (FIndex I = 0; I < 32766; ++I) { Input.push_back(I); Welds.push_back(I); }
    Selection.assign(Input.size() / 3, 1);
    // A UV-seam copy sharing raw AB but duplicating C adds exactly two more.
    Welds.push_back(2);
    Input.insert(Input.end(), {0, 1, 32766});
    Selection.push_back(0);
    const auto Exact = PlanPass(Input, Welds, Selection, Cap);
    Check(Exact.Status == EStatus::Applied && Exact.Midpoints.size() == Cap,
        "Exactly 32768 added vertices must fit the inclusive cap");
    const auto Rejected = PlanPass(Input, Welds, Selection, Cap - 1);
    Check(Rejected.Status == EStatus::BudgetExceeded && Rejected.Midpoints.empty()
        && Rejected.Indices.empty() && Rejected.WeldIds.empty(),
        "A nearly complete oversized pass must be rejected atomically");
}

void ClosedCubeSphereSeams()
{
    struct FPoint3 { double X, Y, Z; };
    const auto Unit = [](FPoint3 P)
    { const double L = std::sqrt(P.X * P.X + P.Y * P.Y + P.Z * P.Z); return FPoint3{P.X / L, P.Y / L, P.Z / L}; };
    const FPoint3 Faces[6][3] = {
        {{1,0,0},{0,1,0},{0,0,1}}, {{-1,0,0},{0,-1,0},{0,0,1}},
        {{0,1,0},{-1,0,0},{0,0,1}}, {{0,-1,0},{1,0,0},{0,0,1}},
        {{0,0,1},{1,0,0},{0,1,0}}, {{0,0,-1},{-1,0,0},{0,1,0}}};
    std::vector<FPoint3> Directions;
    std::vector<FIndex> Indices, Welds;
    std::unordered_map<int, FIndex> InitialWelds;
    for (const auto& Face : Faces)
    {
        const FIndex Start = FIndex(Directions.size());
        for (int V = -1; V <= 1; ++V) for (int U = -1; U <= 1; ++U)
        {
            const FPoint3 P = {Face[0].X + U * Face[1].X + V * Face[2].X,
                Face[0].Y + U * Face[1].Y + V * Face[2].Y, Face[0].Z + U * Face[1].Z + V * Face[2].Z};
            const int Key = (int(P.X) + 1) * 9 + (int(P.Y) + 1) * 3 + int(P.Z) + 1;
            const auto Weld = InitialWelds.emplace(Key, FIndex(InitialWelds.size()));
            Directions.push_back(Unit(P)); Welds.push_back(Weld.first->second);
        }
        for (FIndex Y = 0; Y < 2; ++Y) for (FIndex X = 0; X < 2; ++X)
        {
            const FIndex A = Start + Y * 3 + X, B = A + 1, C = A + 3, D = C + 1;
            Indices.insert(Indices.end(), {A, D, B, A, C, D});
        }
    }
    Check(Directions.size() == 54 && InitialWelds.size() == 26, "Closed cube fixture welds invalid");
    for (unsigned Pass = 0; Pass < 2; ++Pass)
    {
        const auto Side = [&Directions](FIndex I) { const auto& P = Directions[I]; return P.X + 0.37 * P.Y + 0.13 * P.Z - 0.17; };
        std::vector<std::uint8_t> Selected(Indices.size() / 3, 0);
        for (std::size_t T = 0; T < Selected.size(); ++T)
        {
            const double A = Side(Indices[3 * T]), B = Side(Indices[3 * T + 1]), C = Side(Indices[3 * T + 2]);
            Selected[T] = std::min(A, std::min(B, C)) < 0 && std::max(A, std::max(B, C)) >= 0;
        }
        const auto Plan = PlanPass(Indices, Welds, Selected, 32768);
        Check(Plan.Status == EStatus::Applied, "Closed cube coastal pass was not applied");
        for (const auto& M : Plan.Midpoints)
        { const auto A = Directions[M.A], B = Directions[M.B]; Directions.push_back(Unit({A.X + B.X, A.Y + B.Y, A.Z + B.Z})); }
        Indices = Plan.Indices; Welds = Plan.WeldIds;
        std::unordered_map<FIndex, FPoint3> SampleByWeld;
        std::vector<FPoint3> Positions;
        for (std::size_t I = 0; I < Directions.size(); ++I)
        {
            const auto& D = Directions[I];
            const double R = 1.0 + 0.03 * (D.X * D.Y + D.Z * D.Z); // Synthetic deterministic radial sample.
            const FPoint3 P = {D.X * R, D.Y * R, D.Z * R}; Positions.push_back(P);
            const auto Found = SampleByWeld.emplace(Welds[I], P);
            const auto& Other = Found.first->second;
            Check(P.X == Other.X && P.Y == Other.Y && P.Z == Other.Z, "Duplicate seam samples diverged");
        }
        std::unordered_map<std::uint64_t, std::pair<unsigned, int>> Edges;
        for (std::size_t T = 0; T < Indices.size(); T += 3)
        {
            const auto A = Positions[Indices[T]], B = Positions[Indices[T + 1]], C = Positions[Indices[T + 2]];
            const FPoint3 AB = {B.X-A.X,B.Y-A.Y,B.Z-A.Z}, AC = {C.X-A.X,C.Y-A.Y,C.Z-A.Z};
            const double Outward = (AB.Y*AC.Z-AB.Z*AC.Y)*(A.X+B.X+C.X)
                + (AB.Z*AC.X-AB.X*AC.Z)*(A.Y+B.Y+C.Y) + (AB.X*AC.Y-AB.Y*AC.X)*(A.Z+B.Z+C.Z);
            Check(Outward < 0, "Clockwise cube winding flipped after radial sampling");
            for (unsigned E = 0; E < 3; ++E)
            {
                const FIndex WA = Welds[Indices[T + E]], WB = Welds[Indices[T + (E + 1) % 3]];
                auto& Edge = Edges[Detail::EdgeKey(WA, WB)]; ++Edge.first; Edge.second += WA < WB ? 1 : -1;
            }
        }
        for (const auto& Edge : Edges)
            Check(Edge.second.first == 2 && Edge.second.second == 0, "Closed sphere seam is open, nonconforming or misoriented");
    }
}
}

int main()
{
    try
    {
        AllEightMasks();
        SharedAndSeamEdges();
        DeterminismAndValidation();
        ExactProductionBudget();
        ClosedCubeSphereSeams();
        std::cout << "APSCoastalMeshRefinement: PASS (8 masks, both windings, seams, two passes, closed sphere, exact 32768 cap, atomic rejection, validation)\n";
        return 0;
    }
    catch (const std::exception& Error)
    {
        std::cerr << "APSCoastalMeshRefinement: FAIL: " << Error.what() << '\n';
        return 1;
    }
}
