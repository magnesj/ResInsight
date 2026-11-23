// Compile-time benchmark for issue #1202: scriptable objects initialized with the macros. Baseline for
// bench_scriptable_function.cpp and bench_scriptable_fixedstring.cpp.

#include "cafPdmField.h"
#include "cafPdmFieldScriptingCapability.h"
#include "cafPdmObject.h"
#include "cafPdmObjectScriptingCapability.h"

#include <QString>
#include <vector>

// clang-format off

#define BENCH_SCRIPT_OBJECT( N ) \
class BenchScriptObj##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchScriptObj##N() \
    { \
        CAF_PDM_InitScriptableObject( "BenchScriptObj" #N ); \
        CAF_PDM_InitScriptableField( &m_intVal,   "IntVal",   0,         "Int" ); \
        CAF_PDM_InitScriptableField( &m_dblVal,   "DblVal",   0.0,       "Double" ); \
        CAF_PDM_InitScriptableField( &m_boolVal,  "BoolVal",  false,     "Bool" ); \
        CAF_PDM_InitScriptableField( &m_strVal,   "StrVal",   QString(), "String", "", "A tool tip" ); \
        CAF_PDM_InitScriptableField( &m_fltVal,   "FltVal",   0.0f,      "Float" ); \
        CAF_PDM_InitScriptableFieldNoDefault( &m_vecDbl, "VecDbl", "Vector of doubles" ); \
        CAF_PDM_InitScriptableFieldNoDefault( &m_vecStr, "VecStr", "Vector of strings" ); \
        CAF_PDM_InitScriptableFieldNoDefault( &m_vecInt, "VecInt", "Vector of ints" ); \
        CAF_PDM_InitScriptableFieldWithScriptKeyword( &m_legacy, "legacy_keyword", "LegacyKeyword", 0, "Legacy" ); \
        CAF_PDM_InitField( &m_plain, "Plain", 0, "Not scriptable" ); \
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

BENCH_SCRIPT_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchScriptObj01, "BenchScriptObj01" );
BENCH_SCRIPT_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchScriptObj02, "BenchScriptObj02" );
BENCH_SCRIPT_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchScriptObj03, "BenchScriptObj03" );
BENCH_SCRIPT_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchScriptObj04, "BenchScriptObj04" );
BENCH_SCRIPT_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchScriptObj05, "BenchScriptObj05" );
BENCH_SCRIPT_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchScriptObj06, "BenchScriptObj06" );
BENCH_SCRIPT_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchScriptObj07, "BenchScriptObj07" );
BENCH_SCRIPT_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchScriptObj08, "BenchScriptObj08" );
BENCH_SCRIPT_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchScriptObj09, "BenchScriptObj09" );
BENCH_SCRIPT_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchScriptObj10, "BenchScriptObj10" );
BENCH_SCRIPT_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchScriptObj11, "BenchScriptObj11" );
BENCH_SCRIPT_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchScriptObj12, "BenchScriptObj12" );
BENCH_SCRIPT_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchScriptObj13, "BenchScriptObj13" );
BENCH_SCRIPT_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchScriptObj14, "BenchScriptObj14" );
BENCH_SCRIPT_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchScriptObj15, "BenchScriptObj15" );
BENCH_SCRIPT_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchScriptObj16, "BenchScriptObj16" );
BENCH_SCRIPT_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchScriptObj17, "BenchScriptObj17" );
BENCH_SCRIPT_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchScriptObj18, "BenchScriptObj18" );
BENCH_SCRIPT_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchScriptObj19, "BenchScriptObj19" );
BENCH_SCRIPT_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchScriptObj20, "BenchScriptObj20" );

// clang-format on
