// Compile-time benchmark: same as bench_filepath_fields.cpp, but uses the macro-free
// initPdmObject/initFieldNoDefault template API.

#include "cafFilePath.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <vector>

// clang-format off

#define BENCH_FILEPATH_OBJECT_T( N ) \
class BenchFilePathObjT##N : public caf::PdmObject \
{ \
    CAF_PDM_HEADER_INIT; \
public: \
    BenchFilePathObjT##N() \
    { \
        initPdmObject<BenchFilePathObjT##N>( "BenchFilePathObjT" #N ); \
        initFieldNoDefault<BenchFilePathObjT##N, caf::PdmKeyword{ "FileA" }>( &m_fileA, "File A" ); \
        initFieldNoDefault<BenchFilePathObjT##N, caf::PdmKeyword{ "FileB" }>( &m_fileB, "File B" ); \
        initFieldNoDefault<BenchFilePathObjT##N, caf::PdmKeyword{ "FileC" }>( &m_fileC, "File C" ); \
        initFieldNoDefault<BenchFilePathObjT##N, caf::PdmKeyword{ "Files" }>( &m_files, "File list" ); \
    } \
    caf::PdmField<caf::FilePath>              m_fileA; \
    caf::PdmField<caf::FilePath>              m_fileB; \
    caf::PdmField<caf::FilePath>              m_fileC; \
    caf::PdmField<std::vector<caf::FilePath>> m_files; \
};

BENCH_FILEPATH_OBJECT_T( 01 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT01, "BenchFilePathObjT01" );
BENCH_FILEPATH_OBJECT_T( 02 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT02, "BenchFilePathObjT02" );
BENCH_FILEPATH_OBJECT_T( 03 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT03, "BenchFilePathObjT03" );
BENCH_FILEPATH_OBJECT_T( 04 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT04, "BenchFilePathObjT04" );
BENCH_FILEPATH_OBJECT_T( 05 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT05, "BenchFilePathObjT05" );
BENCH_FILEPATH_OBJECT_T( 06 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT06, "BenchFilePathObjT06" );
BENCH_FILEPATH_OBJECT_T( 07 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT07, "BenchFilePathObjT07" );
BENCH_FILEPATH_OBJECT_T( 08 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT08, "BenchFilePathObjT08" );
BENCH_FILEPATH_OBJECT_T( 09 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT09, "BenchFilePathObjT09" );
BENCH_FILEPATH_OBJECT_T( 10 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT10, "BenchFilePathObjT10" );
BENCH_FILEPATH_OBJECT_T( 11 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT11, "BenchFilePathObjT11" );
BENCH_FILEPATH_OBJECT_T( 12 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT12, "BenchFilePathObjT12" );
BENCH_FILEPATH_OBJECT_T( 13 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT13, "BenchFilePathObjT13" );
BENCH_FILEPATH_OBJECT_T( 14 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT14, "BenchFilePathObjT14" );
BENCH_FILEPATH_OBJECT_T( 15 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT15, "BenchFilePathObjT15" );
BENCH_FILEPATH_OBJECT_T( 16 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT16, "BenchFilePathObjT16" );
BENCH_FILEPATH_OBJECT_T( 17 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT17, "BenchFilePathObjT17" );
BENCH_FILEPATH_OBJECT_T( 18 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT18, "BenchFilePathObjT18" );
BENCH_FILEPATH_OBJECT_T( 19 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT19, "BenchFilePathObjT19" );
BENCH_FILEPATH_OBJECT_T( 20 ) CAF_PDM_SOURCE_INIT( BenchFilePathObjT20, "BenchFilePathObjT20" );

// clang-format on
