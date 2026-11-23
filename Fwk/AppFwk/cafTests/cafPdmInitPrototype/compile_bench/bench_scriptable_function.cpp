// Compile-time benchmark for issue #1202: same objects as bench_scriptable_macro.cpp, initialized with functions.

#include "cafProtoScripting.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <vector>

// clang-format off

#define BENCH_SCRIPT_OBJECT( N ) \
class BenchScriptFnObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchScriptFnObj##N() \
    { \
        initObject( "BenchScriptObj" #N ).withScripting(); \
        initScriptableField( m_intVal,   "IntVal",   0,         "Int" ); \
        initScriptableField( m_dblVal,   "DblVal",   0.0,       "Double" ); \
        initScriptableField( m_boolVal,  "BoolVal",  false,     "Bool" ); \
        initScriptableField( m_strVal,   "StrVal",   QString(), "String" ).withToolTip( "A tool tip" ); \
        initScriptableField( m_fltVal,   "FltVal",   0.0f,      "Float" ); \
        initScriptableField( m_vecDbl, "VecDbl", "Vector of doubles" ); \
        initScriptableField( m_vecStr, "VecStr", "Vector of strings" ); \
        initScriptableField( m_vecInt, "VecInt", "Vector of ints" ); \
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

BENCH_SCRIPT_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj01, "BenchScriptFnObj01" );
BENCH_SCRIPT_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj02, "BenchScriptFnObj02" );
BENCH_SCRIPT_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj03, "BenchScriptFnObj03" );
BENCH_SCRIPT_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj04, "BenchScriptFnObj04" );
BENCH_SCRIPT_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj05, "BenchScriptFnObj05" );
BENCH_SCRIPT_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj06, "BenchScriptFnObj06" );
BENCH_SCRIPT_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj07, "BenchScriptFnObj07" );
BENCH_SCRIPT_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj08, "BenchScriptFnObj08" );
BENCH_SCRIPT_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj09, "BenchScriptFnObj09" );
BENCH_SCRIPT_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj10, "BenchScriptFnObj10" );
BENCH_SCRIPT_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj11, "BenchScriptFnObj11" );
BENCH_SCRIPT_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj12, "BenchScriptFnObj12" );
BENCH_SCRIPT_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj13, "BenchScriptFnObj13" );
BENCH_SCRIPT_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj14, "BenchScriptFnObj14" );
BENCH_SCRIPT_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj15, "BenchScriptFnObj15" );
BENCH_SCRIPT_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj16, "BenchScriptFnObj16" );
BENCH_SCRIPT_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj17, "BenchScriptFnObj17" );
BENCH_SCRIPT_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj18, "BenchScriptFnObj18" );
BENCH_SCRIPT_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj19, "BenchScriptFnObj19" );
BENCH_SCRIPT_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchScriptFnObj20, "BenchScriptFnObj20" );

// clang-format on
