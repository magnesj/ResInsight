// Prototype for issue #1202, step 1.3.
//
// Self-contained model of the two API candidates for compile-time keyword validation. No Qt and no PDM dependencies,
// so the same file can be compiled by MSVC, GCC and Clang.
//
//   Candidate A: keyword types with consteval constructors (cafProtoKeyword.h)
//   Candidate B: keyword as a fixed string non-type template parameter (cafProtoFixedString.h)
//
// Compile with -DCASE=0 to build all valid uses. Each CASE > 0 enables exactly one invalid use, and must fail to
// compile. See run_keyword_compile_tests.py.

#include "cafProtoFixedString.h"
#include "cafProtoKeyword.h"

#include <string_view>

#ifndef CASE
#define CASE 0
#endif

namespace model
{
struct Field
{
    std::string_view keyword;
    std::string_view scriptKeyword;
};

//==================================================================================================
// Candidate A
//==================================================================================================
class BuilderA
{
public:
    explicit BuilderA( Field& field )
        : m_field( field )
    {
    }

    // Field keyword and scripting keyword are different
    BuilderA& withScriptKeyword( cafproto::PdmScriptKeyword scriptKeyword )
    {
        m_field.scriptKeyword = scriptKeyword.view();
        return *this;
    }

    BuilderA& withToolTip( const char* ) { return *this; }

private:
    Field& m_field;
};

class ObjectA
{
public:
    BuilderA initField( Field& field, cafproto::PdmKeyword keyword )
    {
        field.keyword = keyword.view();
        return BuilderA( field );
    }

    // The field keyword is also used as scripting keyword
    BuilderA initScriptableField( Field& field, cafproto::PdmScriptKeyword keyword )
    {
        field.keyword       = keyword.view();
        field.scriptKeyword = keyword.view();
        return BuilderA( field );
    }
};

// Equivalent of RicfCommandObject, which declares allowLowercaseScriptKeywords = true. The function in the base class
// is hidden, so all classes derived from this class accept legacy lowercase scripting keywords.
class LegacyObjectA : public ObjectA
{
public:
    BuilderA initScriptableField( Field& field, cafproto::PdmLegacyScriptKeyword keyword )
    {
        field.keyword       = keyword.view();
        field.scriptKeyword = keyword.view();
        return BuilderA( field );
    }
};

//==================================================================================================
// Candidate B
//==================================================================================================
template <cafproto::FixedString Keyword, bool AllowLowercase>
class BuilderB
{
public:
    explicit BuilderB( Field& field )
        : m_field( field )
    {
    }

    // The keyword is part of the builder type, so a parameterless call can be verified at compile time
    BuilderB& withScripting()
    {
        static_assert( AllowLowercase ? cafproto::isValidLegacyScriptKeywordV<Keyword>
                                      : cafproto::isValidScriptKeywordV<Keyword>,
                       "Keyword used for scripting must be in compatible formatted camel casing starting with an "
                       "uppercase letter" );
        m_field.scriptKeyword = m_field.keyword;
        return *this;
    }

    template <cafproto::FixedString ScriptKeyword>
    BuilderB& withScriptKeyword()
    {
        static_assert( cafproto::isValidScriptKeywordV<ScriptKeyword>,
                       "Keyword used for scripting must be in compatible formatted camel casing starting with an "
                       "uppercase letter" );
        static constexpr auto text = ScriptKeyword;
        m_field.scriptKeyword      = text.view();
        return *this;
    }

    BuilderB& withToolTip( const char* ) { return *this; }

private:
    Field& m_field;
};

class ObjectB
{
public:
    template <cafproto::FixedString Keyword>
    BuilderB<Keyword, false> initField( Field& field )
    {
        static_assert( cafproto::isValidXmlKeywordV<Keyword>, "Invalid XML keyword" );
        static constexpr auto text = Keyword;
        field.keyword              = text.view();
        return BuilderB<Keyword, false>( field );
    }
};

class LegacyObjectB : public ObjectB
{
public:
    template <cafproto::FixedString Keyword>
    BuilderB<Keyword, true> initField( Field& field )
    {
        static_assert( cafproto::isValidXmlKeywordV<Keyword>, "Invalid XML keyword" );
        static constexpr auto text = Keyword;
        field.keyword              = text.view();
        return BuilderB<Keyword, true>( field );
    }
};

//==================================================================================================
// Valid uses, compiled for all values of CASE
//==================================================================================================
class ValidA : public ObjectA
{
public:
    ValidA()
    {
        initField( m_a, "Name" );
        initField( m_b, "m_singleFilePath" );
        initField( m_c, "Xml_Is_Only_Rejected_In_Lowercase" );
        initField( m_d, "lowercaseXmlKeyword" ).withScriptKeyword( "ScriptName" ).withToolTip( "tip" );
        initScriptableField( m_e, "WellPath" ).withToolTip( "tip" );
        initScriptableField( m_f, "Name2" );
    }

