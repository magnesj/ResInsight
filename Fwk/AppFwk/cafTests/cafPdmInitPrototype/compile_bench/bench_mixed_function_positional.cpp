// Compile-time benchmark for issue #1202: same objects as cafPdm_CompileBench/bench_mixed_fields.cpp, initialized with
// functions. Keywords are consteval types, default values are positional arguments, as in the macros.

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
enum class BenchMixedPosEnum##N { Alpha, Beta, Gamma, Delta }; \
class BenchMixedPosObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchMixedPosObj##N() \
    { \
        initObject( "BenchMixedObj" #N ); \
        initField( m_intVal,  "IntVal" #N,  0, "Int" ); \
        initField( m_dblVal,  "DblVal" #N,  0.0, "Double" ); \
        initField( m_boolVal, "BoolVal" #N, false, "Bool" ); \
        initField( m_strVal,  "StrVal" #N,  QString(), "String" ); \
        initField( m_enumVal, "EnumVal" #N, BenchMixedPosEnum##N::Alpha, "Enum" ); \
        initField( m_vecDbl,  "VecDbl" #N,  "Vector of doubles" ); \
        initField( m_vecStr,  "VecStr" #N,  "Vector of strings" ); \
        initField( m_optDbl,  "OptDbl" #N,  "Optional double" ); \
        initField( m_fileA,   "FileA" #N,   "File A" ); \
        auto pairInit = std::make_pair( false, 0.0 ); \
        initField( m_pairVal, "PairVal" #N, pairInit, "Pair" ); \
    } \
    caf::PdmField<int>                                m_intVal; \
    caf::PdmField<double>                             m_dblVal; \
    caf::PdmField<bool>                               m_boolVal; \
    caf::PdmField<QString>                            m_strVal; \
    caf::PdmField<caf::AppEnum<BenchMixedPosEnum##N>>  m_enumVal; \
    caf::PdmField<std::vector<double>>                m_vecDbl; \
    caf::PdmField<std::vector<QString>>               m_vecStr; \
    caf::PdmField<std::optional<double>>              m_optDbl; \
    caf::PdmField<caf::FilePath>                      m_fileA; \
    caf::PdmField<std::pair<bool, double>>            m_pairVal; \
};

BENCH_MIXED_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj01, "BenchMixedPosObj01" );
BENCH_MIXED_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj02, "BenchMixedPosObj02" );
BENCH_MIXED_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj03, "BenchMixedPosObj03" );
BENCH_MIXED_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj04, "BenchMixedPosObj04" );
BENCH_MIXED_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj05, "BenchMixedPosObj05" );
BENCH_MIXED_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj06, "BenchMixedPosObj06" );
BENCH_MIXED_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj07, "BenchMixedPosObj07" );
BENCH_MIXED_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj08, "BenchMixedPosObj08" );
BENCH_MIXED_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj09, "BenchMixedPosObj09" );
BENCH_MIXED_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj10, "BenchMixedPosObj10" );
BENCH_MIXED_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj11, "BenchMixedPosObj11" );
BENCH_MIXED_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj12, "BenchMixedPosObj12" );
BENCH_MIXED_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj13, "BenchMixedPosObj13" );
BENCH_MIXED_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj14, "BenchMixedPosObj14" );
BENCH_MIXED_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj15, "BenchMixedPosObj15" );
BENCH_MIXED_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj16, "BenchMixedPosObj16" );
BENCH_MIXED_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj17, "BenchMixedPosObj17" );
BENCH_MIXED_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj18, "BenchMixedPosObj18" );
BENCH_MIXED_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj19, "BenchMixedPosObj19" );
BENCH_MIXED_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchMixedPosObj20, "BenchMixedPosObj20" );

// clang-format on
