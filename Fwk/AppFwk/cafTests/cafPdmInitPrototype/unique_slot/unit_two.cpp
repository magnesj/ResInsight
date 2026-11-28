#include "unique_slot.h"

// The two translation units are intentionally token-identical except for the function name

namespace
{
class LocalObject : public Object
{
public:
    LocalObject()
    {
        m_slotA = slotForCallSite( m_a );
        m_slotB = slotForCallSite( m_b );
        m_slotC = slotForCallSite( m_c );
        m_slotD = slotForCallSite( m_d );
    }

    int    m_a = 0;
    int    m_b = 0;
    double m_c = 0;
    double m_d = 0;
    Slot*  m_slotA;
    Slot*  m_slotB;
    Slot*  m_slotC;
    Slot*  m_slotD;
};
} // namespace

SlotList slotsFromUnitTwo()
{
    LocalObject  first;
    LocalObject  second;
    HeaderObject header;

    // Two instances of the same class must share the slots
    if ( first.m_slotA != second.m_slotA || first.m_slotD != second.m_slotD ) return { {}, -1 };

    return { { first.m_slotA, first.m_slotB, first.m_slotC, first.m_slotD, header.m_slotA, header.m_slotB }, 6 };
}
