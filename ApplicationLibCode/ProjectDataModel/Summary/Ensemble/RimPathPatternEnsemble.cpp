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

#include "EnsembleFileset/RimEnsembleFileset.h"
#include "RiaLogging.h"
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
QList<caf::PdmOptionItemInfo> RimPathPatternEnsemble::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    if ( fieldNeedingOptions == &m_ensembleFileSet )
    {
        return RimEnsembleFileset::ensembleFilSetOptions();
    }
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::populatePathPattern()
{
    QStringList filePaths;

    for ( auto sumCase : allSummaryCases() )
    {
        const auto filePath = sumCase->summaryHeaderFilename();
        if ( filePath.isEmpty() ) continue;

        const auto fileName = RiaFilePathTools::toInternalSeparator( filePath );
        filePaths.push_back( fileName );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::onFilterChanged( const caf::SignalEmitter* emitter )
{
    createSummaryCasesFromEnsembleFileSet();
    buildChildNodes();
    updateAllRequiredEditors();
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
        auto [isOk, newCases] = RiaEnsembleImportTools::createSummaryCasesFromFiles( paths, createConfig );
        if ( !isOk || newCases.empty() )
        {
            RiaLogging::warning( "No new cases are created." );
            return;
        }

        replaceCases( newCases );

        // Update name of cases and ensemble after all cases are added
        for ( auto summaryCase : newCases )
        {
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

    if ( m_ensembleFileSet() )
    {
        m_ensembleFileSet()->fileSetChanged.connect( this, &RimPathPatternEnsemble::onFilterChanged );
        createSummaryCasesFromEnsembleFileSet();
    }

    populatePathPattern();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_ensembleFileSet );

    RimSummaryEnsemble::defineUiOrdering( uiConfigName, uiOrdering );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPathPatternEnsemble::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimSummaryEnsemble::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_ensembleFileSet )
    {
        if ( m_ensembleFileSet() )
        {
            m_ensembleFileSet()->fileSetChanged.connect( this, &RimPathPatternEnsemble::onFilterChanged );
        }
        createSummaryCasesFromEnsembleFileSet();
        caseNameChanged.send();
    }
}
