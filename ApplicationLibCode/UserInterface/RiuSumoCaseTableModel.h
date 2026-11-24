/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025- Equinor ASA
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

#include "Cloud/RiaSumoExplore.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <set>
#include <utility>
#include <vector>

//==================================================================================================
/// The cases of an asset as a table, together with the ensembles picked per case. The selection is kept
/// by case id, so it survives a change of the case being shown, and the model has no widget dependency.
//==================================================================================================
class RiuSumoCaseTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        Selection,
        CaseName,
        Description,
        Author,
        Status,
        Date,
        Model,
        Revision,
        ColumnCount
    };

    using CaseEnsemble = std::pair<QString, QString>; // case id, ensemble name

    struct SelectionDiff
    {
        std::vector<CaseEnsemble> toAdd;
        std::vector<CaseEnsemble> toRemove;
    };

    explicit RiuSumoCaseTableModel( QObject* parent = nullptr );

    void                         setCases( const std::vector<SumoCase>& cases );
    const SumoCase&              caseAt( int row ) const;
    const std::vector<SumoCase>& cases() const;

    // The ensembles already in the project, which are the starting point of the selection
    void setExistingEnsembles( const std::set<CaseEnsemble>& existing );

    void              setEnsembleSelected( const QString& caseId, const QString& ensembleName, bool selected );
    bool              isEnsembleSelected( const QString& caseId, const QString& ensembleName ) const;
    std::set<QString> selectedEnsembles( const QString& caseId ) const;
    SelectionDiff     selectionDiff() const;

    int      rowCount( const QModelIndex& parent = QModelIndex() ) const override;
    int      columnCount( const QModelIndex& parent = QModelIndex() ) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    static QString columnText( const SumoCase& sumoCase, int column );

private:
    std::vector<SumoCase>  m_cases;
    std::set<CaseEnsemble> m_existing;
    std::set<CaseEnsemble> m_selected;
};

//==================================================================================================
/// Text filter per column, multi-select for status, model and revision, and the my cases and official
/// cases toggles.
//==================================================================================================
class RiuSumoCaseFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit RiuSumoCaseFilterProxyModel( QObject* parent = nullptr );

    void setColumnFilter( int column, const QString& text );
    void setAllowedValues( int column, const std::set<QString>& values ); // empty means all
    void setOnlyUser( const QString& userName ); // empty means off
    void setOnlyOfficial( bool onlyOfficial );

    static constexpr const char* officialStatus = "keep";

protected:
    bool filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const override;

private:
    std::map<int, QString>           m_columnFilters;
    std::map<int, std::set<QString>> m_allowedValues;
    QString                          m_onlyUser;
    bool                             m_onlyOfficial;
};
