/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

#include "RiuSumoEnsembleManagementDialog.h"

#include "Cloud/RimCloudDataSourceCollection.h"
#include "Cloud/RimSumoDataSource.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
enum Column
{
    ColName,
    ColAsset,
    ColCase,
    ColEnsemble,
    ColRealizations,
    ColCount
};
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuSumoEnsembleManagementDialog::RiuSumoEnsembleManagementDialog( RimCloudDataSourceCollection* collection, QWidget* parent )
    : QDialog( parent )
    , m_collection( collection )
{
    setWindowTitle( "Manage SUMO Ensembles" );
    resize( 900, 400 );

    auto* layout = new QVBoxLayout( this );

    m_table = new QTableWidget( 0, ColCount, this );
    m_table->setHorizontalHeaderLabels( { "Name", "Asset", "Case", "Ensemble", "Realizations" } );
    m_table->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_table->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_table->horizontalHeader()->setStretchLastSection( true );
    m_table->horizontalHeader()->setSectionResizeMode( ColName, QHeaderView::Stretch );
    layout->addWidget( m_table );

    auto* buttonRow = new QHBoxLayout();
    auto* addButton = new QPushButton( "Add...", this );
    m_removeButton  = new QPushButton( "Remove", this );
    buttonRow->addWidget( addButton );
    buttonRow->addWidget( m_removeButton );
    buttonRow->addStretch();
    layout->addLayout( buttonRow );

    auto* buttons = new QDialogButtonBox( QDialogButtonBox::Close, this );
    layout->addWidget( buttons );

    connect( addButton, &QPushButton::clicked, this, &RiuSumoEnsembleManagementDialog::onAddClicked );
    connect( m_removeButton, &QPushButton::clicked, this, &RiuSumoEnsembleManagementDialog::onRemoveClicked );
    connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::reject );
    connect( m_table, &QTableWidget::itemChanged, this, &RiuSumoEnsembleManagementDialog::onItemChanged );

    populate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleManagementDialog::populate()
{
    QSignalBlocker blocker( m_table );

    const auto dataSources = m_collection->sumoDataSources();
    m_table->setRowCount( static_cast<int>( dataSources.size() ) );

    for ( int row = 0; row < static_cast<int>( dataSources.size() ); row++ )
    {
        auto* ds = dataSources[row];

        auto* nameItem = new QTableWidgetItem( ds->name() );
        nameItem->setData( Qt::UserRole, QVariant::fromValue( reinterpret_cast<quintptr>( ds ) ) );
        m_table->setItem( row, ColName, nameItem );

        auto addReadOnly = [&]( int column, const QString& text )
        {
            auto* item = new QTableWidgetItem( text );
            item->setFlags( item->flags() & ~Qt::ItemIsEditable );
            m_table->setItem( row, column, item );
        };

        addReadOnly( ColAsset, ds->assetName() );
        addReadOnly( ColCase, ds->caseName() );
        addReadOnly( ColEnsemble, ds->ensembleName() );
        addReadOnly( ColRealizations, ds->hasFetchedRealizations() ? QString::number( ds->availableRealizationIds().size() ) : QString( "-" ) );
    }

    m_removeButton->setEnabled( !dataSources.empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoDataSource* RiuSumoEnsembleManagementDialog::dataSourceForRow( int row ) const
{
    auto* item = m_table->item( row, ColName );
    if ( !item ) return nullptr;

    // Resolve through the collection, so a pointer to something deleted since is never used
    const auto ptr = item->data( Qt::UserRole ).value<quintptr>();
    for ( auto* ds : m_collection->sumoDataSources() )
    {
        if ( reinterpret_cast<quintptr>( ds ) == ptr ) return ds;
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleManagementDialog::onAddClicked()
{
    if ( m_collection->selectEnsemblesInDialog() ) populate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleManagementDialog::onRemoveClicked()
{
    std::vector<RimSumoDataSource*> toRemove;
    for ( const auto& range : m_table->selectedRanges() )
    {
        for ( int row = range.topRow(); row <= range.bottomRow(); row++ )
        {
            if ( auto* ds = dataSourceForRow( row ) ) toRemove.push_back( ds );
        }
    }
    if ( toRemove.empty() ) return;

    QString text = QString( "Remove %1 ensemble(s)?" ).arg( toRemove.size() );
    for ( auto* ds : toRemove )
    {
        const auto dependents = RimCloudDataSourceCollection::dependentEnsembles( ds );
        if ( !dependents.empty() )
            text += QString( "\n%1: %2 dependent ensemble(s) will also be deleted" ).arg( ds->name() ).arg( dependents.size() );
    }

    if ( QMessageBox::question( this, "Remove Ensembles", text ) != QMessageBox::Yes ) return;

    for ( auto* ds : toRemove )
    {
        m_collection->removeDataSource( ds );
    }
    populate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleManagementDialog::onItemChanged( QTableWidgetItem* item )
{
    if ( !item || item->column() != ColName ) return;

    if ( auto* ds = dataSourceForRow( item->row() ) )
    {
        m_collection->renameDataSource( ds, item->text() );

        QSignalBlocker blocker( m_table );
        item->setText( ds->name() );
    }
}
