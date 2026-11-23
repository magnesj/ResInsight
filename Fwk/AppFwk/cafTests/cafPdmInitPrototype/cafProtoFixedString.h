#pragma once

// Prototype for issue #1202, step 1.3: keyword as a fixed string non-type template parameter.
//
// Alternative to the consteval keyword types in cafProtoKeyword.h. The keyword is part of the type, so later calls in
// a builder chain can use static_assert on it. The cost is one set of template instantiations per keyword.

#include "cafProtoKeyword.h"

#include <cstddef>
#include <string_view>

namespace cafproto
{
template <std::size_t N>
struct FixedString
{
    constexpr FixedString( const char ( &text )[N] )
    {
        for ( std::size_t i = 0; i < N; ++i )
            value[i] = text[i];
    }

    constexpr std::string_view view() const { return std::string_view( value, N - 1 ); }

    char value[N] = {};
};

template <FixedString Keyword>
inline constexpr bool isValidXmlKeywordV = keyword_detail::isValidXmlKeyword( Keyword.view() );

template <FixedString Keyword>
inline constexpr bool isValidScriptKeywordV = keyword_detail::isValidScriptKeyword( Keyword.view() );

template <FixedString Keyword>
inline constexpr bool isValidLegacyScriptKeywordV =
    keyword_detail::isCamelCase( Keyword.view() ) && keyword_detail::isValidXmlKeyword( Keyword.view() );

} // namespace cafproto
