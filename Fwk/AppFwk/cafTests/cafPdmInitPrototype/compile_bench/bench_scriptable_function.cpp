// Compile-time benchmark for issue #1202: same objects as bench_scriptable_macro.cpp, initialized with functions.

#include "cafProtoScripting.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

// The keywords are unique for each class, as in application code

#include <QString>
#include <vector>

// clang-format off

#define BENCH_SCRIPT_OBJECT( N, S ) \
class BenchScriptFnObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchScriptFnObj##N() \
    { \
        initObject( "BenchScriptObj" #N ).withScripting(); \
        initScriptableField( m_intVal,   "IntVal" #S,   0,         "Int" ); \
        initScriptableField( m_dblVal,   "DblVal" #S,   0.0,       "Double" ); \
        initScriptableField( m_boolVal,  "BoolVal" #S,  false,     "Bool" ); \
        initScriptableField( m_strVal,   "StrVal" #S,   QString(), "String" ).withToolTip( "A tool tip" ); \
        initScriptableField( m_fltVal,   "FltVal" #S,   0.0f,      "Float" ); \
        initScriptableField( m_vecDbl, "VecDbl" #S, "Vector of doubles" ); \
        initScriptableField( m_vecStr, "VecStr" #S, "Vector of strings" ); \
        initScriptableField( m_vecInt, "VecInt" #S, "Vector of ints" ); \
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

BENCH_SCRIPT_OBJECT( 01, Aa ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj01, "BenchScriptFnObj01" );
BENCH_SCRIPT_OBJECT( 02, Ab ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj02, "BenchScriptFnObj02" );
BENCH_SCRIPT_OBJECT( 03, Ac ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj03, "BenchScriptFnObj03" );
BENCH_SCRIPT_OBJECT( 04, Ad ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj04, "BenchScriptFnObj04" );
BENCH_SCRIPT_OBJECT( 05, Ae ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj05, "BenchScriptFnObj05" );
BENCH_SCRIPT_OBJECT( 06, Af ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj06, "BenchScriptFnObj06" );
BENCH_SCRIPT_OBJECT( 07, Ag ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj07, "BenchScriptFnObj07" );
BENCH_SCRIPT_OBJECT( 08, Ah ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj08, "BenchScriptFnObj08" );
BENCH_SCRIPT_OBJECT( 09, Ai ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj09, "BenchScriptFnObj09" );
BENCH_SCRIPT_OBJECT( 10, Aj ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj10, "BenchScriptFnObj10" );
BENCH_SCRIPT_OBJECT( 11, Ak ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj11, "BenchScriptFnObj11" );
BENCH_SCRIPT_OBJECT( 12, Al ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj12, "BenchScriptFnObj12" );
BENCH_SCRIPT_OBJECT( 13, Am ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj13, "BenchScriptFnObj13" );
BENCH_SCRIPT_OBJECT( 14, An ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj14, "BenchScriptFnObj14" );
BENCH_SCRIPT_OBJECT( 15, Ao ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj15, "BenchScriptFnObj15" );
BENCH_SCRIPT_OBJECT( 16, Ap ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj16, "BenchScriptFnObj16" );
BENCH_SCRIPT_OBJECT( 17, Aq ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj17, "BenchScriptFnObj17" );
BENCH_SCRIPT_OBJECT( 18, Ar ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj18, "BenchScriptFnObj18" );
BENCH_SCRIPT_OBJECT( 19, As ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj19, "BenchScriptFnObj19" );
BENCH_SCRIPT_OBJECT( 20, At ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj20, "BenchScriptFnObj20" );

// clang-format on
