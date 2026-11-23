#pragma once

// Prototype for issue #1202, step 1.2: can a function template give every call site its own static storage?
//
// The macros declare "static caf::PdmUiItemInfo objDescr( ... )" at each call site. A function can get the same effect
// if every call instantiates a distinct specialization. Here the distinct type is the closure type of a lambda used as
// default template argument. This test verifies that slots are unique per call site, also across translation units.

struct Slot
{
    const char* owner = nullptr;
};

class Object
{
public:
    template <typename FieldType, typename Tag = decltype( [] {} )>
    Slot* slotForCallSite( FieldType& )
    {
        static Slot slot;
        return &slot;
    }
};

// A constructor defined in a header, and included in several translation units
class HeaderObject : public Object
{
public:
    HeaderObject()
    {
        m_slotA = slotForCallSite( m_a );
        m_slotB = slotForCallSite( m_b );
    }

    int   m_a     = 0;
    int   m_b     = 0;
    Slot* m_slotA = nullptr;
    Slot* m_slotB = nullptr;
};

struct SlotList
{
    Slot* slots[8];
    int   count;
};

SlotList slotsFromUnitOne();
SlotList slotsFromUnitTwo();
