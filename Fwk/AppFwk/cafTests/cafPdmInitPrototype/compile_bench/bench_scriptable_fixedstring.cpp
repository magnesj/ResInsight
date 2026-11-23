// Compile-time benchmark for issue #1202: same objects as bench_scriptable_macro.cpp, initialized with functions using
// fixed string keywords.

#include "cafProtoScripting.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

// The keywords are unique for each class, as in application code

#include <QString>
#include <vector>

// clang-format off

#define BENCH_SCRIPT_OBJECT( N, S ) \
class BenchScriptFsObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchScriptFsObj##N() \
    { \
        initObject( "BenchScriptObj" #N ).withScripting(); \
        initFieldN<"IntVal" #S>( m_intVal, "Int" ).withDefault( 0 ).withScripting(); \
        initFieldN<"DblVal" #S>( m_dblVal, "Double" ).withDefault( 0.0 ).withScripting(); \
        initFieldN<"BoolVal" #S>( m_boolVal, "Bool" ).withDefault( false ).withScripting(); \
        initFieldN<"StrVal" #S>( m_strVal, "String" ).withDefault( QString() ).withScripting().withToolTip( "A tool tip" ); \
        initFieldN<"FltVal" #S>( m_fltVal, "Float" ).withDefault( 0.0f ).withScripting(); \
        initFieldN<"VecDbl" #S>( m_vecDbl, "Vector of doubles" ).withScripting(); \
        initFieldN<"VecStr" #S>( m_vecStr, "Vector of strings" ).withScripting(); \
        initFieldN<"VecInt" #S>( m_vecInt, "Vector of ints" ).withScripting(); \
        initField( m_legacy, "legacy_keyword" #N, 0, "Legacy" ).withScriptKeyword( "LegacyKeyword" ); \
        initField( m_plain, "Plain" #S, 0, "Not scriptable" ); \
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

BENCH_SCRIPT_OBJECT( 01, Aa ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj01, "BenchScriptFsObj01" );
BENCH_SCRIPT_OBJECT( 02, Ab ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj02, "BenchScriptFsObj02" );
BENCH_SCRIPT_OBJECT( 03, Ac ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj03, "BenchScriptFsObj03" );
BENCH_SCRIPT_OBJECT( 04, Ad ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj04, "BenchScriptFsObj04" );
BENCH_SCRIPT_OBJECT( 05, Ae ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj05, "BenchScriptFsObj05" );
BENCH_SCRIPT_OBJECT( 06, Af ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj06, "BenchScriptFsObj06" );
BENCH_SCRIPT_OBJECT( 07, Ag ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj07, "BenchScriptFsObj07" );
BENCH_SCRIPT_OBJECT( 08, Ah ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj08, "BenchScriptFsObj08" );
BENCH_SCRIPT_OBJECT( 09, Ai ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj09, "BenchScriptFsObj09" );
BENCH_SCRIPT_OBJECT( 10, Aj ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj10, "BenchScriptFsObj10" );
BENCH_SCRIPT_OBJECT( 11, Ak ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj11, "BenchScriptFsObj11" );
BENCH_SCRIPT_OBJECT( 12, Al ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj12, "BenchScriptFsObj12" );
BENCH_SCRIPT_OBJECT( 13, Am ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj13, "BenchScriptFsObj13" );
BENCH_SCRIPT_OBJECT( 14, An ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj14, "BenchScriptFsObj14" );
BENCH_SCRIPT_OBJECT( 15, Ao ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj15, "BenchScriptFsObj15" );
BENCH_SCRIPT_OBJECT( 16, Ap ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj16, "BenchScriptFsObj16" );
BENCH_SCRIPT_OBJECT( 17, Aq ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj17, "BenchScriptFsObj17" );
BENCH_SCRIPT_OBJECT( 18, Ar ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj18, "BenchScriptFsObj18" );
BENCH_SCRIPT_OBJECT( 19, As ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj19, "BenchScriptFsObj19" );
BENCH_SCRIPT_OBJECT( 20, At ) CAF_PDM_SOURCE_INIT( BenchScriptFsObj20, "BenchScriptFsObj20" );

// clang-format on
