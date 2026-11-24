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

#include "RimCloudDataSourceCollection.h"

#include "RiaLogging.h"

#include "Cloud/RiaCloudApiService.h"
#include "RiaApplication.h"
#include "Summary/RiaSummaryPlotTools.h"
#include "Summary/RiaSummaryTools.h"

#include "Cloud/RimSummaryEnsembleSumo.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimReservoirGridEnsembleSumo.h"
#include "RimSummaryCaseMainCollection.h"
#include "RimSumoDataSource.h"

#include "RiuPlotMainWindow.h"
#include "RiuPlotMainWindowTools.h"
#include "RiuSumoEnsembleManagementDialog.h"
#include "RiuSumoEnsembleSelectionDialog.h"

#include "cafCmdFeatureMenuBuilder.h"

#include "cafPdmUiButton.h"
#include "cafPdmUiPropertyViewDialog.h"
#include "cafPdmUiPushButtonEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"
#include "cafSelectionManager.h"

#include <QDialogButtonBox>
#include <QMessageBox>

#include <set>

CAF_PDM_SOURCE_INIT( RimCloudDataSourceCollection, "RimCloudDataSourceCollection" );
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCloudDataSourceCollection::RimCloudDataSourceCollection()
{
    CAF_PDM_InitObject( "Cloud Data" + RiaDefines::betaFeaturePostfix(), ":/Cloud.svg" );

    CAF_PDM_InitFieldNoDefault( &m_sumoFieldName, "SumoFieldId", "Field Id" );
    CAF_PDM_InitFieldNoDefault( &m_sumoCaseId, "SumoCaseId", "Case Id" );
    m_sumoCaseId.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_sumoEnsembleNames, "SumoEnsembleNames", "Ensembles" );
    m_sumoEnsembleNames.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_sumoDataSources, "SumoDataSources", "Sumo Data Sources" );

    m_sumoConnector = RiaApplication::instance()->makeSumoConnector();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCloudDataSourceCollection* RimCloudDataSourceCollection::instance()
{
    return RimProject::current()->activeOilField()->cloudDataCollection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimSumoDataSource*> RimCloudDataSourceCollection::sumoDataSources() const
{
    return m_sumoDataSources.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::initAfterRead()
{
    refreshDataSourcesFromSumo();
}

//--------------------------------------------------------------------------------------------------
/// The realization ids, vector names and grid names of a data source are not written to the project file,
/// only the identity of the ensemble and the realization filter are. A data source read back from a project
/// therefore has none of them, which leaves the realization filter without the set it filters. Ask Sumo for
/// them again here, so the data source is as complete as one just added.
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::refreshDataSourcesFromSumo()
{
    if ( !m_sumoConnector ) return;

    for ( auto dataSource : sumoDataSources() )
    {
        if ( !dataSource ) continue;

        const auto caseId       = dataSource->caseId();
        const auto ensembleName = dataSource->ensembleName();
        if ( caseId.get().isEmpty() || ensembleName.isEmpty() ) continue;

        // Only what is missing is asked for, and an empty answer is not stored: a failed request must not
        // replace values that are already there. Note that Sumo answering with nothing is indistinguishable
        // from the request failing, so an ensemble genuinely without realizations is asked about again on the
        // next load. That costs a request and is preferable to recording "no realizations" after a failure.
        if ( !dataSource->hasFetchedRealizations() )
        {
            if ( const auto realizationIds = m_sumoConnector->explore().realizationIds( caseId, ensembleName ); !realizationIds.empty() )
            {
                dataSource->setAvailableRealizationIds( realizationIds );
            }
            else
            {
                // Without the realizations the realization filter has nothing to filter, and editing it
                // cannot be told from deselecting everything. Say so once, here, rather than leaving the
                // data source looking empty for no stated reason.
                RiaLogging::warning( QString( "Unable to load the realizations of Sumo ensemble '%1'. Editing the "
                                              "realization filter will have no effect until they are available." )
                                         .arg( ensembleName )
                                         .toStdString() );
                continue;
            }
        }

        if ( dataSource->vectorNames().empty() )
        {
            if ( const auto vectorNames = m_sumoConnector->summary().vectorNames( caseId, ensembleName ); !vectorNames.empty() )
            {
                dataSource->setVectorNames( vectorNames );
            }
        }

        if ( dataSource->gridNames().empty() )
        {
            if ( const auto gridNames = m_sumoConnector->grid().gridNames( caseId, ensembleName ); !gridNames.empty() )
            {
                dataSource->setGridNames( gridNames );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::createEnsemblesFromSelectedDataSources( const std::vector<RimSumoDataSource*>& dataSources )
{
    auto sumCaseMainColl = RiaSummaryTools::summaryCaseMainCollection();
    if ( !sumCaseMainColl ) return;

    for ( auto dataSource : dataSources )
    {
        RimSummaryEnsembleSumo* ensemble = new RimSummaryEnsembleSumo();
        ensemble->setUsePathKey1( true );
        ensemble->setSumoDataSource( dataSource );
        sumCaseMainColl->addEnsemble( ensemble );
        ensemble->loadDataAndUpdate();

        RiaSummaryPlotTools::createAndAppendDefaultSummaryMultiPlot( {}, { ensemble } );
    }

    RiaSummaryTools::updateSummaryEnsembleNames();
    sumCaseMainColl->updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( !m_sumoConnector ) return;

    if ( changedField == &m_sumoFieldName )
    {
        // What was picked below belonged to the asset that was just left, both the selection and the options
        // it was chosen from. Forget the cached answers as well, or the case list would keep offering the
        // cases of the previous asset. The editors are refreshed by the caller, which updates them as soon
        // as this returns, so asking for that here would only fetch everything a second time.
        m_sumoCaseId = "";
        m_sumoEnsembleNames.setValue( {} );

        clearCachedCases();
        clearCachedEnsembleNames();
    }
    else if ( changedField == &m_sumoCaseId )
    {
        m_sumoEnsembleNames.setValue( {} );

        clearCachedEnsembleNames();
    }
}

//--------------------------------------------------------------------------------------------------
/// True once the local service is serving requests. Being launched is not enough, as uvicorn may still
/// be booting.
//--------------------------------------------------------------------------------------------------
bool RimCloudDataSourceCollection::isCloudApiServerAvailable()
{
    auto* cloudApiService = RiaApplication::instance()->cloudApiService();
    return cloudApiService && cloudApiService->isResponding();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimCloudDataSourceCollection::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    if ( !m_sumoConnector || !m_sumoConnector->isGranted() ) return {};

    // The Sumo data is read through the local service, so requesting anything before it is up produces
    // failing requests against an empty server address.
    if ( !isCloudApiServerAvailable() ) return {};

    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_sumoFieldName )
    {
        for ( const auto& asset : cachedAssets() )
        {
            if ( m_sumoFieldName().isEmpty() )
            {
                m_sumoFieldName = asset.name;
            }

            options.push_back( { asset.name, asset.name } );
        }
    }
    else if ( fieldNeedingOptions == &m_sumoCaseId && !m_sumoFieldName().isEmpty() )
    {
        for ( const auto& sumoCase : cachedCases( m_sumoFieldName ) )
        {
            options.push_back( { sumoCase.name, sumoCase.caseId.get() } );
        }
    }
    else if ( fieldNeedingOptions == &m_sumoEnsembleNames && !m_sumoCaseId().isEmpty() )
    {
        for ( const auto& name : cachedEnsembleNames( SumoCaseId( m_sumoCaseId ) ) )
        {
            options.push_back( { name, name } );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<SumoAsset>& RimCloudDataSourceCollection::cachedAssets()
{
    if ( !m_exploreCache.assets() && m_sumoConnector )
    {
        // An empty answer is not stored, it cannot be told from a failed request
        if ( auto assets = m_sumoConnector->explore().assets(); !assets.empty() ) m_exploreCache.setAssets( assets );
    }

    static const std::vector<SumoAsset> empty;
    auto                                cached = m_exploreCache.assets();
    return cached ? *cached : empty;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<SumoCase>& RimCloudDataSourceCollection::cachedCases( const QString& assetName )
{
    if ( !m_exploreCache.cases( assetName ) && m_sumoConnector )
    {
        m_exploreCache.setCases( assetName, m_sumoConnector->explore().cases( assetName ) );
    }

    static const std::vector<SumoCase> empty;
    auto                               cached = m_exploreCache.cases( assetName );
    return cached ? *cached : empty;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<QString>& RimCloudDataSourceCollection::cachedEnsembleNames( const SumoCaseId& caseId )
{
    if ( !m_exploreCache.ensembleNames( caseId.get() ) && m_sumoConnector )
    {
        m_exploreCache.setEnsembleNames( caseId.get(), m_sumoConnector->explore().ensembleNames( caseId ) );
    }

    static const std::vector<QString> empty;
    auto                              cached = m_exploreCache.ensembleNames( caseId.get() );
    return cached ? *cached : empty;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaSumoExploreCache& RimCloudDataSourceCollection::exploreCache()
{
    return m_exploreCache;
}

//--------------------------------------------------------------------------------------------------
/// Forget the cases Sumo answered with, so the next request for them asks again.
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::clearCachedCases()
{
    m_exploreCache.clearCases();
}

//--------------------------------------------------------------------------------------------------
/// The same for the ensemble names, which belong to a case.
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::clearCachedEnsembleNames()
{
    m_exploreCache.clearEnsembleNames();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // Authentication group
    auto authGroup = uiOrdering.addNewGroup( "Authentication" );

    bool    isGranted = m_sumoConnector && m_sumoConnector->isGranted();
    QString text      = " Authentication Status: ";
    text += isGranted ? "<font color='#228B22'>✔ Granted</font>" : "<font color='#FFA500'>❌ Not Granted</font>";

    authGroup->addNewLabel( text, { .newRow = true, .totalColumnSpan = 1 } );
    auto* authButton = authGroup->addNewButton( "Authenticate",
                                                [this]()
                                                {
                                                    if ( !m_sumoConnector ) return;

                                                    // Authentication only, the server is started from its own button.
                                                    m_sumoConnector->requestTokenWithCancelButton();
                                                    updateConnectedEditors();
                                                },
                                                { .newRow = false, .totalColumnSpan = 1 } );
    authButton->setUiReadOnly( isGranted );
    // Cloud server group
    auto serverGroup = uiOrdering.addNewGroup( QString( "Cloud API Server %1" ).arg( RiaDefines::betaFeaturePostfix() ) );

    auto* cloudApiService = RiaApplication::instance()->cloudApiService();
    bool  isServerRunning = cloudApiService && cloudApiService->isRunning();

    // A launched process is not yet a working server: uvicorn may still be booting, or may be about
    // to exit on a missing dependency. Only report "Running" once a health check has succeeded.
    bool    isServerResponding = cloudApiService && cloudApiService->isResponding();
    QString serverStatus       = "Server Status: ";
    if ( isServerResponding )
    {
        serverStatus += QString( "<font color='#228B22'>✔ Running (port %1)</font>" ).arg( cloudApiService->port() );
    }
    else if ( isServerRunning )
    {
        serverStatus += QString( "<font color='#FFA500'>… Starting (port %1)</font>" ).arg( cloudApiService->port() );
    }
    else
    {
        serverStatus += "<font color='#FFA500'>❌ Stopped</font>";
    }

    // The status text is shown as a label ahead of the server buttons, mirroring the Authentication status above.
    serverGroup->addNewLabel( serverStatus );

    auto* startButton = serverGroup->addNewButton( "Start",
                                                   [this]()
                                                   {
                                                       // Authenticate up front, as the server is of little use without a token. The server
                                                       // is started regardless of the outcome, so it can still be force-started if
                                                       // authentication is cancelled.
                                                       if ( m_sumoConnector && !m_sumoConnector->isGranted() )
                                                       {
                                                           m_sumoConnector->requestTokenWithCancelButton();
                                                       }

                                                       RiaApplication::instance()->cloudApiService()->start();
                                                       updateConnectedEditors();
                                                   },
                                                   { .newRow = true, .totalColumnSpan = 1 } );
    startButton->setUiReadOnly( isServerRunning );

    auto* stopButton = serverGroup->addNewButton( "Stop",
                                                  [this]()
                                                  {
                                                      RiaApplication::instance()->cloudApiService()->stop();
                                                      updateConnectedEditors();
                                                  },
                                                  { .newRow = false, .totalColumnSpan = 1 } );
    stopButton->setUiReadOnly( !isServerRunning );

    auto* restartButton = serverGroup->addNewButton( "Restart",
                                                     [this]()
                                                     {
                                                         RiaApplication::instance()->cloudApiService()->restart();
                                                         updateConnectedEditors();
                                                     },
                                                     { .newRow = false, .totalColumnSpan = 1 } );
    restartButton->setUiReadOnly( !isServerRunning );

    // Cloud selector. Both a token and a running service are required to browse the Sumo content.
    if ( isGranted && isCloudApiServerAvailable() )
    {
        // The three selection fields are kept in the project file for one release, but are no longer shown.
        auto* manageButton = uiOrdering.addNewButton( "Manage Ensembles...", [this]() { manageEnsemblesInDialog(); } );
        manageButton->setUiToolTip( "Add, rename and remove SUMO ensembles" );
    }

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimSumoDataSource*> RimCloudDataSourceCollection::addDataSources()
{
    if ( !m_sumoConnector ) return {};

    std::vector<RimSumoDataSource*> dataSources;

    const auto sumoCaseId = SumoCaseId( m_sumoCaseId );

    QString caseName;
    for ( const auto& sumoCase : cachedCases( m_sumoFieldName ) )
    {
        if ( sumoCase.caseId == sumoCaseId )
        {
            caseName = sumoCase.name;
            break;
        }
    }

    for ( const auto& ensembleName : m_sumoEnsembleNames() )
    {
        if ( auto dataSource = addDataSource( m_sumoFieldName, sumoCaseId, caseName, ensembleName ) )
        {
            dataSources.push_back( dataSource );
        }
    }

    if ( !dataSources.empty() )
    {
        RiuPlotMainWindowTools::setExpanded( dataSources.back() );
        RiuPlotMainWindowTools::selectAsCurrentItem( dataSources.back() );
    }

    return dataSources;
}

//--------------------------------------------------------------------------------------------------
/// Creates a data source for the ensemble and fetches what it needs from Sumo. Returns nullptr if the
/// ensemble already has a data source, or if there is no connection.
//--------------------------------------------------------------------------------------------------
RimSumoDataSource* RimCloudDataSourceCollection::addDataSource( const QString&    assetName,
                                                                const SumoCaseId& caseId,
                                                                const QString&    caseName,
                                                                const QString&    ensembleName )
{
    if ( !m_sumoConnector ) return nullptr;

    if ( findDataSource( caseId, ensembleName ) ) return nullptr;

    const auto availableRealizationIds = m_sumoConnector->explore().realizationIds( caseId, ensembleName );
    const auto gridNames               = m_sumoConnector->grid().gridNames( caseId, ensembleName );
    const auto vectorNames             = m_sumoConnector->summary().vectorNames( caseId, ensembleName );

    auto dataSource = new RimSumoDataSource();
    dataSource->setCaseId( caseId );
    dataSource->setAssetName( assetName );
    dataSource->setCaseName( caseName );
    dataSource->setEnsembleName( ensembleName );
    dataSource->setAvailableRealizationIds( availableRealizationIds );
    dataSource->setVectorNames( vectorNames );
    dataSource->setGridNames( gridNames );

    if ( !gridNames.empty() )
    {
        dataSource->setSelectedGridName( gridNames.front() );
    }

    dataSource->updateName();

    m_sumoDataSources.push_back( dataSource );

    uiCapability()->updateAllRequiredEditors();

    return dataSource;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoDataSource* RimCloudDataSourceCollection::findDataSource( const SumoCaseId& caseId, const QString& ensembleName ) const
{
    for ( auto dataSource : sumoDataSources() )
    {
        if ( dataSource->caseId() == caseId && dataSource->ensembleName() == ensembleName ) return dataSource;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
/// The summary and grid ensembles that use the data source.
//--------------------------------------------------------------------------------------------------
std::vector<caf::PdmObject*> RimCloudDataSourceCollection::dependentEnsembles( const RimSumoDataSource* dataSource )
{
    std::vector<caf::PdmObject*> ensembles;
    if ( !dataSource ) return ensembles;

    for ( auto obj : dataSource->objectsWithReferringPtrFieldsOfType<RimSummaryEnsembleSumo>() )
        ensembles.push_back( obj );
    for ( auto obj : dataSource->objectsWithReferringPtrFieldsOfType<RimReservoirGridEnsembleSumo>() )
        ensembles.push_back( obj );

    return ensembles;
}

//--------------------------------------------------------------------------------------------------
/// Deletes the data source together with the summary and grid ensembles that use it.
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::removeDataSource( RimSumoDataSource* dataSource )
{
    if ( !dataSource ) return;

    const auto ensembles = dependentEnsembles( dataSource );

    std::vector<caf::PdmObjectHandle*> toDelete;
    for ( auto ensemble : ensembles )
    {
        caf::SelectionManager::instance()->removeObjectFromAllSelections( ensemble );

        if ( auto summaryEnsemble = dynamic_cast<RimSummaryEnsembleSumo*>( ensemble ) )
        {
            if ( auto mainColl = summaryEnsemble->firstAncestorOrThisOfType<RimSummaryCaseMainCollection>() )
            {
                mainColl->removeEnsemble( summaryEnsemble );
            }
            summaryEnsemble->cleanupBeforeDelete();
            toDelete.push_back( summaryEnsemble );
        }
        else if ( auto parentField = ensemble->parentField() )
        {
            parentField->removeChild( ensemble );
            toDelete.push_back( ensemble );
        }
    }

    caf::SelectionManager::instance()->removeObjectFromAllSelections( dataSource );
    m_sumoDataSources.removeChild( dataSource );

    // Update the tree while the detached objects are alive, then delete them
    RiaSummaryTools::updateSummaryEnsembleNames();
    if ( auto mainColl = RiaSummaryTools::summaryCaseMainCollection() ) mainColl->updateConnectedEditors();
    updateConnectedEditors();

    for ( auto obj : toDelete )
        delete obj;
    delete dataSource;
}

//--------------------------------------------------------------------------------------------------
/// Open the selection dialog. Ensembles unchecked in it are removed from the project together with what
/// uses them, and newly checked ones are added with a summary ensemble.
//--------------------------------------------------------------------------------------------------
bool RimCloudDataSourceCollection::selectEnsemblesInDialog()
{
    if ( !m_sumoConnector ) return false;

    std::set<RiuSumoCaseTableModel::CaseEnsemble> existing;
    for ( auto dataSource : sumoDataSources() )
    {
        existing.insert( { dataSource->caseId().get(), dataSource->ensembleName() } );
    }

    RiuSumoEnsembleSelectionDialog dialog( *m_sumoConnector, m_exploreCache, existing, RiuPlotMainWindow::instance() );
    if ( dialog.exec() != QDialog::Accepted ) return false;

    const auto toRemove = dialog.entriesToRemove();
    const auto toAdd    = dialog.entriesToAdd();

    if ( !toRemove.empty() )
    {
        QString text = "The following ensembles and the summary and grid ensembles using them will be deleted:\n";
        for ( const auto& entry : toRemove )
        {
            text += QString( "\n%1 / %2" ).arg( entry.caseName.isEmpty() ? entry.caseId : entry.caseName ).arg( entry.ensembleName );
        }

        if ( QMessageBox::question( RiuPlotMainWindow::instance(), "Remove Ensembles", text ) != QMessageBox::Yes ) return false;

        for ( const auto& entry : toRemove )
        {
            removeDataSource( findDataSource( SumoCaseId( entry.caseId ), entry.ensembleName ) );
        }
    }

    std::vector<RimSumoDataSource*> added;
    for ( const auto& entry : toAdd )
    {
        if ( auto dataSource = addDataSource( entry.assetName, SumoCaseId( entry.caseId ), entry.caseName, entry.ensembleName ) )
        {
            added.push_back( dataSource );
        }
    }

    createEnsemblesFromSelectedDataSources( added );

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::renameDataSource( RimSumoDataSource* dataSource, const QString& name )
{
    if ( !dataSource ) return;

    dataSource->setCustomName( name );

    RiaSummaryTools::updateSummaryEnsembleNames();
    if ( auto mainColl = RiaSummaryTools::summaryCaseMainCollection() ) mainColl->updateConnectedEditors();
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaSumoConnector* RimCloudDataSourceCollection::sumoConnector() const
{
    return m_sumoConnector;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::manageEnsemblesInDialog()
{
    RiuSumoEnsembleManagementDialog dialog( this, RiuPlotMainWindow::instance() );
    dialog.exec();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder.addCmdFeature( "RicSumoEnsembleManagementFeature" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::addEnsembles()
{
    auto dataSources = addDataSources();
    createEnsemblesFromSelectedDataSources( dataSources );
}
