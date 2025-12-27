//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
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

#pragma once

#include <cstddef>

#include <atomic>

class QString;

namespace caf
{
class ProgressInfo;

//==================================================================================================
/// RAII helper class that automatically increments parent progress on destruction
/// \code
/// caf::ProgressInfo progress(3, "Processing");
/// {
///     auto task = progress.task("Step 1");
///     // Do work...
/// } // Automatically increments progress by 1
/// \endcode
//==================================================================================================
class ProgressTask
{
public:
    ProgressTask( ProgressInfo& parentTask );
    ~ProgressTask();

private:
    ProgressInfo& m_parentTask;
};

//==================================================================================================
/// Main progress reporting class with support for hierarchical progress tracking
///
/// This class provides a simple frontend to the Qt progress dialog, allowing distributed
/// progress calculation across nested function calls.
///
/// Key Features:
/// - Hierarchical/nested progress tracking
/// - Thread-safe cancellation support
/// - Automatic cursor management
/// - Task-based progress with RAII
///
/// Basic Usage:
/// \code
/// caf::ProgressInfo progress(100, "Loading Data");
/// progress.setProgressDescription("Reading file");
/// // ... read file
/// progress.setProgress(50);
/// progress.setProgressDescription("Processing data");
/// // ... process
/// progress.setProgress(100);
/// \endcode
///
/// Nested Progress Example:
/// \code
/// caf::ProgressInfo outerProgress(3, "Full Pipeline");
/// {
///     caf::ProgressInfo innerProgress(100, "Step 1");
///     // ... inner work with detailed progress
/// }
/// outerProgress.incrementProgress();
/// \endcode
///
/// Task-based Progress:
/// \code
/// caf::ProgressInfo progress(5, "Processing");
/// {
///     auto task = progress.task("Initialize", 1);
///     // Task automatically increments on destruction
/// }
/// \endcode
///
/// Important Limitations:
/// - Progress updates must be from the same thread that shows the dialog
/// - Nested progress requires incrementProgress() between independent nested tasks
/// - Multiple ProgressInfo objects in the same scope are not supported
///
//==================================================================================================
class ProgressInfo
{
public:
    /// Constructor
    /// \param maxProgressValue Maximum value for this progress level (e.g., 100 for percentage, or item count)
    /// \param title Title shown in the progress dialog
    /// \param delayShowingProgress If true, dialog appears after 2 seconds delay
    /// \param allowCancel If true, shows a cancel button that can be checked via isCancelled()
    ProgressInfo( size_t maxProgressValue, const QString& title, bool delayShowingProgress = true, bool allowCancel = false );

    ~ProgressInfo();

    /// Set descriptive text for current operation (appended to title as "Title: Description")
    void setProgressDescription( const QString& description );

    /// Set absolute progress value (must be <= maxProgressValue)
    void setProgress( size_t progressValue );

    /// Increment progress by current step size (default 1, or value set by setNextProgressIncrement)
    void incrementProgress();

    /// Set the increment amount for the next call to incrementProgress()
    /// \param nextStepSize Number of progress units the next increment should span
    void setNextProgressIncrement( size_t nextStepSize );

    /// Request cancellation of the operation
    void cancel();

    /// Check if cancellation was requested
    bool isCancelled() const;

    /// Create a RAII task that automatically increments progress on destruction
    /// \param description Description of the task
    /// \param stepSize Number of progress units this task represents
    ProgressTask task( const QString& description, int stepSize = 1 );

private:
    std::atomic<bool> m_isCancelled;
};

//==================================================================================================
/// Blocks event processing while a progress dialog is shown
///
/// This is required when the progress dialog is shown from a non-GUI thread to prevent
/// event loop issues.
///
/// Usage:
/// \code
/// caf::ProgressInfoEventProcessingBlocker blocker;
/// // Progress updates won't call processEvents()
/// \endcode
//==================================================================================================
class ProgressInfoEventProcessingBlocker
{
public:
    ProgressInfoEventProcessingBlocker();
    ~ProgressInfoEventProcessingBlocker();
};

//==================================================================================================
/// Temporarily disables all progress reporting
///
/// Useful for nested operations where you want to suppress intermediate progress updates.
///
/// Usage:
/// \code
/// caf::ProgressInfoBlocker blocker;
/// // Progress updates are suppressed in this scope
/// \endcode
//==================================================================================================
class ProgressInfoBlocker
{
public:
    ProgressInfoBlocker();
    ~ProgressInfoBlocker();
};

//==================================================================================================
/// Static interface for progress reporting
///
/// Provides static methods for controlling progress. Generally, you should use the ProgressInfo
/// class instead of calling these methods directly, as ProgressInfo handles lifecycle management.
///
/// These methods are used internally by ProgressInfo and can be used for advanced scenarios.
//==================================================================================================
class ProgressInfoStatic
{
public:
    static void start( ProgressInfo&  progressInfo,
                       size_t         maxProgressValue,
                       const QString& title,
                       bool           delayShowingProgress,
                       bool           allowCancel );

