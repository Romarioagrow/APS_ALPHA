#include "APSBoundedFoliageSeed.h"
#include <set>
#include <cstdio>
#include <limits>

int main()
{
    using namespace APSBoundedFoliageSeed;
    int Failures = 0;
    const auto Check = [&](bool OK, const char* Name)
    { std::printf("%s %s\n", OK ? "PASS" : "FAIL", Name); if (!OK) ++Failures; };
    const auto S = Make(100, -34, 67, 1337, 0, 0, 0);
    Check(S == Make(100, -34, 67, 1337, 0, 0, 0), "repeatable across unload/reload");
    Check(S != Make(100, -33, 67, 1337, 0, 0, 0), "first species depends on Y");
    Check(S != Make(100, -34, 68, 1337, 0, 0, 0), "first species depends on Z");
    Check(S != Make(101, -34, 67, 1337, 0, 0, 0), "first species depends on X");
    Check(S != Make(100, -34, 67, 1338, 0, 0, 0), "planet seed participates");
    Check(S != Make(100, -34, 67, 1337, 0, 1, 0), "species streams differ");
    Check(S != Make(100, -34, 67, 1337, 1, 0, 0), "collection streams differ");
    Check(S != Make(100, -34, 67, 1337, 0, 0, 1), "layer streams differ");
    Check(Make(-1, 0, 0, 1337, 0, 0, 0) != Make(1, 0, 0, 1337, 0, 0, 0), "negative cells distinct");
    Check(Make(1ll << 33, 0, 0, 1337, 0, 0, 0) != Make(0, 0, 0, 1337, 0, 0, 0), "high coordinate bits retained");
    Check(S == ForSector(800000, -272000, 536000, 8000, 1337, 0, 0, 0), "physical centimetres to local cells");
    Check(ForSector(1, 1, 1, 0, 1337, 0, 0, 0) == 0, "zero size rejected");
    Check(ForSector(1.e308, 1, 1, 1, 1337, 0, 0, 0) == 0, "integer overflow rejected");
    Check(ForSector(std::numeric_limits<double>::quiet_NaN(), 1, 1, 1, 1337, 0, 0, 0) == 0, "NaN rejected");
    std::set<std::uint32_t> Seeds;
    for (int X = -4; X <= 4; ++X) for (int Y = -4; Y <= 4; ++Y) for (int Z = -4; Z <= 4; ++Z)
        Seeds.insert(Make(X, Y, Z, 1337, 0, 0, 0));
    Check(Seeds.size() == 729, "729 retained neighbour cells have distinct streams in this fixture");
    std::printf("failures=%d; CPU seed contract only, not rendered placement acceptance\n", Failures);
    return Failures == 0 ? 0 : 1;
}
