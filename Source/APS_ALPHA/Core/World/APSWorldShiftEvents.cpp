#include "APSWorldShiftEvents.h"

APSWorldShiftEvents::FPostDoubleShift& APSWorldShiftEvents::OnPostDoubleShift()
{
    static FPostDoubleShift Event;
    return Event;
}
