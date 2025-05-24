#include "gtest/gtest.h"

#include "RiaCurveMerger.h"

#include "RiaWeightedMeanCalculator.h"
#include <cmath> // Needed for HUGE_VAL on Linux
#include <random>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, TestDateInterpolation )
{
    std::vector<double> values{ 2.0, 3.5, 5.0, 6.0 };
    std::vector<time_t> timeSteps{ 1, 5, 10, 15 };
    bool                isMonotonicallyIncreasing = true;

    {
        double val = RiaTimeHistoryCurveMerger::interpolatedYValue( 1, timeSteps, values, isMonotonicallyIncreasing );

        EXPECT_EQ( 2.0, val );
    }

    {
        double val = RiaTimeHistoryCurveMerger::interpolatedYValue( 0, timeSteps, values, isMonotonicallyIncreasing );

        EXPECT_EQ( HUGE_VAL, val );
    }

    {
        double val = RiaTimeHistoryCurveMerger::interpolatedYValue( 20, timeSteps, values, isMonotonicallyIncreasing );

        EXPECT_EQ( HUGE_VAL, val );
    }

    {
        double val = RiaTimeHistoryCurveMerger::interpolatedYValue( 3, timeSteps, values, isMonotonicallyIncreasing );

        EXPECT_EQ( 2.75, val );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, ExtractIntervalsWithSameTimeSteps )
{
    std::vector<double> valuesA{ HUGE_VAL, 1.0, HUGE_VAL, 2.0, 2.5, 3.0, 4.0, 5.0, 6.0, HUGE_VAL };
    std::vector<double> valuesB{ 10, 20, 30, 40, 45, HUGE_VAL, HUGE_VAL, 5.0, 6.0, HUGE_VAL };

    EXPECT_EQ( valuesA.size(), valuesB.size() );

    std::vector<time_t> timeSteps;
    for ( size_t i = 0; i < 10; i++ )
    {
        timeSteps.push_back( i );
    }

    RiaTimeHistoryCurveMerger interpolate;
    interpolate.addCurveData( timeSteps, valuesA );
    interpolate.addCurveData( timeSteps, valuesB );
    interpolate.computeInterpolatedValues( true );

    auto interpolatedTimeSteps = interpolate.allXValues();
    auto intervals             = interpolate.validIntervalsForAllXValues();

    EXPECT_EQ( 10, static_cast<int>( interpolatedTimeSteps.size() ) );
    EXPECT_EQ( 3, static_cast<int>( intervals.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, ExtractIntervalsWithSameTimeStepsOneComplete )
{
    std::vector<double> valuesA{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
    std::vector<double> valuesB{ 10, 20, 30, HUGE_VAL, 50, HUGE_VAL, 70 };

    EXPECT_EQ( valuesA.size(), valuesB.size() );

    std::vector<time_t> timeSteps;
    for ( size_t i = 0; i < 7; i++ )
    {
        timeSteps.push_back( i );
    }

    RiaTimeHistoryCurveMerger interpolate;
    interpolate.addCurveData( timeSteps, valuesA );
    interpolate.addCurveData( timeSteps, valuesB );
    interpolate.computeInterpolatedValues( true );

    auto interpolatedTimeSteps = interpolate.allXValues();
    auto intervals             = interpolate.validIntervalsForAllXValues();

    EXPECT_EQ( 7, static_cast<int>( interpolatedTimeSteps.size() ) );
    EXPECT_EQ( 3, static_cast<int>( intervals.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, ExtractIntervalsWithSameTimeStepsBothComplete )
{
    std::vector<double> valuesA{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
    std::vector<double> valuesB{ 10, 20, 30, 40, 50, 60, 70 };

    EXPECT_EQ( valuesA.size(), valuesB.size() );

    std::vector<time_t> timeSteps;
    for ( size_t i = 0; i < 7; i++ )
    {
        timeSteps.push_back( i );
    }

    RiaTimeHistoryCurveMerger interpolate;
    interpolate.addCurveData( timeSteps, valuesA );
    interpolate.addCurveData( timeSteps, valuesB );
    interpolate.computeInterpolatedValues( true );

    auto interpolatedTimeSteps = interpolate.allXValues();
    auto intervals             = interpolate.validIntervalsForAllXValues();

    EXPECT_EQ( 7, static_cast<int>( interpolatedTimeSteps.size() ) );
    EXPECT_EQ( 1, static_cast<int>( intervals.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, OverlappintTimes )
{
    std::vector<double> valuesA{ 1, 2, 3, 4, 5 };
    std::vector<double> valuesB{ 10, 20, 30, 40, 50 };

    EXPECT_EQ( valuesA.size(), valuesB.size() );

    std::vector<time_t> timeStepsA{ 0, 10, 11, 15, 20 };
    std::vector<time_t> timeStepsB{ 1, 2, 3, 5, 7 };

    RiaTimeHistoryCurveMerger interpolate;
    interpolate.addCurveData( timeStepsA, valuesA );
    interpolate.addCurveData( timeStepsB, valuesB );
    interpolate.computeInterpolatedValues( true );
    EXPECT_EQ( 2, static_cast<int>( interpolate.curveCount() ) );

    auto interpolatedTimeSteps = interpolate.allXValues();
    auto intervals             = interpolate.validIntervalsForAllXValues();

    EXPECT_EQ( 10, static_cast<int>( interpolatedTimeSteps.size() ) );
    EXPECT_EQ( 1, static_cast<int>( intervals.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, RobustUse )
{
    {
        RiaTimeHistoryCurveMerger curveMerger;
        curveMerger.computeInterpolatedValues( true );
        EXPECT_EQ( 0, static_cast<int>( curveMerger.allXValues().size() ) );
    }

    std::vector<double> valuesA{ 1, 2, 3, 4, 5 };
    std::vector<double> valuesB{ 10, 20, 30 };

    std::vector<time_t> timeStepsA{ 0, 10, 11, 15, 20 };
    std::vector<time_t> timeStepsB{ 1, 2, 3 };

    {
        RiaTimeHistoryCurveMerger curveMerger;
        curveMerger.addCurveData( timeStepsA, valuesA );
        curveMerger.computeInterpolatedValues( true );
        EXPECT_EQ( timeStepsA.size(), curveMerger.allXValues().size() );
        EXPECT_EQ( timeStepsA.size(), curveMerger.interpolatedYValuesForAllXValues( 0 ).size() );
    }

    {
        RiaTimeHistoryCurveMerger curveMerger;
        curveMerger.addCurveData( timeStepsA, valuesA );
        curveMerger.addCurveData( timeStepsB, valuesB );

        // Execute interpolation twice is allowed
        curveMerger.computeInterpolatedValues( true );
        curveMerger.computeInterpolatedValues( true );
        EXPECT_EQ( 8, static_cast<int>( curveMerger.allXValues().size() ) );
        EXPECT_EQ( 8, static_cast<int>( curveMerger.interpolatedYValuesForAllXValues( 0 ).size() ) );
        EXPECT_EQ( 8, static_cast<int>( curveMerger.interpolatedYValuesForAllXValues( 1 ).size() ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, NoTimeStepOverlap )
{
    std::vector<double> valuesA{ 1, 2, 3, 4, 5 };
    std::vector<double> valuesB{ 10, 20, 30 };

    std::vector<time_t> timeStepsA{ 0, 10, 11, 15, 20 };
    std::vector<time_t> timeStepsB{ 100, 200, 300 };

    {
        RiaTimeHistoryCurveMerger curveMerger;
        curveMerger.addCurveData( timeStepsA, valuesA );
        curveMerger.addCurveData( timeStepsB, valuesB );

        // Execute interpolation twice is allowed
        curveMerger.computeInterpolatedValues( true );
        EXPECT_EQ( 8, static_cast<int>( curveMerger.allXValues().size() ) );
        EXPECT_EQ( 0, static_cast<int>( curveMerger.validIntervalsForAllXValues().size() ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, SharedXValues )
{
    std::vector<double> valuesA{ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0 };
    std::vector<double> valuesB{ 10, 20, 30, 40, 50, 60, 70 };
    std::vector<time_t> timeSteps{ 1, 2, 3, 4, 5, 6, 7 };

    RiaTimeHistoryCurveMerger interpolate;
    interpolate.addCurveData( timeSteps, valuesA );
    interpolate.addCurveData( timeSteps, valuesB );
    interpolate.computeInterpolatedValues( true );

    auto interpolatedTimeSteps = interpolate.allXValues();
    EXPECT_TRUE( std::equal( timeSteps.begin(), timeSteps.end(), interpolatedTimeSteps.begin() ) );

    auto generatedYValuesA = interpolate.interpolatedYValuesForAllXValues( 0 );
    EXPECT_TRUE( std::equal( valuesA.begin(), valuesA.end(), generatedYValuesA.begin() ) );

    auto generatedYValuesB = interpolate.interpolatedYValuesForAllXValues( 1 );
    EXPECT_TRUE( std::equal( valuesB.begin(), valuesB.end(), generatedYValuesB.begin() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaTimeHistoryCurveMergerTest, PerformanceInterpolation )
{
    std::vector<double> valuesA;
    std::vector<double> valuesB;
    std::vector<time_t> timeStepsA;
    std::vector<time_t> timeStepsB;

    bool isMonotonicallyIncreasing = true;

    const size_t maxTimeStep = 10000;
    for ( size_t i = 0; i < maxTimeStep; i++ )
    {
        valuesA.push_back( static_cast<double>( i ) );
        valuesB.push_back( static_cast<double>( i ) * 2.0 );
        timeStepsA.push_back( static_cast<time_t>( i ) );
        timeStepsB.push_back( static_cast<time_t>( i ) + 0.5 );
    }

    // create random time steps
    std::vector<time_t> timeSteps;
    for ( size_t i = 0; i < maxTimeStep; i++ )
    {
        timeSteps.push_back( static_cast<time_t>( i ) + 0.25 );
    }

    std::shuffle( timeSteps.begin(), timeSteps.end(), std::default_random_engine( 0 ) );

    auto interpolationStart = std::chrono::high_resolution_clock::now();
    {
        const size_t N = 10000;
        for ( size_t i = 0; i < N; i++ )
        {
            for ( auto timeStep : timeSteps )
            {
                double interpolatedA = RiaTimeHistoryCurveMerger::interpolatedYValue( timeStep, timeSteps, valuesA, isMonotonicallyIncreasing );
                double interpolatedB = RiaTimeHistoryCurveMerger::interpolatedYValue( timeStep, timeSteps, valuesB, isMonotonicallyIncreasing );
            }
        }
    }

    auto                                      interpolationEnd      = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> interpolationDuration = interpolationEnd - interpolationStart;
    std::cout << "Direct interpolation time for " << maxTimeStep << " values: " << interpolationDuration << " ms" << std::endl;
}
