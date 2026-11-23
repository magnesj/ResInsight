// Compile-time benchmark for issue #1202: same objects as cafPdm_CompileBench/bench_mixed_fields.cpp, initialized with
// functions. Keywords are consteval types, default values are set using withDefault().

#include "cafProtoFieldInit.h"

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

// The keywords are unique for each class, as in application code

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_MIXED_OBJECT( N ) \
enum class BenchMixedFnEnum##N { Alpha, Beta, Gamma, Delta }; \
class BenchMixedFnObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchMixedFnObj##N() \
    { \
        initObject( "BenchMixedObj" #N ); \
        initField( m_intVal,  "IntVal" #N,  "Int" ).withDefault( 0 ); \
        initField( m_dblVal,  "DblVal" #N,  "Double" ).withDefault( 0.0 ); \
        initField( m_boolVal, "BoolVal" #N, "Bool" ).withDefault( false ); \
        initField( m_strVal,  "StrVal" #N,  "String" ).withDefault( QString() ); \
        initField( m_enumVal, "EnumVal" #N, "Enum" ).withDefault( BenchMixedFnEnum##N::Alpha ); \
        initField( m_vecDbl,  "VecDbl" #N,  "Vector of doubles" ); \
        initField( m_vecStr,  "VecStr" #N,  "Vector of strings" ); \
        initField( m_optDbl,  "OptDbl" #N,  "Optional double" ); \
        initField( m_fileA,   "FileA" #N,   "File A" ); \
        auto pairInit = std::make_pair( false, 0.0 ); \
        initField( m_pairVal, "PairVal" #N, "Pair" ).withDefault( pairInit ); \
    } \
    caf::PdmField<int>                                m_intVal; \
    caf::PdmField<double>                             m_dblVal; \
    caf::PdmField<bool>                               m_boolVal; \
    caf::PdmField<QString>                            m_strVal; \
    caf::PdmField<caf::AppEnum<BenchMixedFnEnum##N>>  m_enumVal; \
    caf::PdmField<std::vector<double>>                m_vecDbl; \
    caf::PdmField<std::vector<QString>>               m_vecStr; \
    caf::PdmField<std::optional<double>>              m_optDbl; \
    caf::PdmField<caf::FilePath>                      m_fileA; \
    caf::PdmField<std::pair<bool, double>>            m_pairVal; \
};

BENCH_MIXED_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj01, "BenchMixedFnObj01" );
BENCH_MIXED_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj02, "BenchMixedFnObj02" );
BENCH_MIXED_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj03, "BenchMixedFnObj03" );
BENCH_MIXED_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj04, "BenchMixedFnObj04" );
BENCH_MIXED_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj05, "BenchMixedFnObj05" );
BENCH_MIXED_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj06, "BenchMixedFnObj06" );
BENCH_MIXED_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj07, "BenchMixedFnObj07" );
BENCH_MIXED_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj08, "BenchMixedFnObj08" );
BENCH_MIXED_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj09, "BenchMixedFnObj09" );
BENCH_MIXED_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj10, "BenchMixedFnObj10" );
BENCH_MIXED_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj11, "BenchMixedFnObj11" );
BENCH_MIXED_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj12, "BenchMixedFnObj12" );
BENCH_MIXED_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj13, "BenchMixedFnObj13" );
BENCH_MIXED_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj14, "BenchMixedFnObj14" );
BENCH_MIXED_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj15, "BenchMixedFnObj15" );
BENCH_MIXED_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj16, "BenchMixedFnObj16" );
BENCH_MIXED_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj17, "BenchMixedFnObj17" );
BENCH_MIXED_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj18, "BenchMixedFnObj18" );
BENCH_MIXED_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj19, "BenchMixedFnObj19" );
BENCH_MIXED_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchMixedFnObj20, "BenchMixedFnObj20" );

// clang-format on
