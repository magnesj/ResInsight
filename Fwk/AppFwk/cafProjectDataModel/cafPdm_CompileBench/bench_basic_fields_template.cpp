// Compile-time benchmark: same as bench_basic_fields.cpp, but uses the macro-free
// initPdmObject/initField template API. Used to measure the compile-time delta of the
// new API versus the CAF_PDM_InitObject/CAF_PDM_InitField macros.

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>

// clang-format off

#define BENCH_BASIC_OBJECT_T( N ) \
class BenchBasicObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchBasicObjT##N() \
    { \
        initPdmObject<BenchBasicObjT##N>( "BenchBasicObjT" #N ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "IntA"  }>( &m_intA,  0,         "Int A" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "IntB"  }>( &m_intB,  0,         "Int B" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "DblA"  }>( &m_dblA,  0.0,       "Double A" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "DblB"  }>( &m_dblB,  0.0,       "Double B" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "BoolA" }>( &m_boolA, false,     "Bool A" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "BoolB" }>( &m_boolB, false,     "Bool B" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "StrA"  }>( &m_strA,  QString(), "String A" ); \
        initField<BenchBasicObjT##N, caf::PdmKeyword{ "StrB"  }>( &m_strB,  QString(), "String B" ); \
    } \
    caf::PdmField<int>     m_intA; \
    caf::PdmField<int>     m_intB; \
    caf::PdmField<double>  m_dblA; \
    caf::PdmField<double>  m_dblB; \
    caf::PdmField<bool>    m_boolA; \
    caf::PdmField<bool>    m_boolB; \
    caf::PdmField<QString> m_strA; \
    caf::PdmField<QString> m_strB; \
};

BENCH_BASIC_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT01, "BenchBasicObjT01" );
BENCH_BASIC_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT02, "BenchBasicObjT02" );
BENCH_BASIC_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT03, "BenchBasicObjT03" );
BENCH_BASIC_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT04, "BenchBasicObjT04" );
BENCH_BASIC_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT05, "BenchBasicObjT05" );
BENCH_BASIC_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT06, "BenchBasicObjT06" );
BENCH_BASIC_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT07, "BenchBasicObjT07" );
BENCH_BASIC_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT08, "BenchBasicObjT08" );
BENCH_BASIC_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT09, "BenchBasicObjT09" );
BENCH_BASIC_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT10, "BenchBasicObjT10" );
BENCH_BASIC_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT11, "BenchBasicObjT11" );
BENCH_BASIC_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT12, "BenchBasicObjT12" );
BENCH_BASIC_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT13, "BenchBasicObjT13" );
BENCH_BASIC_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT14, "BenchBasicObjT14" );
BENCH_BASIC_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT15, "BenchBasicObjT15" );
BENCH_BASIC_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT16, "BenchBasicObjT16" );
BENCH_BASIC_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT17, "BenchBasicObjT17" );
BENCH_BASIC_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT18, "BenchBasicObjT18" );
BENCH_BASIC_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT19, "BenchBasicObjT19" );
BENCH_BASIC_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchBasicObjT20, "BenchBasicObjT20" );

// clang-format on
