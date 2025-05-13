/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024     Equinor ASA
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

#include "RimPathPatternEnsemble.h"

#include "Ensemble/RiaEnsembleImportTools.h"
#include "RiaFilePathTools.h"
#include "RiaLogging.h"

#include "EnsembleFileSet/RimEnsembleFileSet.h"
#include "EnsembleFileSet/RimEnsembleFileSetCollection.h"
#include "RimSummaryCase.h"

CAF_PDM_SOURCE_INIT( RimPathPatternEnsemble, "RimPathPatternEnsemble" );

namespace internal
{
const QString pathPatternPlaceholder = "*";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPathPatternEnsemble::RimPathPatternEnsemble()
{
    CAF_PDM_InitObject( "Path Pattern Ensemble", ":/SummaryCase.svg" );

    CAF_PDM_InitFieldNoDefault( &m_ensembleFileSet, "EnsembleFileSet", "Ensemble File Set" );

    m_cases.xmlCapability()->disableIO();

    setAsEnsemble( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::setEnsembleFileSet( RimEnsembleFileSet* ensembleFileSet )
{
    m_ensembleFileSet = ensembleFileSet;
    connectSignals();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::updateName( const std::set<QString>& existingEnsembleNames )
{
    QString candidateName = m_ensembleFileSet() ? m_ensembleFileSet()->name() : "Path Pattern Ensemble";

    if ( m_name == candidateName ) return;

    m_name = candidateName;
    caseNameChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::cleanupBeforeDelete()
{
    if ( m_ensembleFileSet() )
    {
        m_ensembleFileSet()->fileSetChanged.disconnect( this );
        m_ensembleFileSet()->nameChanged.disconnect( this );

        auto fileSet      = m_ensembleFileSet();
        m_ensembleFileSet = nullptr;

        if ( auto coll = fileSet->firstAncestorOrThisOfType<RimEnsembleFileSetCollection>() )
        {
            coll->deleteFileSetIfPossible( fileSet );
        }
    }
    RimSummaryEnsemble::cleanupBeforeDelete();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimPathPatternEnsemble::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    if ( fieldNeedingOptions == &m_ensembleFileSet )
    {
        return RimEnsembleFileSet::ensembleFilSetOptions();
    }
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::onFileSetChanged( const caf::SignalEmitter* emitter )
{
    createSummaryCasesFromEnsembleFileSet();
    buildChildNodes();
    updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::onFileSetNameChanged( const caf::SignalEmitter* emitter )
{
    updateName( {} );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::createSummaryCasesFromEnsembleFileSet()
{
    m_cases.deleteChildrenAsync();

    if ( m_ensembleFileSet() )
    {
        auto paths = m_ensembleFileSet()->createPaths( ".SMSPEC" );

        RiaDefines::FileType fileType = RiaDefines::FileType::SMSPEC;

        RiaEnsembleImportTools::CreateConfig createConfig{ .fileType = fileType, .ensembleOrGroup = false, .allowDialogs = false };
        auto                                 newCases = RiaEnsembleImportTools::createSummaryCasesFromFiles( paths, createConfig );
        if ( newCases.empty() )
        {
            RiaLogging::warning( "No new cases are created." );
            return;
        }

        replaceCases( newCases );

        // Update name of cases and ensemble after all cases are added
        for ( auto summaryCase : newCases )
        {
            summaryCase->setShowVectorItemsInProjectTree( false );
            summaryCase->setDisplayNameOption( RimCaseDisplayNameTools::DisplayName::SHORT_CASE_NAME );
            summaryCase->updateAutoShortName();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::initAfterRead()
{
    RimSummaryEnsemble::initAfterRead();

    connectSignals();
    createSummaryCasesFromEnsembleFileSet();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::connectSignals()
{
    if ( m_ensembleFileSet() )
    {
        m_ensembleFileSet()->fileSetChanged.connect( this, &RimPathPatternEnsemble::onFileSetChanged );
        m_ensembleFileSet()->nameChanged.connect( this, &RimPathPatternEnsemble::onFileSetNameChanged );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_ensembleFileSet );

    if ( m_ensembleFileSet() )
    {
        auto group = uiOrdering.addNewGroup( "Ensemble Definition" );
        m_ensembleFileSet()->uiOrdering( uiConfigName, *group );
    }

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimSummaryEnsemble::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_ensembleFileSet )
    {
        connectSignals();
        createSummaryCasesFromEnsembleFileSet();
        caseNameChanged.send();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::onLoadDataAndUpdate()
{
    if ( m_cases.empty() ) createSummaryCasesFromEnsembleFileSet();

    RimSummaryEnsemble::onLoadDataAndUpdate();
}
