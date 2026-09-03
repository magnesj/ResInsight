// Compile-time benchmark: same as bench_enum_fields.cpp, but uses the macro-free
// initPdmObject/initField template API (raw-enum overload).

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

// clang-format off

#define BENCH_ENUM_OBJECT_T( N ) \
enum class BenchEnumT##N { ValueA, ValueB, ValueC, ValueD }; \
class BenchEnumObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchEnumObjT##N() \
    { \
        initPdmObject<BenchEnumObjT##N>( "BenchEnumObjT" #N ); \
        initField<BenchEnumObjT##N, caf::PdmKeyword{ "EnumA" }>( &m_enumA, BenchEnumT##N::ValueA, "Enum A" ); \
        initField<BenchEnumObjT##N, caf::PdmKeyword{ "EnumB" }>( &m_enumB, BenchEnumT##N::ValueB, "Enum B" ); \
        initField<BenchEnumObjT##N, caf::PdmKeyword{ "EnumC" }>( &m_enumC, BenchEnumT##N::ValueC, "Enum C" ); \
    } \
    caf::PdmField<caf::AppEnum<BenchEnumT##N>> m_enumA; \
    caf::PdmField<caf::AppEnum<BenchEnumT##N>> m_enumB; \
    caf::PdmField<caf::AppEnum<BenchEnumT##N>> m_enumC; \
};

BENCH_ENUM_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT01, "BenchEnumObjT01" );
BENCH_ENUM_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT02, "BenchEnumObjT02" );
BENCH_ENUM_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT03, "BenchEnumObjT03" );
BENCH_ENUM_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT04, "BenchEnumObjT04" );
BENCH_ENUM_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT05, "BenchEnumObjT05" );
BENCH_ENUM_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT06, "BenchEnumObjT06" );
BENCH_ENUM_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT07, "BenchEnumObjT07" );
BENCH_ENUM_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT08, "BenchEnumObjT08" );
BENCH_ENUM_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT09, "BenchEnumObjT09" );
BENCH_ENUM_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT10, "BenchEnumObjT10" );
BENCH_ENUM_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT11, "BenchEnumObjT11" );
BENCH_ENUM_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT12, "BenchEnumObjT12" );
BENCH_ENUM_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT13, "BenchEnumObjT13" );
BENCH_ENUM_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT14, "BenchEnumObjT14" );
BENCH_ENUM_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT15, "BenchEnumObjT15" );
BENCH_ENUM_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT16, "BenchEnumObjT16" );
BENCH_ENUM_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT17, "BenchEnumObjT17" );
BENCH_ENUM_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT18, "BenchEnumObjT18" );
BENCH_ENUM_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT19, "BenchEnumObjT19" );
BENCH_ENUM_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchEnumObjT20, "BenchEnumObjT20" );

// clang-format on
