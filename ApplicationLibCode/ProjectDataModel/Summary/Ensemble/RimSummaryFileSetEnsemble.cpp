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

#include "RimSummaryFileSetEnsemble.h"

#include "Ensemble/RiaEnsembleImportTools.h"
#include "RiaFilePathTools.h"
#include "RiaLogging.h"
#include "Summary/RiaSummaryTools.h"

#include "EnsembleFileSet/RimEnsembleFileSet.h"
#include "EnsembleFileSet/RimEnsembleFileSetCollection.h"
#include "EnsembleFileSet/RimEnsembleFileSetTools.h"
#include "RimFileSummaryCase.h"
#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryEnsembleTools.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <map>

CAF_PDM_SOURCE_INIT( RimSummaryFileSetEnsemble, "RimSummaryFileSetEnsemble" );

namespace internal
{
const QString pathPatternPlaceholder = "*";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryFileSetEnsemble::RimSummaryFileSetEnsemble()
{
    CAF_PDM_InitObject( "Path Pattern Ensemble", ":/SummaryCase.svg" );

    CAF_PDM_InitFieldNoDefault( &m_ensembleFileSet, "EnsembleFileSet", "Ensemble File Set" );

    CAF_PDM_InitFieldNoDefault( &m_additionalSummaryFiles, "AdditionalSummaryFiles", "Additional Summary Files" );
    m_additionalSummaryFiles.uiCapability()->setUiHidden( true );

    m_cases.xmlCapability()->disableIO();

    setAsEnsemble( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleFileSet* RimSummaryFileSetEnsemble::ensembleFileSet()
{
    return m_ensembleFileSet();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::setEnsembleFileSet( RimEnsembleFileSet* ensembleFileSet )
{
    m_ensembleFileSet = ensembleFileSet;
    setGroupingMode( m_ensembleFileSet->groupingMode() );
    connectSignals();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::updateName( const std::set<QString>& existingEnsembleNames )
{
    QString candidateName = m_ensembleFileSet() ? m_ensembleFileSet()->name() : "Path Pattern Ensemble";

    if ( m_name == candidateName ) return;

    m_name = candidateName;
    caseNameChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::cleanupBeforeDelete()
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
void RimSummaryFileSetEnsemble::reloadCases()
{
    if ( m_ensembleFileSet() )
    {
        m_ensembleFileSet()->reload();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSummaryFileSetEnsemble::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    if ( fieldNeedingOptions == &m_ensembleFileSet )
    {
        return RimEnsembleFileSetTools::ensembleFileSetOptions();
    }
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::onFileSetChanged( const caf::SignalEmitter* emitter )
{
    bool notifyChange = true;
    createSummaryCasesFromEnsembleFileSet( notifyChange );
    RiaSummaryTools::updateConnectedPlots( this );
    buildChildNodes();
    updateAllRequiredEditors();

    RimSummaryEnsembleTools::updateDependentDeltaEnsembles( this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::onFileSetNameChanged( const caf::SignalEmitter* emitter )
{
    updateName( {} );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::createSummaryCasesFromEnsembleFileSet( bool notifyChange )
{
    // Keep user-defined summary vectors across a recreation of the cases
    storeAdditionalSummaryFiles();

    m_cases.deleteChildren();

    if ( m_ensembleFileSet() )
    {
        auto paths = m_ensembleFileSet()->createPaths( ".SMSPEC" );

        RiaDefines::FileType fileType = RiaDefines::FileType::SMSPEC;

        RiaEnsembleImportTools::CreateConfig createConfig{ .fileType              = fileType,
                                                           .ensembleOrGroup       = false,
                                                           .allowDialogs          = false,
                                                           .buildSummaryAddresses = false };

        auto newCases = RiaEnsembleImportTools::createSummaryCasesFromFiles( paths, createConfig );
        if ( newCases.empty() )
        {
            RiaLogging::warning( "No new cases are created." );
            return;
        }

        restoreAdditionalSummaryFiles( newCases );

        replaceCases( newCases, notifyChange );

        // Update name of cases and ensemble after all cases are added
        for ( auto summaryCase : newCases )
        {
            summaryCase->setShowTreeNodes( false );
            summaryCase->setDisplayNameOption( RimCaseDisplayNameTools::DisplayName::SHORT_CASE_NAME );
            summaryCase->updateAutoShortName();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::onProjectBeingSaved()
{
    storeAdditionalSummaryFiles();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::storeAdditionalSummaryFiles()
{
    if ( m_cases.empty() ) return;

    QStringList entries;
    for ( auto summaryCase : m_cases.childrenByType() )
    {
        if ( auto fileSummaryCase = dynamic_cast<RimFileSummaryCase*>( summaryCase ) )
        {
            QString additionalFile = fileSummaryCase->additionalSummaryFilePath();
            if ( !additionalFile.isEmpty() )
            {
                entries.push_back( fileSummaryCase->summaryHeaderFilename() + "|" + additionalFile );
            }
        }
    }

    m_additionalSummaryFiles = entries.join( '\n' );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::restoreAdditionalSummaryFiles( const std::vector<RimSummaryCase*>& summaryCases )
{
    if ( m_additionalSummaryFiles().isEmpty() ) return;

    std::map<QString, QString> additionalFileForHeader;
    for ( const auto& entry : m_additionalSummaryFiles().split( '\n', Qt::SkipEmptyParts ) )
    {
        // The additional file name is generated and does not contain separator characters
        auto separatorIndex = entry.lastIndexOf( '|' );
        if ( separatorIndex > 0 )
        {
            additionalFileForHeader[entry.left( separatorIndex )] = entry.mid( separatorIndex + 1 );
        }
    }

    QString projectFolder;
    if ( RimProject::current() && !RimProject::current()->fileName().isEmpty() )
    {
        projectFolder = QFileInfo( RimProject::current()->fileName() ).absolutePath();
    }

    for ( auto summaryCase : summaryCases )
    {
        auto fileSummaryCase = dynamic_cast<RimFileSummaryCase*>( summaryCase );
        if ( !fileSummaryCase ) continue;

        auto it = additionalFileForHeader.find( fileSummaryCase->summaryHeaderFilename() );
        if ( it == additionalFileForHeader.end() ) continue;

        QString filePath = it->second;
        if ( !QFile::exists( filePath ) && !projectFolder.isEmpty() )
        {
            // The project folder can have been moved
            filePath = QDir( projectFolder ).absoluteFilePath( QFileInfo( filePath ).fileName() );
        }

        fileSummaryCase->setAdditionalSummaryFilePath( filePath );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::createDerivedObjects()
{
    if ( m_cases.empty() )
    {
        bool notifyChange = false;
        createSummaryCasesFromEnsembleFileSet( notifyChange );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::initAfterRead()
{
    RimSummaryEnsemble::initAfterRead();

    connectSignals();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::connectSignals()
{
    if ( m_ensembleFileSet() )
    {
        m_ensembleFileSet()->fileSetChanged.connect( this, &RimSummaryFileSetEnsemble::onFileSetChanged );
        m_ensembleFileSet()->nameChanged.connect( this, &RimSummaryFileSetEnsemble::onFileSetNameChanged );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    if ( m_ensembleFileSet() )
    {
        auto group = uiOrdering.addNewGroup( "Ensemble Definition" );
        m_ensembleFileSet()->uiOrdering( uiConfigName, *group );
    }
    uiOrdering.add( &m_includeInAutoReload );

    auto developersGroup = uiOrdering.addNewGroup( " -- Developers --" );
    developersGroup->setCollapsedByDefault();
    developersGroup->add( &m_ensembleFileSet );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimSummaryEnsemble::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_ensembleFileSet )
    {
        connectSignals();
        bool notifyChange = true;
        createSummaryCasesFromEnsembleFileSet( notifyChange );
        RiaSummaryTools::updateConnectedPlots( this );
        caseNameChanged.send();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryFileSetEnsemble::onLoadDataAndUpdate()
{
    createDerivedObjects();

    RimSummaryEnsemble::onLoadDataAndUpdate();
}