    Field m_a, m_b, m_c, m_d, m_e, m_f;
};

class ValidLegacyA : public LegacyObjectA
{
public:
    ValidLegacyA()
    {
        initField( m_a, "Name" );
        initScriptableField( m_b, "caseId" );
        initScriptableField( m_c, "WellPath" );
    }

    Field m_a, m_b, m_c;
};

class ValidB : public ObjectB
{
public:
    ValidB()
    {
        initField<"Name">( m_a );
        initField<"m_singleFilePath">( m_b );
        initField<"Xml_Is_Only_Rejected_In_Lowercase">( m_c );
        initField<"lowercaseXmlKeyword">( m_d ).withScriptKeyword<"ScriptName">().withToolTip( "tip" );
        initField<"WellPath">( m_e ).withToolTip( "tip" ).withScripting();
        initField<"Name2">( m_f ).withScripting();
    }

    Field m_a, m_b, m_c, m_d, m_e, m_f;
};

class ValidLegacyB : public LegacyObjectB
{
public:
    ValidLegacyB()
    {
        initField<"Name">( m_a );
        initField<"caseId">( m_b ).withScripting();
        initField<"WellPath">( m_c ).withScripting();
    }

    Field m_a, m_b, m_c;
};

static_assert( cafproto::PdmKeyword( "Name" ).view() == "Name" );
static_assert( cafproto::PdmScriptKeyword( "WellPath" ).view() == "WellPath" );
static_assert( cafproto::PdmLegacyScriptKeyword( "caseId" ).view() == "caseId" );
static_assert( cafproto::FixedString( "Name" ).view() == "Name" );
static_assert( sizeof( cafproto::PdmKeyword ) == sizeof( std::string_view ) );

//==================================================================================================
// Invalid uses, one per CASE. Every case must be rejected by the compiler.
//==================================================================================================
class InvalidA : public ObjectA
{
public:
    InvalidA()
    {
#if CASE == 1
        initField( m_a, "My Keyword" );
#elif CASE == 2
        initField( m_a, "1Keyword" );
#elif CASE == 3
        initField( m_a, "xmlKeyword" );
#elif CASE == 4
        initField( m_a, "" );
#elif CASE == 5
        initScriptableField( m_a, "lowercase" );
#elif CASE == 6
        initScriptableField( m_a, "ABCKeyword" );
#elif CASE == 7
        initField( m_a, "Name" ).withScriptKeyword( "has_underscore" );
#elif CASE == 8
        // A keyword must be a constant expression
        const char* runtimeKeyword = m_a.keyword.data();
        initField( m_a, runtimeKeyword );
#elif CASE == 9
        initScriptableField( m_a, "Name.Sub" );
#endif
    }

    Field m_a;
};

class InvalidLegacyA : public LegacyObjectA
{
public:
    InvalidLegacyA()
    {
#if CASE == 10
        initScriptableField( m_a, "has_underscore" );
#elif CASE == 11
        initScriptableField( m_a, "xmlLowercase" );
#endif
    }

    Field m_a;
};

class InvalidB : public ObjectB
{
public:
    InvalidB()
    {
#if CASE == 21
        initField<"My Keyword">( m_a );
#elif CASE == 22
        initField<"1Keyword">( m_a );
#elif CASE == 23
        initField<"xmlKeyword">( m_a );
#elif CASE == 24
        initField<"">( m_a );
#elif CASE == 25
        initField<"lowercase">( m_a ).withScripting();
#elif CASE == 26
        initField<"ABCKeyword">( m_a ).withScripting();
#elif CASE == 27
        initField<"Name">( m_a ).withScriptKeyword<"has_underscore">();
#elif CASE == 28
        // The check also works when other builder calls are placed between initField and withScripting
        initField<"lowercase">( m_a ).withToolTip( "tip" ).withScripting();
#endif
    }

    Field m_a;
};

class InvalidLegacyB : public LegacyObjectB
{
public:
    InvalidLegacyB()
    {
#if CASE == 30
        initField<"has_underscore">( m_a ).withScripting();
#endif
    }

    Field m_a;
};

} // namespace model

int main()
{
    model::ValidA       a;
    model::ValidLegacyA legacyA;
    model::ValidB       b;
    model::ValidLegacyB legacyB;

    bool ok = a.m_d.keyword == "lowercaseXmlKeyword" && a.m_d.scriptKeyword == "ScriptName" &&
              a.m_e.scriptKeyword == "WellPath" && legacyA.m_b.scriptKeyword == "caseId" &&
              b.m_d.keyword == "lowercaseXmlKeyword" && b.m_d.scriptKeyword == "ScriptName" &&
              b.m_e.scriptKeyword == "WellPath" && legacyB.m_b.scriptKeyword == "caseId";

    return ok ? 0 : 1;
}
