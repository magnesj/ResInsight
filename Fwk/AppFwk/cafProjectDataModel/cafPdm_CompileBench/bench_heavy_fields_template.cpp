// Compile-time benchmark: same as bench_heavy_fields.cpp (50 classes, 10 fields each), but
// using the macro-free initPdmObject / initField / initFieldNoDefault template API.

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_HEAVY_OBJECT_T( N ) \
enum class BenchHeavyEnumT##N { Alpha, Beta, Gamma, Delta }; \
class BenchHeavyObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchHeavyObjT##N() \
    { \
        initPdmObject<BenchHeavyObjT##N>( "BenchHeavyObjT" #N ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "IntVal"  }>( &m_intVal,  0,                          "Int" ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "DblVal"  }>( &m_dblVal,  0.0,                        "Double" ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "BoolVal" }>( &m_boolVal, false,                      "Bool" ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "StrVal"  }>( &m_strVal,  QString(),                  "String" ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "EnumVal" }>( &m_enumVal, BenchHeavyEnumT##N::Alpha,  "Enum" ); \
        initFieldNoDefault<BenchHeavyObjT##N, caf::PdmKeyword{ "VecDbl" }>( &m_vecDbl, "Vector of doubles" ); \
        initFieldNoDefault<BenchHeavyObjT##N, caf::PdmKeyword{ "VecStr" }>( &m_vecStr, "Vector of strings" ); \
        initFieldNoDefault<BenchHeavyObjT##N, caf::PdmKeyword{ "OptDbl" }>( &m_optDbl, "Optional double" ); \
        initFieldNoDefault<BenchHeavyObjT##N, caf::PdmKeyword{ "FileA"  }>( &m_fileA,  "File A" ); \
        initField<BenchHeavyObjT##N, caf::PdmKeyword{ "PairVal" }>( &m_pairVal, std::make_pair( false, 0.0 ), "Pair" ); \
    } \
    caf::PdmField<int>                              m_intVal; \
    caf::PdmField<double>                           m_dblVal; \
    caf::PdmField<bool>                             m_boolVal; \
    caf::PdmField<QString>                          m_strVal; \
    caf::PdmField<caf::AppEnum<BenchHeavyEnumT##N>> m_enumVal; \
    caf::PdmField<std::vector<double>>              m_vecDbl; \
    caf::PdmField<std::vector<QString>>             m_vecStr; \
    caf::PdmField<std::optional<double>>            m_optDbl; \
    caf::PdmField<caf::FilePath>                    m_fileA; \
    caf::PdmField<std::pair<bool, double>>          m_pairVal; \
};

