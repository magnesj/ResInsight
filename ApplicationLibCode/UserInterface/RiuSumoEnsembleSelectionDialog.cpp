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

#include "RiuSumoEnsembleSelectionDialog.h"

#include "Cloud/RiaSumoExplore.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuSumoEnsembleSelectionDialog::RiuSumoEnsembleSelectionDialog( RiaSumoExplore&                                      explore,
                                                                const std::set<RiuSumoCaseTableModel::CaseEnsemble>& existing,
                                                                QWidget*                                             parent )
    : QDialog( parent )
    , m_explore( explore )
    , m_updatingList( false )
{
    setWindowTitle( "Select Sumo Ensembles" );
    resize( 1000, 600 );

    m_model.setExistingEnsembles( existing );
    m_proxy.setSourceModel( &m_model );

    m_assetCombo       = new QComboBox;
    m_officialCheckBox = new QCheckBox( "Only official cases" );
    m_caseFilter       = new QLineEdit;
    m_caseFilter->setPlaceholderText( "Filter cases" );
    m_ensembleFilter = new QLineEdit;
    m_ensembleFilter->setPlaceholderText( "Filter ensembles" );

    m_caseView = new QTableView;
    m_caseView->setModel( &m_proxy );
    m_caseView->setSortingEnabled( true );
    m_caseView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_caseView->setSelectionMode( QAbstractItemView::SingleSelection );
    m_caseView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_caseView->horizontalHeader()->setStretchLastSection( true );
    m_caseView->verticalHeader()->hide();

    m_ensembleList = new QListWidget;

    auto* addAllButton    = new QPushButton( "Add all" );
    auto* removeAllButton = new QPushButton( "Remove all" );

    auto* buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    buttons->button( QDialogButtonBox::Ok )->setText( "Apply" );

    auto* topRow = new QHBoxLayout;
    topRow->addWidget( new QLabel( "Asset" ) );
    topRow->addWidget( m_assetCombo );
    topRow->addWidget( m_officialCheckBox );
    topRow->addWidget( m_caseFilter, 1 );

    auto* ensembleButtons = new QHBoxLayout;
    ensembleButtons->addWidget( addAllButton );
    ensembleButtons->addWidget( removeAllButton );

    auto* ensembleColumn = new QVBoxLayout;
    ensembleColumn->addWidget( new QLabel( "Ensembles" ) );
    ensembleColumn->addWidget( m_ensembleFilter );
    ensembleColumn->addWidget( m_ensembleList );
    ensembleColumn->addLayout( ensembleButtons );

    auto* middle = new QHBoxLayout;
    middle->addWidget( m_caseView, 3 );
    middle->addLayout( ensembleColumn, 1 );

    auto* layout = new QVBoxLayout( this );
    layout->addLayout( topRow );
    layout->addLayout( middle );
    layout->addWidget( buttons );

    for ( const auto& asset : m_explore.assets() )
    {
        m_assetCombo->addItem( asset.name );
    }

    connect( m_assetCombo, &QComboBox::currentIndexChanged, this, &RiuSumoEnsembleSelectionDialog::assetChanged );
    connect( m_officialCheckBox, &QCheckBox::toggled, this, &RiuSumoEnsembleSelectionDialog::applyFilters );
    connect( m_caseFilter, &QLineEdit::textChanged, this, &RiuSumoEnsembleSelectionDialog::applyFilters );
    connect( m_ensembleFilter, &QLineEdit::textChanged, this, &RiuSumoEnsembleSelectionDialog::currentCaseChanged );
    connect( m_caseView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &RiuSumoEnsembleSelectionDialog::currentCaseChanged );
    connect( m_ensembleList, &QListWidget::itemChanged, this, &RiuSumoEnsembleSelectionDialog::ensembleItemChanged );
    connect( addAllButton, &QPushButton::clicked, this, [this]() { setAllEnsembles( true ); } );
    connect( removeAllButton, &QPushButton::clicked, this, [this]() { setAllEnsembles( false ); } );
    connect( buttons, &QDialogButtonBox::accepted, this, &QDialog::accept );
    connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::reject );

    if ( m_assetCombo->count() > 0 ) assetChanged();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::assetChanged()
{
    const QString assetName = m_assetCombo->currentText();
    const auto    cases     = m_explore.cases( assetName );

    for ( const auto& sumoCase : cases )
    {
        m_caseInfo[sumoCase.caseId.get()] = { assetName, sumoCase.name };
    }

    m_model.setCases( cases );
    currentCaseChanged();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuSumoEnsembleSelectionDialog::currentCaseId() const
{
    const auto current = m_caseView->selectionModel()->currentIndex();
    if ( !current.isValid() ) return {};

    return m_model.caseAt( m_proxy.mapToSource( current ).row() ).caseId.get();
}

//--------------------------------------------------------------------------------------------------
/// Show the ensembles of the current case. Names that did not come with the case list are asked for once.
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::currentCaseChanged()
{
    m_updatingList = true;
    m_ensembleList->clear();

    const QString caseId = currentCaseId();
    if ( !caseId.isEmpty() )
    {
        const auto current = m_proxy.mapToSource( m_caseView->selectionModel()->currentIndex() );

        std::vector<QString> names = m_model.caseAt( current.row() ).ensembleNames;
        if ( names.empty() )
        {
            auto it = m_fetchedEnsembleNames.find( caseId );
            if ( it == m_fetchedEnsembleNames.end() )
            {
                it = m_fetchedEnsembleNames.emplace( caseId, m_explore.ensembleNames( SumoCaseId( caseId ) ) ).first;
            }
            names = it->second;
        }

        // Selected ensembles the case list does not know of are kept visible, so they can be deselected
        for ( const auto& name : m_model.selectedEnsembles( caseId ) )
        {
            if ( std::find( names.begin(), names.end(), name ) == names.end() ) names.push_back( name );
        }

        const QString filter = m_ensembleFilter->text();
        for ( const auto& name : names )
        {
            if ( !filter.isEmpty() && !name.contains( filter, Qt::CaseInsensitive ) ) continue;

            auto* item = new QListWidgetItem( name, m_ensembleList );
            item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
            item->setCheckState( m_model.isEnsembleSelected( caseId, name ) ? Qt::Checked : Qt::Unchecked );
        }
    }

    m_updatingList = false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::ensembleItemChanged( QListWidgetItem* item )
{
    if ( m_updatingList || !item ) return;

    m_model.setEnsembleSelected( currentCaseId(), item->text(), item->checkState() == Qt::Checked );
}

//--------------------------------------------------------------------------------------------------
/// Applies to the ensembles shown, so the ensemble filter limits what Add all and Remove all affect.
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::setAllEnsembles( bool selected )
{
    for ( int i = 0; i < m_ensembleList->count(); i++ )
    {
        m_ensembleList->item( i )->setCheckState( selected ? Qt::Checked : Qt::Unchecked );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::applyFilters()
{
    m_proxy.setColumnFilter( RiuSumoCaseTableModel::CaseName, m_caseFilter->text() );
    m_proxy.setOnlyOfficial( m_officialCheckBox->isChecked() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiuSumoEnsembleSelectionDialog::Entry>
    RiuSumoEnsembleSelectionDialog::toEntries( const std::vector<RiuSumoCaseTableModel::CaseEnsemble>& keys ) const
{
    std::vector<Entry> entries;
    for ( const auto& [caseId, ensembleName] : keys )
    {
        Entry entry;
        entry.caseId       = caseId;
        entry.ensembleName = ensembleName;

        auto it = m_caseInfo.find( caseId );
        if ( it != m_caseInfo.end() )
        {
            entry.assetName = it->second.first;
            entry.caseName  = it->second.second;
        }
        entries.push_back( entry );
    }
    return entries;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiuSumoEnsembleSelectionDialog::Entry> RiuSumoEnsembleSelectionDialog::entriesToAdd() const
{
    return toEntries( m_model.selectionDiff().toAdd );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiuSumoEnsembleSelectionDialog::Entry> RiuSumoEnsembleSelectionDialog::entriesToRemove() const
{
    return toEntries( m_model.selectionDiff().toRemove );
}
