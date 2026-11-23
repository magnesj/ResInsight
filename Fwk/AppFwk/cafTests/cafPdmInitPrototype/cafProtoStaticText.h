#pragma once

// Prototype for issue #1202, step 1.2: text with static storage duration.

namespace cafproto
{
//==================================================================================================
/// Text given as a string literal. The constructor is consteval, so the pointer always refers to an
/// object with static storage duration and constant content. This makes it valid to keep the
/// pointer, and to use the pointer value as identity for the text.
//==================================================================================================
class PdmStaticText
{
public:
    consteval PdmStaticText( const char* text )
        : m_text( text )
    {
    }

    constexpr const char* c_str() const { return m_text; }

private:
    const char* m_text;
};

} // namespace cafproto
