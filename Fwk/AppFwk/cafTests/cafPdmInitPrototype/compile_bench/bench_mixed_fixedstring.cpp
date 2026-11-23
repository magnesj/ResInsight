// Compile-time benchmark for issue #1202: same objects as cafPdm_CompileBench/bench_mixed_fields.cpp, initialized with
// functions. Keywords are fixed string template arguments, default values are set using withDefault().

#include "cafProtoFieldInit.h"

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_MIXED_OBJECT( N ) \
enum class BenchMixedFsEnum##N { Alpha, Beta, Gamma, Delta }; \
class BenchMixedFsObj##N : public cafproto::PdmObjectProto \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchMixedFsObj##N() \
    { \
        initObject( "BenchMixedObj" #N ); \
        initFieldN<"IntVal">( m_intVal,    "Int" ).withDefault( 0 ); \
        initFieldN<"DblVal">( m_dblVal,    "Double" ).withDefault( 0.0 ); \
        initFieldN<"BoolVal">( m_boolVal,  "Bool" ).withDefault( false ); \
        initFieldN<"StrVal">( m_strVal,    "String" ).withDefault( QString() ); \
        initFieldN<"EnumVal">( m_enumVal,  "Enum" ).withDefault( BenchMixedFsEnum##N::Alpha ); \
        initFieldN<"VecDbl">( m_vecDbl,    "Vector of doubles" ); \
        initFieldN<"VecStr">( m_vecStr,    "Vector of strings" ); \
        initFieldN<"OptDbl">( m_optDbl,    "Optional double" ); \
        initFieldN<"FileA">( m_fileA,      "File A" ); \
        auto pairInit = std::make_pair( false, 0.0 ); \
        initFieldN<"PairVal">( m_pairVal,  "Pair" ).withDefault( pairInit ); \
    } \
    caf::PdmField<int>                                m_intVal; \
    caf::PdmField<double>                             m_dblVal; \
    caf::PdmField<bool>                               m_boolVal; \
    caf::PdmField<QString>                            m_strVal; \
    caf::PdmField<caf::AppEnum<BenchMixedFsEnum##N>>  m_enumVal; \
    caf::PdmField<std::vector<double>>                m_vecDbl; \
    caf::PdmField<std::vector<QString>>               m_vecStr; \
    caf::PdmField<std::optional<double>>              m_optDbl; \
    caf::PdmField<caf::FilePath>                      m_fileA; \
    caf::PdmField<std::pair<bool, double>>            m_pairVal; \
};

BENCH_MIXED_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj01, "BenchMixedFsObj01" );
BENCH_MIXED_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj02, "BenchMixedFsObj02" );
BENCH_MIXED_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj03, "BenchMixedFsObj03" );
BENCH_MIXED_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj04, "BenchMixedFsObj04" );
BENCH_MIXED_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj05, "BenchMixedFsObj05" );
BENCH_MIXED_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj06, "BenchMixedFsObj06" );
BENCH_MIXED_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj07, "BenchMixedFsObj07" );
BENCH_MIXED_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj08, "BenchMixedFsObj08" );
BENCH_MIXED_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj09, "BenchMixedFsObj09" );
BENCH_MIXED_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj10, "BenchMixedFsObj10" );
BENCH_MIXED_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj11, "BenchMixedFsObj11" );
BENCH_MIXED_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj12, "BenchMixedFsObj12" );
BENCH_MIXED_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj13, "BenchMixedFsObj13" );
BENCH_MIXED_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj14, "BenchMixedFsObj14" );
BENCH_MIXED_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj15, "BenchMixedFsObj15" );
BENCH_MIXED_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj16, "BenchMixedFsObj16" );
BENCH_MIXED_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj17, "BenchMixedFsObj17" );
BENCH_MIXED_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj18, "BenchMixedFsObj18" );
BENCH_MIXED_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj19, "BenchMixedFsObj19" );
BENCH_MIXED_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchMixedFsObj20, "BenchMixedFsObj20" );

// clang-format on
