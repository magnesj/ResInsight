// Compile-time benchmark: same as bench_mixed_fields.cpp, but uses the macro-free
// initPdmObject/initField/initFieldNoDefault template API.

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_MIXED_OBJECT_T( N ) \
enum class BenchMixedEnumT##N { Alpha, Beta, Gamma, Delta }; \
class BenchMixedObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchMixedObjT##N() \
    { \
        initPdmObject<BenchMixedObjT##N>( "BenchMixedObjT" #N ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "IntVal"  }>( &m_intVal,  0,                          "Int" ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "DblVal"  }>( &m_dblVal,  0.0,                        "Double" ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "BoolVal" }>( &m_boolVal, false,                      "Bool" ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "StrVal"  }>( &m_strVal,  QString(),                  "String" ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "EnumVal" }>( &m_enumVal, BenchMixedEnumT##N::Alpha,  "Enum" ); \
        initFieldNoDefault<BenchMixedObjT##N, caf::PdmKeyword{ "VecDbl" }>( &m_vecDbl, "Vector of doubles" ); \
        initFieldNoDefault<BenchMixedObjT##N, caf::PdmKeyword{ "VecStr" }>( &m_vecStr, "Vector of strings" ); \
        initFieldNoDefault<BenchMixedObjT##N, caf::PdmKeyword{ "OptDbl" }>( &m_optDbl, "Optional double" ); \
        initFieldNoDefault<BenchMixedObjT##N, caf::PdmKeyword{ "FileA"  }>( &m_fileA,  "File A" ); \
        initField<BenchMixedObjT##N, caf::PdmKeyword{ "PairVal" }>( &m_pairVal, std::make_pair( false, 0.0 ), "Pair" ); \
    } \
    caf::PdmField<int>                              m_intVal; \
    caf::PdmField<double>                           m_dblVal; \
    caf::PdmField<bool>                             m_boolVal; \
    caf::PdmField<QString>                          m_strVal; \
    caf::PdmField<caf::AppEnum<BenchMixedEnumT##N>> m_enumVal; \
    caf::PdmField<std::vector<double>>              m_vecDbl; \
    caf::PdmField<std::vector<QString>>             m_vecStr; \
    caf::PdmField<std::optional<double>>            m_optDbl; \
    caf::PdmField<caf::FilePath>                    m_fileA; \
    caf::PdmField<std::pair<bool, double>>          m_pairVal; \
};

BENCH_MIXED_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT01, "BenchMixedObjT01" );
BENCH_MIXED_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT02, "BenchMixedObjT02" );
BENCH_MIXED_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT03, "BenchMixedObjT03" );
BENCH_MIXED_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT04, "BenchMixedObjT04" );
BENCH_MIXED_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT05, "BenchMixedObjT05" );
BENCH_MIXED_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT06, "BenchMixedObjT06" );
BENCH_MIXED_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT07, "BenchMixedObjT07" );
BENCH_MIXED_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT08, "BenchMixedObjT08" );
BENCH_MIXED_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT09, "BenchMixedObjT09" );
BENCH_MIXED_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT10, "BenchMixedObjT10" );
BENCH_MIXED_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT11, "BenchMixedObjT11" );
BENCH_MIXED_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT12, "BenchMixedObjT12" );
BENCH_MIXED_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT13, "BenchMixedObjT13" );
BENCH_MIXED_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT14, "BenchMixedObjT14" );
BENCH_MIXED_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT15, "BenchMixedObjT15" );
BENCH_MIXED_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT16, "BenchMixedObjT16" );
BENCH_MIXED_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT17, "BenchMixedObjT17" );
BENCH_MIXED_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT18, "BenchMixedObjT18" );
BENCH_MIXED_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT19, "BenchMixedObjT19" );
BENCH_MIXED_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchMixedObjT20, "BenchMixedObjT20" );

// clang-format on
