// Compile-time benchmark: heavy version of bench_mixed_fields with 50 classes (instead of 20)
// to push compile time into the ~10s range. Used to amplify any compile-time signal between the
// CAF_PDM_Init* macros and the macro-free template API beyond run-to-run noise.

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <optional>
#include <vector>

// clang-format off

#define BENCH_HEAVY_OBJECT( N ) \
enum class BenchHeavyEnum##N { Alpha, Beta, Gamma, Delta }; \
class BenchHeavyObj##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchHeavyObj##N() \
    { \
        CAF_PDM_InitObject( "BenchHeavyObj" #N ); \
        CAF_PDM_InitField( &m_intVal,    "IntVal",    0,                           "Int" ); \
        CAF_PDM_InitField( &m_dblVal,    "DblVal",    0.0,                         "Double" ); \
        CAF_PDM_InitField( &m_boolVal,   "BoolVal",   false,                       "Bool" ); \
        CAF_PDM_InitField( &m_strVal,    "StrVal",    QString(),                   "String" ); \
        CAF_PDM_InitField( &m_enumVal,   "EnumVal",   BenchHeavyEnum##N::Alpha,    "Enum" ); \
        CAF_PDM_InitFieldNoDefault( &m_vecDbl,    "VecDbl",    "Vector of doubles" ); \
        CAF_PDM_InitFieldNoDefault( &m_vecStr,    "VecStr",    "Vector of strings" ); \
        CAF_PDM_InitFieldNoDefault( &m_optDbl,    "OptDbl",    "Optional double" ); \
        CAF_PDM_InitFieldNoDefault( &m_fileA,     "FileA",     "File A" ); \
        auto pairInit = std::make_pair( false, 0.0 ); \
        CAF_PDM_InitField( &m_pairVal, "PairVal", pairInit, "Pair" ); \
    } \
    caf::PdmField<int>                              m_intVal; \
    caf::PdmField<double>                           m_dblVal; \
    caf::PdmField<bool>                             m_boolVal; \
    caf::PdmField<QString>                          m_strVal; \
    caf::PdmField<caf::AppEnum<BenchHeavyEnum##N>>  m_enumVal; \
    caf::PdmField<std::vector<double>>              m_vecDbl; \
    caf::PdmField<std::vector<QString>>             m_vecStr; \
    caf::PdmField<std::optional<double>>            m_optDbl; \
    caf::PdmField<caf::FilePath>                    m_fileA; \
    caf::PdmField<std::pair<bool, double>>          m_pairVal; \
};

BENCH_HEAVY_OBJECT( 01 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj01, "BenchHeavyObj01" );
BENCH_HEAVY_OBJECT( 02 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj02, "BenchHeavyObj02" );
BENCH_HEAVY_OBJECT( 03 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj03, "BenchHeavyObj03" );
BENCH_HEAVY_OBJECT( 04 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj04, "BenchHeavyObj04" );
BENCH_HEAVY_OBJECT( 05 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj05, "BenchHeavyObj05" );
BENCH_HEAVY_OBJECT( 06 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj06, "BenchHeavyObj06" );
BENCH_HEAVY_OBJECT( 07 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj07, "BenchHeavyObj07" );
BENCH_HEAVY_OBJECT( 08 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj08, "BenchHeavyObj08" );
BENCH_HEAVY_OBJECT( 09 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj09, "BenchHeavyObj09" );
BENCH_HEAVY_OBJECT( 10 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj10, "BenchHeavyObj10" );
BENCH_HEAVY_OBJECT( 11 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj11, "BenchHeavyObj11" );
BENCH_HEAVY_OBJECT( 12 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj12, "BenchHeavyObj12" );
BENCH_HEAVY_OBJECT( 13 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj13, "BenchHeavyObj13" );
BENCH_HEAVY_OBJECT( 14 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj14, "BenchHeavyObj14" );
BENCH_HEAVY_OBJECT( 15 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj15, "BenchHeavyObj15" );
BENCH_HEAVY_OBJECT( 16 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj16, "BenchHeavyObj16" );
BENCH_HEAVY_OBJECT( 17 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj17, "BenchHeavyObj17" );
BENCH_HEAVY_OBJECT( 18 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj18, "BenchHeavyObj18" );
BENCH_HEAVY_OBJECT( 19 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj19, "BenchHeavyObj19" );
BENCH_HEAVY_OBJECT( 20 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj20, "BenchHeavyObj20" );
BENCH_HEAVY_OBJECT( 21 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj21, "BenchHeavyObj21" );
BENCH_HEAVY_OBJECT( 22 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj22, "BenchHeavyObj22" );
BENCH_HEAVY_OBJECT( 23 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj23, "BenchHeavyObj23" );
BENCH_HEAVY_OBJECT( 24 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj24, "BenchHeavyObj24" );
BENCH_HEAVY_OBJECT( 25 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj25, "BenchHeavyObj25" );
BENCH_HEAVY_OBJECT( 26 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj26, "BenchHeavyObj26" );
BENCH_HEAVY_OBJECT( 27 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj27, "BenchHeavyObj27" );
BENCH_HEAVY_OBJECT( 28 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj28, "BenchHeavyObj28" );
BENCH_HEAVY_OBJECT( 29 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj29, "BenchHeavyObj29" );
BENCH_HEAVY_OBJECT( 30 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj30, "BenchHeavyObj30" );
BENCH_HEAVY_OBJECT( 31 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj31, "BenchHeavyObj31" );
BENCH_HEAVY_OBJECT( 32 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj32, "BenchHeavyObj32" );
BENCH_HEAVY_OBJECT( 33 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj33, "BenchHeavyObj33" );
BENCH_HEAVY_OBJECT( 34 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj34, "BenchHeavyObj34" );
BENCH_HEAVY_OBJECT( 35 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj35, "BenchHeavyObj35" );
BENCH_HEAVY_OBJECT( 36 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj36, "BenchHeavyObj36" );
BENCH_HEAVY_OBJECT( 37 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj37, "BenchHeavyObj37" );
BENCH_HEAVY_OBJECT( 38 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj38, "BenchHeavyObj38" );
BENCH_HEAVY_OBJECT( 39 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj39, "BenchHeavyObj39" );
BENCH_HEAVY_OBJECT( 40 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj40, "BenchHeavyObj40" );
BENCH_HEAVY_OBJECT( 41 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj41, "BenchHeavyObj41" );
BENCH_HEAVY_OBJECT( 42 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj42, "BenchHeavyObj42" );
BENCH_HEAVY_OBJECT( 43 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj43, "BenchHeavyObj43" );
BENCH_HEAVY_OBJECT( 44 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj44, "BenchHeavyObj44" );
BENCH_HEAVY_OBJECT( 45 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj45, "BenchHeavyObj45" );
BENCH_HEAVY_OBJECT( 46 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj46, "BenchHeavyObj46" );
BENCH_HEAVY_OBJECT( 47 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj47, "BenchHeavyObj47" );
BENCH_HEAVY_OBJECT( 48 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj48, "BenchHeavyObj48" );
BENCH_HEAVY_OBJECT( 49 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj49, "BenchHeavyObj49" );
BENCH_HEAVY_OBJECT( 50 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj50, "BenchHeavyObj50" );
BENCH_HEAVY_OBJECT( 51 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj51, "BenchHeavyObj51" );
BENCH_HEAVY_OBJECT( 52 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj52, "BenchHeavyObj52" );
BENCH_HEAVY_OBJECT( 53 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj53, "BenchHeavyObj53" );
BENCH_HEAVY_OBJECT( 54 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj54, "BenchHeavyObj54" );
BENCH_HEAVY_OBJECT( 55 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj55, "BenchHeavyObj55" );
BENCH_HEAVY_OBJECT( 56 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj56, "BenchHeavyObj56" );
BENCH_HEAVY_OBJECT( 57 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj57, "BenchHeavyObj57" );
BENCH_HEAVY_OBJECT( 58 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj58, "BenchHeavyObj58" );
BENCH_HEAVY_OBJECT( 59 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj59, "BenchHeavyObj59" );
BENCH_HEAVY_OBJECT( 60 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj60, "BenchHeavyObj60" );
BENCH_HEAVY_OBJECT( 61 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj61, "BenchHeavyObj61" );
BENCH_HEAVY_OBJECT( 62 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj62, "BenchHeavyObj62" );
BENCH_HEAVY_OBJECT( 63 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj63, "BenchHeavyObj63" );
BENCH_HEAVY_OBJECT( 64 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj64, "BenchHeavyObj64" );
BENCH_HEAVY_OBJECT( 65 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj65, "BenchHeavyObj65" );
BENCH_HEAVY_OBJECT( 66 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj66, "BenchHeavyObj66" );
BENCH_HEAVY_OBJECT( 67 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj67, "BenchHeavyObj67" );
BENCH_HEAVY_OBJECT( 68 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj68, "BenchHeavyObj68" );
BENCH_HEAVY_OBJECT( 69 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj69, "BenchHeavyObj69" );
BENCH_HEAVY_OBJECT( 70 ) CAF_PDM_SOURCE_INIT( BenchHeavyObj70, "BenchHeavyObj70" );

// clang-format on
