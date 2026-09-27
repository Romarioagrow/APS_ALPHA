#include "APS_ALPHA/Core/Planetary/APSWorldScapeSurfaceEnvelope.h"
#include <cassert>
#include <limits>
#include <cstdio>
#include <initializer_list>

int main()
{
    using namespace APSWorldScapeSurfaceEnvelope;
    assert(Eligible(true, true, false, 1.0));
    assert(!Eligible(false, true, false, 1.0)); // water, ammonia, dry
    assert(!Eligible(true, false, false, 1.0));
    assert(!Eligible(true, true, true, 1.0));
    assert(!Eligible(true, true, false, 0.002));
    assert(!Eligible(true, true, false, std::numeric_limits<double>::quiet_NaN()));
    unsigned Cases = 0;
    for (double Sea : {-75000.0, 0.0, 43723.773})
        for (int I = -50000; I <= 50000; ++I)
        {
            const double Rock = I * 17.123;
            const double Visible = Rock > Sea ? Rock : Sea;
            const double Unified = Height(Rock, Sea, true);
            assert(Unified == Visible);
            assert(Height(Unified, Sea, true) == Unified);
            assert(Height(Rock, Sea, false) == Rock);
            ++Cases;
        }
    std::printf("PASS: %u upper-envelope cases, identity/idempotence and family eligibility. Math only, not rendered proof.\n", Cases);
}
