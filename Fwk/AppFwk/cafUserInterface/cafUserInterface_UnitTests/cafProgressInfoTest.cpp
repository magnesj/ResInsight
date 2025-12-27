//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2024 Ceetron Solutions AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "gtest/gtest.h"

#include "cafProgressInfo.h"

#include <QApplication>
#include <thread>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, BasicConstruction )
{
    // Disable progress to avoid showing dialogs during tests
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 100, "Test Progress" );
        EXPECT_FALSE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, SetProgress )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 100, "Test Progress" );
        progress.setProgress( 50 );
        EXPECT_FALSE( progress.isCancelled() );
        progress.setProgress( 100 );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, IncrementProgress )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 10, "Test Progress" );
        for ( int i = 0; i < 10; i++ )
        {
            progress.incrementProgress();
        }
        EXPECT_FALSE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, NestedProgress )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo outerProgress( 3, "Outer Progress" );

        {
            caf::ProgressInfo innerProgress( 10, "Inner Progress 1" );
            for ( int i = 0; i < 10; i++ )
            {
                innerProgress.incrementProgress();
            }
        }

        outerProgress.incrementProgress();

        {
            caf::ProgressInfo innerProgress( 5, "Inner Progress 2" );
            for ( int i = 0; i < 5; i++ )
            {
                innerProgress.incrementProgress();
            }
        }

        outerProgress.incrementProgress();
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, ProgressDescription )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 100, "Test Progress" );
        progress.setProgressDescription( "Loading data" );
        progress.setProgress( 50 );
        progress.setProgressDescription( "Processing data" );
        progress.setProgress( 100 );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, ProgressTask )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 5, "Test Progress" );

        {
            auto task = progress.task( "Task 1" );
            // Task automatically increments on destruction
        }

        {
            auto task = progress.task( "Task 2", 2 );
            // Task automatically increments by 2 on destruction
        }
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, Cancellation )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 100, "Test Progress" );
        EXPECT_FALSE( progress.isCancelled() );

        progress.cancel();
        EXPECT_TRUE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, ProgressInfoBlocker )
{
    caf::ProgressInfoStatic::setEnabled( true );

    {
        caf::ProgressInfoBlocker blocker;
        // Progress should be disabled inside this scope
        caf::ProgressInfo progress( 100, "Test Progress" );
        progress.setProgress( 50 );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressInfoTest, NextProgressIncrement )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressInfo progress( 10, "Test Progress" );
        progress.setNextProgressIncrement( 3 );
        progress.incrementProgress(); // Should increment by 3

        progress.setNextProgressIncrement( 5 );
        progress.incrementProgress(); // Should increment by 5
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressScopeTest, BasicUsage )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressScope progress( 10, "Test Progress Scope" );
        for ( int i = 0; i < 10; i++ )
        {
            progress.setDescription( QString( "Step %1" ).arg( i ) );
            progress.increment();
            EXPECT_FALSE( progress.isCancelled() );
        }
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressScopeTest, WithCancellation )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressScope progress( 10, "Test Progress Scope", true, true );
        for ( int i = 0; i < 10; i++ )
        {
            if ( i == 5 )
            {
                progress.progressInfo().cancel();
            }
            if ( progress.isCancelled() )
            {
                break;
            }
            progress.increment();
        }
        EXPECT_TRUE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressPercentageTest, BasicUsage )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressPercentage progress( "Test Percentage Progress" );
        progress.setPercentage( 0.0 );
        progress.setDescription( "Starting" );
        progress.setPercentage( 25.5 );
        progress.setDescription( "Quarter done" );
        progress.setPercentage( 50.0 );
        progress.setDescription( "Halfway" );
        progress.setPercentage( 75.0 );
        progress.setDescription( "Almost there" );
        progress.setPercentage( 100.0 );
        EXPECT_FALSE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProgressPercentageTest, BoundaryValues )
{
    caf::ProgressInfoStatic::setEnabled( false );

    {
        caf::ProgressPercentage progress( "Test Percentage Progress" );
        progress.setPercentage( -10.0 ); // Should clamp to 0
        progress.setPercentage( 150.0 ); // Should clamp to 100
        EXPECT_FALSE( progress.isCancelled() );
    }

    caf::ProgressInfoStatic::setEnabled( true );
}