BENCH_HEAVY_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT01, "BenchHeavyObjT01" );
BENCH_HEAVY_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT02, "BenchHeavyObjT02" );
BENCH_HEAVY_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT03, "BenchHeavyObjT03" );
BENCH_HEAVY_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT04, "BenchHeavyObjT04" );
BENCH_HEAVY_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT05, "BenchHeavyObjT05" );
BENCH_HEAVY_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT06, "BenchHeavyObjT06" );
BENCH_HEAVY_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT07, "BenchHeavyObjT07" );
BENCH_HEAVY_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT08, "BenchHeavyObjT08" );
BENCH_HEAVY_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT09, "BenchHeavyObjT09" );
BENCH_HEAVY_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT10, "BenchHeavyObjT10" );
BENCH_HEAVY_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT11, "BenchHeavyObjT11" );
BENCH_HEAVY_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT12, "BenchHeavyObjT12" );
BENCH_HEAVY_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT13, "BenchHeavyObjT13" );
BENCH_HEAVY_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT14, "BenchHeavyObjT14" );
BENCH_HEAVY_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT15, "BenchHeavyObjT15" );
BENCH_HEAVY_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT16, "BenchHeavyObjT16" );
BENCH_HEAVY_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT17, "BenchHeavyObjT17" );
BENCH_HEAVY_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT18, "BenchHeavyObjT18" );
BENCH_HEAVY_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT19, "BenchHeavyObjT19" );
BENCH_HEAVY_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT20, "BenchHeavyObjT20" );
BENCH_HEAVY_OBJECT_T( 21 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT21, "BenchHeavyObjT21" );
BENCH_HEAVY_OBJECT_T( 22 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT22, "BenchHeavyObjT22" );
BENCH_HEAVY_OBJECT_T( 23 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT23, "BenchHeavyObjT23" );
BENCH_HEAVY_OBJECT_T( 24 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT24, "BenchHeavyObjT24" );
BENCH_HEAVY_OBJECT_T( 25 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT25, "BenchHeavyObjT25" );
BENCH_HEAVY_OBJECT_T( 26 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT26, "BenchHeavyObjT26" );
BENCH_HEAVY_OBJECT_T( 27 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT27, "BenchHeavyObjT27" );
BENCH_HEAVY_OBJECT_T( 28 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT28, "BenchHeavyObjT28" );
BENCH_HEAVY_OBJECT_T( 29 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT29, "BenchHeavyObjT29" );
BENCH_HEAVY_OBJECT_T( 30 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT30, "BenchHeavyObjT30" );
BENCH_HEAVY_OBJECT_T( 31 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT31, "BenchHeavyObjT31" );
BENCH_HEAVY_OBJECT_T( 32 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT32, "BenchHeavyObjT32" );
BENCH_HEAVY_OBJECT_T( 33 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT33, "BenchHeavyObjT33" );
BENCH_HEAVY_OBJECT_T( 34 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT34, "BenchHeavyObjT34" );
BENCH_HEAVY_OBJECT_T( 35 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT35, "BenchHeavyObjT35" );
BENCH_HEAVY_OBJECT_T( 36 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT36, "BenchHeavyObjT36" );
BENCH_HEAVY_OBJECT_T( 37 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT37, "BenchHeavyObjT37" );
BENCH_HEAVY_OBJECT_T( 38 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT38, "BenchHeavyObjT38" );
BENCH_HEAVY_OBJECT_T( 39 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT39, "BenchHeavyObjT39" );
BENCH_HEAVY_OBJECT_T( 40 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT40, "BenchHeavyObjT40" );
BENCH_HEAVY_OBJECT_T( 41 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT41, "BenchHeavyObjT41" );
BENCH_HEAVY_OBJECT_T( 42 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT42, "BenchHeavyObjT42" );
BENCH_HEAVY_OBJECT_T( 43 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT43, "BenchHeavyObjT43" );
BENCH_HEAVY_OBJECT_T( 44 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT44, "BenchHeavyObjT44" );
BENCH_HEAVY_OBJECT_T( 45 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT45, "BenchHeavyObjT45" );
BENCH_HEAVY_OBJECT_T( 46 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT46, "BenchHeavyObjT46" );
BENCH_HEAVY_OBJECT_T( 47 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT47, "BenchHeavyObjT47" );
BENCH_HEAVY_OBJECT_T( 48 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT48, "BenchHeavyObjT48" );
BENCH_HEAVY_OBJECT_T( 49 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT49, "BenchHeavyObjT49" );
BENCH_HEAVY_OBJECT_T( 50 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT50, "BenchHeavyObjT50" );
BENCH_HEAVY_OBJECT_T( 51 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT51, "BenchHeavyObjT51" );
BENCH_HEAVY_OBJECT_T( 52 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT52, "BenchHeavyObjT52" );
BENCH_HEAVY_OBJECT_T( 53 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT53, "BenchHeavyObjT53" );
BENCH_HEAVY_OBJECT_T( 54 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT54, "BenchHeavyObjT54" );
BENCH_HEAVY_OBJECT_T( 55 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT55, "BenchHeavyObjT55" );
BENCH_HEAVY_OBJECT_T( 56 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT56, "BenchHeavyObjT56" );
BENCH_HEAVY_OBJECT_T( 57 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT57, "BenchHeavyObjT57" );
BENCH_HEAVY_OBJECT_T( 58 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT58, "BenchHeavyObjT58" );
BENCH_HEAVY_OBJECT_T( 59 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT59, "BenchHeavyObjT59" );
BENCH_HEAVY_OBJECT_T( 60 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT60, "BenchHeavyObjT60" );
BENCH_HEAVY_OBJECT_T( 61 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT61, "BenchHeavyObjT61" );
BENCH_HEAVY_OBJECT_T( 62 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT62, "BenchHeavyObjT62" );
BENCH_HEAVY_OBJECT_T( 63 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT63, "BenchHeavyObjT63" );
BENCH_HEAVY_OBJECT_T( 64 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT64, "BenchHeavyObjT64" );
BENCH_HEAVY_OBJECT_T( 65 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT65, "BenchHeavyObjT65" );
BENCH_HEAVY_OBJECT_T( 66 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT66, "BenchHeavyObjT66" );
BENCH_HEAVY_OBJECT_T( 67 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT67, "BenchHeavyObjT67" );
BENCH_HEAVY_OBJECT_T( 68 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT68, "BenchHeavyObjT68" );
BENCH_HEAVY_OBJECT_T( 69 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT69, "BenchHeavyObjT69" );
BENCH_HEAVY_OBJECT_T( 70 ) CAF_PDM_SOURCE_INIT( BenchHeavyObjT70, "BenchHeavyObjT70" );

// clang-format on
