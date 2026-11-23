// Compile-time benchmark for issue #1202: same objects as bench_scriptable_macro.cpp, initialized with functions using
// fixed string keywords.

#include "cafProtoScripting.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <vector>

// clang-format off

#define BENCH_SCRIPT_OBJECT( N ) \
class BenchScriptFsObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchScriptFsObj##N() \
    { \
        initObject( "BenchScriptObj" #N ).withScripting(); \
        initFieldN<"IntVal">( m_intVal, "Int" ).withDefault( 0 ).withScripting(); \
        initFieldN<"DblVal">( m_dblVal, "Double" ).withDefault( 0.0 ).withScripting(); \
        initFieldN<"BoolVal">( m_boolVal, "Bool" ).withDefault( false ).withScripting(); \
        initFieldN<"StrVal">( m_strVal, "String" ).withDefault( QString() ).withScripting().withToolTip( "A tool tip" ); \
        initFieldN<"FltVal">( m_fltVal, "Float" ).withDefault( 0.0f ).withScripting(); \
        initFieldN<"VecDbl">( m_vecDbl, "Vector of doubles" ).withScripting(); \
        initFieldN<"VecStr">( m_vecStr, "Vector of strings" ).withScripting(); \
        initFieldN<"VecInt">( m_vecInt, "Vector of ints" ).withScripting(); \
        initField( m_legacy, "legacy_keyword", 0, "Legacy" ).withScriptKeyword( "LegacyKeyword" ); \
        initField( m_plain, "Plain", 0, "Not scriptable" ); \
    } \
    caf::PdmField<int>                   m_intVal; \
    caf::PdmField<double>                m_dblVal; \
    caf::PdmField<bool>                  m_boolVal; \
    caf::PdmField<QString>               m_strVal; \
    caf::PdmField<float>                 m_fltVal; \
    caf::PdmField<std::vector<double>>   m_vecDbl; \
    caf::PdmField<std::vector<QString>>  m_vecStr; \
    caf::PdmField<std::vector<int>>      m_vecInt; \
    caf::PdmField<int>                   m_legacy; \
    caf::PdmField<int>                   m_plain; \
};

BENCH_SCRIPT_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj01, "BenchScriptFsObj01" );
BENCH_SCRIPT_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj02, "BenchScriptFsObj02" );
BENCH_SCRIPT_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj03, "BenchScriptFsObj03" );
BENCH_SCRIPT_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj04, "BenchScriptFsObj04" );
BENCH_SCRIPT_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj05, "BenchScriptFsObj05" );
BENCH_SCRIPT_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj06, "BenchScriptFsObj06" );
BENCH_SCRIPT_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj07, "BenchScriptFsObj07" );
BENCH_SCRIPT_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj08, "BenchScriptFsObj08" );
BENCH_SCRIPT_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj09, "BenchScriptFsObj09" );
BENCH_SCRIPT_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj10, "BenchScriptFsObj10" );
BENCH_SCRIPT_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj11, "BenchScriptFsObj11" );
BENCH_SCRIPT_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj12, "BenchScriptFsObj12" );
BENCH_SCRIPT_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj13, "BenchScriptFsObj13" );
BENCH_SCRIPT_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj14, "BenchScriptFsObj14" );
BENCH_SCRIPT_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj15, "BenchScriptFsObj15" );
BENCH_SCRIPT_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj16, "BenchScriptFsObj16" );
BENCH_SCRIPT_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj17, "BenchScriptFsObj17" );
BENCH_SCRIPT_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj18, "BenchScriptFsObj18" );
BENCH_SCRIPT_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj19, "BenchScriptFsObj19" );
BENCH_SCRIPT_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj20, "BenchScriptFsObj20" );

// clang-format on
