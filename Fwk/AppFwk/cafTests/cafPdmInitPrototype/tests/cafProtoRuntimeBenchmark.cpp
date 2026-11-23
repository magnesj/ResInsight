#include "gtest/gtest.h"

#include "cafProtoTestObjects.h"

#include <QUuid>

#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <windows.h>

#include <malloc.h>
#else
#include <malloc.h>
#endif

// Runtime benchmark for issue #1202, step 1.2. Disabled by default, run with
//
//   cafPdmInitPrototype_UnitTests --gtest_also_run_disabled_tests --gtest_filter=*ProtoBenchmark*

namespace
{
//--------------------------------------------------------------------------------------------------
/// Bytes currently allocated from the heap
//--------------------------------------------------------------------------------------------------
size_t processMemory()
{
#ifdef _WIN32
    HEAP_SUMMARY summary = {};
    summary.cb           = sizeof( summary );
    if ( !HeapSummary( reinterpret_cast<HANDLE>( _get_heap_handle() ), 0, &summary ) ) return 0;
    return summary.cbAllocated;
#else
    return mallinfo2().uordblks;
#endif
}

struct Result
{
    double constructNanoSeconds = 0.0;
    double destroyNanoSeconds   = 0.0;
    double bytesPerObject       = 0.0;
};

//--------------------------------------------------------------------------------------------------
/// Construct and destroy objectCount objects. The best of several repetitions is reported.
//--------------------------------------------------------------------------------------------------
template <typename ObjectType>
Result measure( size_t objectCount, int repetitions )
{
    using Clock = std::chrono::steady_clock;

    Result best;

    // Create static information and registrations before the measurement
    {
        ObjectType warmUp;
    }

    for ( int r = 0; r < repetitions; r++ )
    {
        std::vector<std::unique_ptr<ObjectType>> objects;
        objects.reserve( objectCount );

        size_t memoryBefore = processMemory();
        auto   start        = Clock::now();

        for ( size_t i = 0; i < objectCount; i++ )
        {
            objects.push_back( std::make_unique<ObjectType>() );
        }

        auto   constructed = Clock::now();
        size_t memoryAfter = processMemory();

        objects.clear();

        auto destroyed = Clock::now();

        Result result;
        result.constructNanoSeconds = std::chrono::duration<double, std::nano>( constructed - start ).count() / objectCount;
        result.destroyNanoSeconds =
            std::chrono::duration<double, std::nano>( destroyed - constructed ).count() / objectCount;
        result.bytesPerObject =
            memoryAfter > memoryBefore ? static_cast<double>( memoryAfter - memoryBefore ) / objectCount : 0.0;

        if ( r == 0 || result.constructNanoSeconds < best.constructNanoSeconds ) best = result;
    }

    return best;
}

void report( const char* name, const Result& result, const Result& reference )
{
    std::printf( "| %-34s | %9.0f | %7.2f | %9.0f | %9.0f | %7.2f |\n",
                 name,
                 result.constructNanoSeconds,
                 result.constructNanoSeconds / reference.constructNanoSeconds,
                 result.destroyNanoSeconds,
                 result.bytesPerObject,
                 result.bytesPerObject / reference.bytesPerObject );
}

void reportHeader( const char* title, size_t objectCount )
{
    std::printf( "\n%s, %zu objects with ten fields\n\n", title, objectCount );
    std::printf( "| %-34s | %9s | %7s | %9s | %9s | %7s |\n", "Variant", "construct", "ratio", "destroy", "memory", "ratio" );
    std::printf( "| %-34s | %9s | %7s | %9s | %9s | %7s |\n", "", "ns/object", "", "ns/object", "bytes/obj", "" );
    std::printf( "|------------------------------------|-----------|---------|-----------|-----------|---------|\n" );
}

const size_t OBJECT_COUNT = 200000;
const int    REPETITIONS  = 5;

} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoBenchmark, DISABLED_ObjectConstruction )
{
    std::printf( "sizeof( caf::PdmUiItemInfo ) = %zu bytes\n", sizeof( caf::PdmUiItemInfo ) );

    {
        reportHeader( "Fields that are not scriptable", OBJECT_COUNT );

        auto macro = measure<BenchPlainMacroObj>( OBJECT_COUNT, REPETITIONS );
        report( "Macros", macro, macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::INTERNED );
        report( "Functions, interned information", measure<BenchPlainFnObj>( OBJECT_COUNT, REPETITIONS ), macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::CLASS_REGISTRY );
        report( "Functions, registry per class", measure<BenchPlainFnObj>( OBJECT_COUNT, REPETITIONS ), macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::PER_INSTANCE );
        report( "Functions, information per field", measure<BenchPlainFnObj>( OBJECT_COUNT, 1 ), macro );
        cafproto::clearMetadata();
    }

    {
        reportHeader( "Scriptable fields", OBJECT_COUNT );

        auto macro = measure<BenchScriptMacroObj>( OBJECT_COUNT, REPETITIONS );
        report( "Macros", macro, macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::INTERNED );
        report( "Functions, interned information", measure<BenchScriptFnObj>( OBJECT_COUNT, REPETITIONS ), macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::CLASS_REGISTRY );
        report( "Functions, registry per class", measure<BenchScriptFnObj>( OBJECT_COUNT, REPETITIONS ), macro );

        cafproto::setMetadataStorage( cafproto::MetadataStorage::PER_INSTANCE );
        report( "Functions, information per field", measure<BenchScriptFnObj>( OBJECT_COUNT, 1 ), macro );
        cafproto::clearMetadata();
    }

    cafproto::setMetadataStorage( cafproto::MetadataStorage::INTERNED );
}

//--------------------------------------------------------------------------------------------------
/// Cost of generating a UUID for every object, for the object identity proposal
//--------------------------------------------------------------------------------------------------
TEST( ProtoBenchmark, DISABLED_UuidGeneration )
{
    using Clock = std::chrono::steady_clock;

    const size_t count = 1000000;

    std::vector<QUuid> uuids( count );

    auto start = Clock::now();
    for ( auto& uuid : uuids )
    {
        uuid = QUuid::createUuid();
    }
    auto created = Clock::now();

    std::vector<QString> texts( count );
    for ( size_t i = 0; i < count; i++ )
    {
        texts[i] = uuids[i].toString( QUuid::WithoutBraces );
    }
    auto converted = Clock::now();

    std::printf( "\nQUuid::createUuid()  : %6.0f ns per UUID, %zu bytes per UUID\n",
                 std::chrono::duration<double, std::nano>( created - start ).count() / count,
                 sizeof( QUuid ) );
    std::printf( "QUuid::toString()    : %6.0f ns per UUID\n",
                 std::chrono::duration<double, std::nano>( converted - created ).count() / count );
}
