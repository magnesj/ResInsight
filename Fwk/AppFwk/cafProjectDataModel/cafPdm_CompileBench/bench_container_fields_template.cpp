// Compile-time benchmark: same as bench_container_fields.cpp, but uses the macro-free
// initPdmObject/initField/initFieldNoDefault template API.

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_CONTAINER_OBJECT_T( N ) \
class BenchContainerObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchContainerObjT##N() \
    { \
        initPdmObject<BenchContainerObjT##N>( "BenchContainerObjT" #N ); \
        initFieldNoDefault<BenchContainerObjT##N, caf::PdmKeyword{ "VecDouble" }>( &m_vecDouble, "Vector of doubles" ); \
        initFieldNoDefault<BenchContainerObjT##N, caf::PdmKeyword{ "VecInt"    }>( &m_vecInt,    "Vector of ints" ); \
        initFieldNoDefault<BenchContainerObjT##N, caf::PdmKeyword{ "VecString" }>( &m_vecString, "Vector of strings" ); \
        initFieldNoDefault<BenchContainerObjT##N, caf::PdmKeyword{ "OptDouble" }>( &m_optDouble, "Optional double" ); \
        initFieldNoDefault<BenchContainerObjT##N, caf::PdmKeyword{ "OptString" }>( &m_optString, "Optional string" ); \
        initField<BenchContainerObjT##N, caf::PdmKeyword{ "PairBoolDbl" }>( &m_pairBoolDbl, std::make_pair( false, 0.0 ),       "Pair bool/double" ); \
        initField<BenchContainerObjT##N, caf::PdmKeyword{ "PairBoolStr" }>( &m_pairBoolStr, std::make_pair( false, QString() ), "Pair bool/string" ); \
    } \
    caf::PdmField<std::vector<double>>          m_vecDouble; \
    caf::PdmField<std::vector<int>>             m_vecInt; \
    caf::PdmField<std::vector<QString>>         m_vecString; \
    caf::PdmField<std::optional<double>>        m_optDouble; \
    caf::PdmField<std::optional<QString>>       m_optString; \
    caf::PdmField<std::pair<bool, double>>      m_pairBoolDbl; \
    caf::PdmField<std::pair<bool, QString>>     m_pairBoolStr; \
};

BENCH_CONTAINER_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT01, "BenchContainerObjT01" );
BENCH_CONTAINER_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT02, "BenchContainerObjT02" );
BENCH_CONTAINER_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT03, "BenchContainerObjT03" );
BENCH_CONTAINER_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT04, "BenchContainerObjT04" );
BENCH_CONTAINER_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT05, "BenchContainerObjT05" );
BENCH_CONTAINER_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT06, "BenchContainerObjT06" );
BENCH_CONTAINER_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT07, "BenchContainerObjT07" );
BENCH_CONTAINER_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT08, "BenchContainerObjT08" );
BENCH_CONTAINER_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT09, "BenchContainerObjT09" );
BENCH_CONTAINER_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT10, "BenchContainerObjT10" );
BENCH_CONTAINER_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT11, "BenchContainerObjT11" );
BENCH_CONTAINER_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT12, "BenchContainerObjT12" );
BENCH_CONTAINER_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT13, "BenchContainerObjT13" );
BENCH_CONTAINER_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT14, "BenchContainerObjT14" );
BENCH_CONTAINER_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT15, "BenchContainerObjT15" );
BENCH_CONTAINER_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT16, "BenchContainerObjT16" );
BENCH_CONTAINER_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT17, "BenchContainerObjT17" );
BENCH_CONTAINER_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT18, "BenchContainerObjT18" );
BENCH_CONTAINER_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT19, "BenchContainerObjT19" );
BENCH_CONTAINER_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchContainerObjT20, "BenchContainerObjT20" );

// clang-format on
