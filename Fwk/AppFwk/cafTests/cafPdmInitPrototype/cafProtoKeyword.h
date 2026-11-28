#pragma once

// Prototype for issue #1202, step 1.3: compile-time validated keyword types.
//
// This header is self-contained and does not depend on Qt, so it can be compiled by MSVC, GCC and Clang in isolation.
// The rules are the same as the ones enforced by CAF_PDM_VERIFY_XML_KEYWORD and CAF_PDM_CheckScriptableKeyword.

#include <cstddef>
#include <string_view>

namespace cafproto
{
namespace keyword_detail
{
    constexpr bool isLower( char c )
    {
        return c >= 'a' && c <= 'z';
    }

    constexpr bool isUpper( char c )
    {
        return c >= 'A' && c <= 'Z';
    }

    constexpr bool isDigit( char c )
    {
        return c >= '0' && c <= '9';
    }

    constexpr bool isValidFirstCharacterInXmlKeyword( char c )
    {
        return ( c != '.' ) && !isDigit( c );
    }

    constexpr bool isValidCharacterInXmlKeyword( char c )
    {
        return isLower( c ) || isUpper( c ) || isDigit( c ) || ( c == '_' );
    }

    constexpr bool startsWithXml( std::string_view text )
    {
        return text.size() >= 3 && text[0] == 'x' && text[1] == 'm' && text[2] == 'l';
    }

    constexpr bool hasOnlyValidXmlCharacters( std::string_view text )
    {
        for ( char c : text )
        {
            if ( !isValidCharacterInXmlKeyword( c ) ) return false;
        }
        return true;
    }

    constexpr bool isValidXmlKeyword( std::string_view text )
    {
        return !text.empty() && isValidFirstCharacterInXmlKeyword( text[0] ) && !startsWithXml( text ) &&
               hasOnlyValidXmlCharacters( text );
    }

    // Same rules as isCamelCase() in cafPdmFieldScriptingCapability.h
    constexpr bool isCamelCase( std::string_view text )
    {
        if ( text.empty() ) return false;

        bool hasLower       = false;
        int  prevUpperCount = 0;

        if ( isUpper( text[0] ) || isDigit( text[0] ) ) prevUpperCount++;

        for ( std::size_t i = 1; i < text.size(); ++i )
        {
            if ( isUpper( text[i] ) || isDigit( text[i] ) )
            {
                if ( prevUpperCount >= 2 ) return false;
                prevUpperCount++;
            }
            else if ( isLower( text[i] ) )
            {
                prevUpperCount = 0;
                hasLower       = true;
            }
            else
            {
                return false;
            }
        }

        if ( prevUpperCount > 1 ) return false;

        return hasLower;
    }

    constexpr bool isValidScriptKeyword( std::string_view text )
    {
        return isCamelCase( text ) && isUpper( text[0] );
    }

    // These functions are intentionally not constexpr. Reaching one of them during constant evaluation is a compile
    // error, and the compiler reports the function name as the reason.
    void keywordIsEmpty();
    void keywordFirstCharacterIsInvalid();
    void keywordStartsWithXml();
    void keywordContainsInvalidCharacter();
    void scriptKeywordIsNotCamelCase();
    void scriptKeywordMustStartWithUppercaseLetter();

    constexpr void verifyXmlKeyword( std::string_view text )
    {
        if ( text.empty() ) keywordIsEmpty();
        if ( !text.empty() && !isValidFirstCharacterInXmlKeyword( text[0] ) ) keywordFirstCharacterIsInvalid();
        if ( startsWithXml( text ) ) keywordStartsWithXml();
        if ( !hasOnlyValidXmlCharacters( text ) ) keywordContainsInvalidCharacter();
    }

    constexpr void verifyScriptKeyword( std::string_view text, bool allowLowercaseFirstCharacter )
    {
        if ( !isCamelCase( text ) ) scriptKeywordIsNotCamelCase();
        if ( !allowLowercaseFirstCharacter && !text.empty() && !isUpper( text[0] ) )
            scriptKeywordMustStartWithUppercaseLetter();
    }
} // namespace keyword_detail

//==================================================================================================
/// Keyword used for XML serialization of a field or a class. An invalid string literal is a compile
/// error. Replaces CAF_PDM_VERIFY_XML_KEYWORD.
//==================================================================================================
class PdmKeyword
{
public:
    consteval PdmKeyword( const char* text )
        : m_text( text )
    {
        keyword_detail::verifyXmlKeyword( m_text );
    }

    constexpr std::string_view view() const { return m_text; }

private:
    std::string_view m_text;
};

//==================================================================================================
/// Keyword exposed to scripting. Replaces CAF_PDM_CheckScriptableKeyword.
///
/// A camel case keyword starting with an uppercase letter contains only letters and digits, so it is
/// always a valid XML keyword. The same literal can be used for both purposes.
//==================================================================================================
class PdmScriptKeyword
{
public:
    consteval PdmScriptKeyword( const char* text )
        : m_text( text )
    {
        keyword_detail::verifyScriptKeyword( m_text, false );
    }

    constexpr std::string_view view() const { return m_text; }

private:
    std::string_view m_text;
};

//==================================================================================================
/// Scripting keyword for classes that must keep legacy keywords starting with a lowercase letter
/// (the command file interface). Replaces the allowLowercaseScriptKeywords class member.
//==================================================================================================
class PdmLegacyScriptKeyword
{
public:
    consteval PdmLegacyScriptKeyword( const char* text )
        : m_text( text )
    {
        keyword_detail::verifyScriptKeyword( m_text, true );
        keyword_detail::verifyXmlKeyword( m_text );
    }

    constexpr std::string_view view() const { return m_text; }

private:
    std::string_view m_text;
};

} // namespace cafproto
