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

#pragma once

#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include "Cloud/RiaSumoConnector.h"
#include "Cloud/RiaSumoExploreCache.h"

#include <QPointer>
#include <QString>

class RimSumoDataSource;
namespace caf
{
class CmdFeatureMenuBuilder;
}

//==================================================================================================
///
///
//==================================================================================================
class RimCloudDataSourceCollection : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimCloudDataSourceCollection();

    static RimCloudDataSourceCollection* instance();

    std::vector<RimSumoDataSource*> sumoDataSources() const;

    RimSumoDataSource* findDataSource( const SumoCaseId& caseId, const QString& ensembleName ) const;

    // Creates a data source for one ensemble. Returns nullptr if it already exists or there is no connection.
    RimSumoDataSource* addDataSource( const QString& assetName, const SumoCaseId& caseId, const QString& caseName, const QString& ensembleName );

    // The summary and grid ensembles that use the data source, and so are deleted with it.
    static std::vector<caf::PdmObject*> dependentEnsembles( const RimSumoDataSource* dataSource );

    // Deletes the data source and the ensembles that use it.
    void removeDataSource( RimSumoDataSource* dataSource );

    // Gives the data source a name of its own, empty for the generated one, and updates what shows it.
    void renameDataSource( RimSumoDataSource* dataSource, const QString& name );

    // Opens the ensemble selection dialog and applies what is picked. Returns true if it was applied.
    bool selectEnsemblesInDialog();

    RiaSumoConnector* sumoConnector() const;

    // Opens the dialog listing the data sources, for adding, renaming and removing them.
    void manageEnsemblesInDialog();

    // What Sumo answered, shared with the selection dialog
    RiaSumoExploreCache& exploreCache();

    static void createEnsemblesFromSelectedDataSources( const std::vector<RimSumoDataSource*>& dataSources );

private:
    void initAfterRead() override;

    // Fetch the realization ids, vector names and grid names of the data sources that do not have them. They
    // are not written to the project file, so a data source restored from one starts out without them.
    void refreshDataSourcesFromSumo();

    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void                          appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const override;

    std::vector<RimSumoDataSource*> addDataSources();
    void                            addEnsembles();

    static bool isCloudApiServerAvailable();

    // The option lists are rebuilt every time the property editor refreshes, so what Sumo answered is kept
    // here and only asked for again when the selection it belongs to changes.
    const std::vector<SumoAsset>& cachedAssets();
    const std::vector<SumoCase>&  cachedCases( const QString& assetName );
    const std::vector<QString>&   cachedEnsembleNames( const SumoCaseId& caseId );

    void clearCachedCases();

    void clearCachedEnsembleNames();

private:
    caf::PdmField<QString>              m_sumoFieldName;
    caf::PdmField<QString>              m_sumoCaseId;
    caf::PdmField<std::vector<QString>> m_sumoEnsembleNames;

    caf::PdmChildArrayField<RimSumoDataSource*> m_sumoDataSources;

    QPointer<RiaSumoConnector> m_sumoConnector;

    RiaSumoExploreCache m_exploreCache;
};
