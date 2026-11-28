#include "unique_slot.h"

#include <cstdio>
#include <set>

int main()
{
    SlotList one = slotsFromUnitOne();
    SlotList two = slotsFromUnitTwo();

    if ( one.count != 6 || two.count != 6 )
    {
        std::printf( "FAIL: instances of the same class do not share slots\n" );
        return 1;
    }

    // Slots 0-3 belong to classes local to each translation unit: 8 distinct slots expected.
    std::set<Slot*> localSlots;
    for ( int i = 0; i < 4; ++i )
    {
        localSlots.insert( one.slots[i] );
        localSlots.insert( two.slots[i] );
    }

    // Slots 4-5 belong to a class with an inline constructor in a header: the two fields must have
    // distinct slots. The slots may or may not be shared between the translation units.
    bool headerSlotsDistinct = one.slots[4] != one.slots[5] && two.slots[4] != two.slots[5];
    bool headerSlotsShared   = one.slots[4] == two.slots[4] && one.slots[5] == two.slots[5];
    bool headerSlotsCrossed  = one.slots[4] == two.slots[5] || one.slots[5] == two.slots[4];

    std::set<Slot*> allSlots = localSlots;
    for ( int i = 4; i < 6; ++i )
    {
        allSlots.insert( one.slots[i] );
        allSlots.insert( two.slots[i] );
    }

    std::printf( "distinct local slots: %d of 8\n", (int)localSlots.size() );
    std::printf( "header slots distinct per field: %s, shared between translation units: %s\n",
                 headerSlotsDistinct ? "yes" : "no",
                 headerSlotsShared ? "yes" : "no" );
    std::printf( "distinct slots in total: %d (expected %d)\n", (int)allSlots.size(), headerSlotsShared ? 10 : 12 );

    bool ok = localSlots.size() == 8 && headerSlotsDistinct && !headerSlotsCrossed &&
              allSlots.size() == ( headerSlotsShared ? 10u : 12u );
    std::printf( "%s\n", ok ? "OK" : "FAIL" );
    return ok ? 0 : 1;
}
