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

#include "RimEclipseCase.h"

#include "cafPdmField.h"
#include "cafPdmPtrField.h"

#include <QPointer>
#include <QString>

#include <set>
#include <utility>
#include <vector>

class RiaSumoConnector;
class RimSumoDataSource;

//==================================================================================================
//
// Eclipse grid case backed by a roff grid stored on Sumo. The grid geometry is downloaded as a
// blob through RiaSumoConnector and parsed in memory, so there is no grid file on disk.
//
//==================================================================================================
class RimRoffCaseSumo : public RimEclipseCase
{
    CAF_PDM_HEADER_INIT;

public:
    RimRoffCaseSumo();
    ~RimRoffCaseSumo() override;

    // Create a grid case for a single realization of the given grid, linked back to the data source
    // so the case can be updated when the data source realization filter changes.
    static RimRoffCaseSumo* createFromDataSource( RimSumoDataSource* dataSource, const QString& gridName, int realization );

    void setSumoDataSource( RimSumoDataSource* dataSource );
    void setSumoCaseId( const QString& sumoCaseId );
    void setEnsembleName( const QString& ensembleName );
    void setGridName( const QString& gridName );
    void setRealization( int realization );

    QString gridName() const;
    int     realization() const;

    bool openEclipseGridFile() override;

    QString locationOnDisc() const override;

protected:
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;

private:
    // Fetch the list of available grid properties from Sumo (static + single-timestamp; time intervals are skipped).
    void fetchAvailableProperties();
    // Download and import the roff data of every selected property that is not already loaded.
    void loadSelectedProperties();
    bool loadProperty( const QString& propertyName, const QString& isoDateOrInterval );

    static QString propertyKey( const QString& propertyName, const QString& isoDateOrInterval );
    static QString propertyLabel( const QString& propertyName, const QString& isoDateOrInterval );

private:
    caf::PdmPtrField<RimSumoDataSource*> m_sumoDataSource;
    caf::PdmField<QString>               m_sumoCaseId;
    caf::PdmField<QString>               m_ensembleName;
    caf::PdmField<QString>               m_gridName;
    caf::PdmField<int>                   m_realization;
    caf::PdmField<std::vector<QString>>  m_selectedProperties;

    // Transient (not persisted): the available properties fetched from Sumo, as (name, isoDateOrInterval) pairs,
    // and the keys of the properties already imported into the grid.
    std::vector<std::pair<QString, QString>> m_availableProperties;
    std::set<QString>                        m_loadedPropertyKeys;

    QPointer<RiaSumoConnector> m_sumoConnector;
};
