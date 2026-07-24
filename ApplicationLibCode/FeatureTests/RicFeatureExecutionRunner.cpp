/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RicFeatureExecutionRunner.h"

#include "RiaFeatureTestModelBuilder.h"
#include "RiaViewRedrawScheduler.h"
#include "RicFeatureSweepDenylist.h"

#include "RimEclipseCase.h"
#include "RimGridView.h"
#include "RimProject.h"
#include "RimWellPath.h"

#include "cafCmdFeature.h"
#include "cafCmdFeatureManager.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmPointer.h"
#include "cafPdmUiItem.h"
#include "cafSelectionManager.h"

#include <QAction>
#include <QApplication>
#include <QTimer>
#include <QWidget>

#include <iostream>
#include <vector>

namespace
{
enum class ScenarioKind
{
    ECLIPSE_CASE,
    ECLIPSE_VIEW,
    WELL_PATH
};

struct Scenario
{
    const char*  name;
    ScenarioKind kind;
};

// Only populated scenarios are executed. Executing a feature with an empty selection is not
// meaningful and mostly trips preconditions.
const std::vector<Scenario>& executionScenarios()
{
    static const std::vector<Scenario> scenarios = {
        { "EclipseCase", ScenarioKind::ECLIPSE_CASE },
        { "EclipseView", ScenarioKind::ECLIPSE_VIEW },
        { "WellPath", ScenarioKind::WELL_PATH },
    };
    return scenarios;
}

//--------------------------------------------------------------------------------------------------
/// Closes any modal dialog that appears while a feature is executing.
///
/// A feature that opens a modal dialog is treated as a success (it ran up to the dialog without
/// crashing); the dialog is simply dismissed so the run continues.
//--------------------------------------------------------------------------------------------------
class ModalDialogWatchdog
{
public:
    ModalDialogWatchdog()
    {
        m_timer.setInterval( 200 );
        QObject::connect( &m_timer,
                          &QTimer::timeout,
                          []()
                          {
                              if ( QWidget* modal = QApplication::activeModalWidget() )
                              {
                                  modal->close();
                              }
                          } );
        m_timer.start();
    }

    ~ModalDialogWatchdog() { m_timer.stop(); }

private:
    QTimer m_timer;
};

// The combined model is reused across features to avoid the expensive Eclipse rebuild. It is only
// rebuilt when a previously executed feature broke it: closed the project or deleted one of the
// objects the scenarios select (detected via the dangling-safe caf::PdmPointer guards).
FeatureTestModel                      s_model;
caf::PdmPointer<caf::PdmObjectHandle> s_eclipseCaseGuard;
caf::PdmPointer<caf::PdmObjectHandle> s_eclipseViewGuard;
caf::PdmPointer<caf::PdmObjectHandle> s_wellPathGuard;
bool                                  s_modelBuilt = false;

void rebuildModel()
{
    caf::SelectionManager::instance()->clearAll();
    s_model = RiaFeatureTestModelBuilder::combinedModel();

    s_eclipseCaseGuard = s_model.eclipseCase;
    s_eclipseViewGuard = s_model.eclipseView;
    s_wellPathGuard    = s_model.wellPath;
    s_modelBuilt       = true;
}

bool modelNeedsRebuild()
{
    if ( !s_modelBuilt ) return true;
    if ( RimProject::current() == nullptr ) return true;
    if ( s_eclipseCaseGuard.isNull() ) return true;
    if ( s_eclipseViewGuard.isNull() ) return true;
    if ( s_wellPathGuard.isNull() ) return true;
    return false;
}

// Select the objects for a scenario. Returns false if the scenario cannot be represented (e.g. the
// model failed to build), in which case the caller skips it.
bool applySelection( ScenarioKind kind )
{
    caf::SelectionManager::instance()->clearAll();

    caf::PdmUiItem* item = nullptr;
    switch ( kind )
    {
        case ScenarioKind::ECLIPSE_CASE:
            item = s_model.eclipseCase;
            break;
        case ScenarioKind::ECLIPSE_VIEW:
            item = s_model.eclipseView;
            break;
        case ScenarioKind::WELL_PATH:
            item = s_model.wellPath;
            break;
    }

    if ( !item ) return false;

    caf::SelectionManager::instance()->setSelectedItem( item );
    return true;
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicFeatureExecutionRunner::executeFeature( const std::string& commandId )
{
    if ( isFeatureDenylisted( commandId ) ) return true;

    caf::CmdFeature* feature = caf::CmdFeatureManager::instance()->getCommandFeature( commandId );
    if ( !feature ) return false;

    for ( const Scenario& scenario : executionScenarios() )
    {
        if ( modelNeedsRebuild() ) rebuildModel();

        if ( !applySelection( scenario.kind ) ) continue;

        // Re-resolve after a rebuild; the feature instance itself is owned by the manager and stable,
        // but the enabled state depends on the selection just applied.
        if ( !feature->canFeatureBeExecuted() ) continue;

        // Attribution line: printed and flushed before execution so a hard crash in a child process
        // still identifies the culprit in the captured output.
        std::cout << "[exec] " << scenario.name << " : " << commandId << std::endl;

        ModalDialogWatchdog watchdog;

        // Trigger through the QAction rather than calling actionTriggered() directly. Features that
        // read caf::CmdFeature::userData() get it from qobject_cast<QAction*>( sender() ), which
        // asserts when the slot is invoked as a plain function call. Going through the action is also
        // how the application itself invokes a feature.
        if ( QAction* action = feature->action() )
        {
            action->trigger();
        }
        else
        {
            feature->actionTriggered( false );
        }

        // Deliberately do NOT pump the event loop here.
        //
        // RiaGuiApplication::initialize() creates and shows the main window, so the 3D viewer widgets
        // exist and Qt has paint events queued for them. Offscreen there is no OpenGL context, and
        // caf::Viewer::paintGL() dereferences the null context, which shows up as an access violation
        // blamed on whichever feature happened to run. Measured: with processEvents() the cell-filter
        // features all crash; without it they pass. Discarding the scheduled redraws is not enough,
        // because the paint events come from Qt rather than from RiaViewRedrawScheduler.
        //
        // Exercising real rendering requires the software-GL tier, not this one, so drop the pending
        // redraws and leave the event loop alone.
        RiaViewRedrawScheduler::instance()->clearViewsScheduledForUpdate();

        if ( RimProject::current() == nullptr ) return false;
    }

    // Phase marker: everything above is feature execution, everything after is harness teardown. A
    // child that crashes without printing this crashed inside the feature; one that crashes after it
    // crashed while closing the project, which is a harness problem and not a feature defect.
    std::cout << executionCompleteMarker << " " << commandId << std::endl;

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicFeatureExecutionRunner::releaseModel()
{
    caf::SelectionManager::instance()->clearAll();
    RiaFeatureTestModelBuilder::closeProject();
    s_modelBuilt = false;
}
