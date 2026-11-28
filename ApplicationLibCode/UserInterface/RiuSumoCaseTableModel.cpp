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

#include "RiuSumoCaseTableModel.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuSumoCaseTableModel::RiuSumoCaseTableModel( QObject* parent )
    : QAbstractTableModel( parent )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseTableModel::setCases( const std::vector<SumoCase>& cases )
{
    beginResetModel();
    m_cases = cases;
    endResetModel();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const SumoCase& RiuSumoCaseTableModel::caseAt( int row ) const
{
    return m_cases.at( row );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<SumoCase>& RiuSumoCaseTableModel::cases() const
{
    return m_cases;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseTableModel::setExistingEnsembles( const std::set<CaseEnsemble>& existing )
{
    m_existing = existing;
    m_selected = existing;

    if ( rowCount() > 0 ) emit dataChanged( index( 0, Selection ), index( rowCount() - 1, Selection ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseTableModel::setEnsembleSelected( const QString& caseId, const QString& ensembleName, bool selected )
{
    const CaseEnsemble key( caseId, ensembleName );
    if ( selected )
        m_selected.insert( key );
    else
        m_selected.erase( key );

    for ( int row = 0; row < rowCount(); row++ )
    {
        if ( m_cases[row].caseId.get() == caseId ) emit dataChanged( index( row, Selection ), index( row, Selection ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiuSumoCaseTableModel::isEnsembleSelected( const QString& caseId, const QString& ensembleName ) const
{
    return m_selected.count( CaseEnsemble( caseId, ensembleName ) ) > 0;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::set<QString> RiuSumoCaseTableModel::selectedEnsembles( const QString& caseId ) const
{
    std::set<QString> names;
    for ( const auto& [id, name] : m_selected )
    {
        if ( id == caseId ) names.insert( name );
    }
    return names;
}

//--------------------------------------------------------------------------------------------------
/// What Apply has to do: ensembles selected but not in the project are added, and ensembles in the
/// project but no longer selected are removed.
//--------------------------------------------------------------------------------------------------
RiuSumoCaseTableModel::SelectionDiff RiuSumoCaseTableModel::selectionDiff() const
{
    SelectionDiff diff;
    for ( const auto& key : m_selected )
    {
        if ( !m_existing.count( key ) ) diff.toAdd.push_back( key );
    }
    for ( const auto& key : m_existing )
    {
        if ( !m_selected.count( key ) ) diff.toRemove.push_back( key );
    }
    return diff;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiuSumoCaseTableModel::rowCount( const QModelIndex& parent ) const
{
    return parent.isValid() ? 0 : static_cast<int>( m_cases.size() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiuSumoCaseTableModel::columnCount( const QModelIndex& parent ) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

//--------------------------------------------------------------------------------------------------
/// The selection column shows selected/total, and is empty for the total until the ensemble names of
/// the case are known.
//--------------------------------------------------------------------------------------------------
QString RiuSumoCaseTableModel::columnText( const SumoCase& sumoCase, int column )
{
    switch ( column )
    {
        case CaseName:
            return sumoCase.name;
        case Description:
            return sumoCase.description;
        case Author:
            return sumoCase.author;
        case Status:
            return sumoCase.status;
        case Date:
            return sumoCase.date;
        case Model:
            return sumoCase.modelName;
        case Revision:
            return sumoCase.modelRevision;
        default:
            return {};
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QVariant RiuSumoCaseTableModel::data( const QModelIndex& idx, int role ) const
{
    if ( !idx.isValid() || idx.row() >= rowCount() ) return {};

    const auto& sumoCase = m_cases[idx.row()];

    if ( idx.column() == Selection )
    {
        if ( role == Qt::DisplayRole )
        {
            const auto selected = selectedEnsembles( sumoCase.caseId.get() ).size();
            const auto total    = sumoCase.ensembleNames.size();
            return total > 0 ? QString( "%1/%2" ).arg( selected ).arg( total ) : QString::number( selected );
        }
        if ( role == Qt::EditRole ) return static_cast<qulonglong>( selectedEnsembles( sumoCase.caseId.get() ).size() );
        return {};
    }

    if ( role == Qt::DisplayRole || role == Qt::EditRole ) return columnText( sumoCase, idx.column() );
    if ( role == Qt::ToolTipRole && idx.column() == CaseName ) return sumoCase.caseId.get();

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QVariant RiuSumoCaseTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( orientation != Qt::Horizontal || role != Qt::DisplayRole ) return {};

    static const char* headers[] = { "", "Case", "Description", "Author", "Status", "Date", "Model", "Revision" };
    return section >= 0 && section < ColumnCount ? QString( headers[section] ) : QVariant();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuSumoCaseFilterProxyModel::RiuSumoCaseFilterProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , m_onlyOfficial( false )
{
    setSortRole( Qt::EditRole );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseFilterProxyModel::setColumnFilter( int column, const QString& text )
{
    if ( text.isEmpty() )
        m_columnFilters.erase( column );
    else
        m_columnFilters[column] = text;

    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseFilterProxyModel::setAllowedValues( int column, const std::set<QString>& values )
{
    if ( values.empty() )
        m_allowedValues.erase( column );
    else
        m_allowedValues[column] = values;

    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseFilterProxyModel::setOnlyUser( const QString& userName )
{
    m_onlyUser = userName;
    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoCaseFilterProxyModel::setOnlyOfficial( bool onlyOfficial )
{
    m_onlyOfficial = onlyOfficial;
    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiuSumoCaseFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    auto* source = qobject_cast<RiuSumoCaseTableModel*>( sourceModel() );
    if ( !source || sourceRow < 0 || sourceRow >= source->rowCount( sourceParent ) ) return true;

    const auto& sumoCase = source->caseAt( sourceRow );

    if ( !m_onlyUser.isEmpty() && sumoCase.author.compare( m_onlyUser, Qt::CaseInsensitive ) != 0 ) return false;
    if ( m_onlyOfficial && sumoCase.status.compare( officialStatus, Qt::CaseInsensitive ) != 0 ) return false;

    for ( const auto& [column, text] : m_columnFilters )
    {
        if ( !RiuSumoCaseTableModel::columnText( sumoCase, column ).contains( text, Qt::CaseInsensitive ) ) return false;
    }

    for ( const auto& [column, values] : m_allowedValues )
    {
        if ( !values.count( RiuSumoCaseTableModel::columnText( sumoCase, column ) ) ) return false;
    }

    return true;
}