    static void setProgressDescription( const QString& description );
    static void setProgress( size_t progressValue );
    static void incrementProgress();
    static void setNextProgressIncrement( size_t nextStepSize );
    static bool isRunning();
    static void finished();

    /// Enable or disable all progress reporting globally
    static void setEnabled( bool enable );

private:
    static bool isUpdatePossible();

private:
    friend class ProgressInfoBlocker;
    friend class ProgressInfoEventProcessingBlocker;

    static std::atomic<bool> s_running;

    static bool s_disabled;
    static bool s_isButtonConnected;
    static bool s_shouldProcessEvents;
};

//==================================================================================================
/// RAII-based progress scope with automatic progress tracking
///
/// Simplifies common progress patterns with automatic initialization and cleanup.
///
/// Usage:
/// \code
/// void processFiles(const std::vector<QString>& files) {
///     caf::ProgressScope progress(files.size(), "Processing Files");
///     for (const auto& file : files) {
///         if (progress.isCancelled()) break;
///         progress.setDescription(QString("Processing %1").arg(file));
///         // Do work...
///         progress.increment();
///     }
/// }
/// \endcode
//==================================================================================================
class ProgressScope
{
public:
    /// Constructor with step count
    /// \param maxSteps Total number of steps
    /// \param title Title for the progress dialog
    /// \param delayShowingProgress Whether to delay showing the dialog
    /// \param allowCancel Whether to show a cancel button
    ProgressScope( size_t maxSteps, const QString& title, bool delayShowingProgress = true, bool allowCancel = false )
        : m_progress( maxSteps, title, delayShowingProgress, allowCancel )
    {
    }

    /// Set description for current step
    void setDescription( const QString& description ) { m_progress.setProgressDescription( description ); }

    /// Increment progress by one step
    void increment() { m_progress.incrementProgress(); }

    /// Set absolute progress value
    void setProgress( size_t value ) { m_progress.setProgress( value ); }

    /// Check if operation was cancelled
    bool isCancelled() const { return m_progress.isCancelled(); }

    /// Get underlying ProgressInfo object
    ProgressInfo& progressInfo() { return m_progress; }

private:
    ProgressInfo m_progress;
};

//==================================================================================================
/// Percentage-based progress helper
///
/// Provides a simpler API for percentage-based progress (0-100).
///
/// Usage:
/// \code
/// caf::ProgressPercentage progress("Loading Data");
/// progress.setPercentage(25.5);
/// progress.setDescription("Reading header");
/// progress.setPercentage(50.0);
/// \endcode
//==================================================================================================
class ProgressPercentage
{
public:
    /// Constructor
    /// \param title Title for the progress dialog
    /// \param delayShowingProgress Whether to delay showing the dialog
    /// \param allowCancel Whether to show a cancel button
    ProgressPercentage( const QString& title, bool delayShowingProgress = true, bool allowCancel = false )
        : m_progress( 1000, title, delayShowingProgress, allowCancel )
        , m_lastValue( 0 )
    {
    }

    /// Set progress as a percentage (0.0 - 100.0)
    void setPercentage( double percentage )
    {
        size_t value = static_cast<size_t>( percentage * 10.0 );
        if ( value > 1000 ) value = 1000;
        if ( value != m_lastValue )
        {
            m_progress.setProgress( value );
            m_lastValue = value;
        }
    }

    /// Set description for current operation
    void setDescription( const QString& description ) { m_progress.setProgressDescription( description ); }

    /// Check if operation was cancelled
    bool isCancelled() const { return m_progress.isCancelled(); }

    /// Get underlying ProgressInfo object
    ProgressInfo& progressInfo() { return m_progress; }

private:
    ProgressInfo m_progress;
    size_t       m_lastValue;
};

} // namespace caf
