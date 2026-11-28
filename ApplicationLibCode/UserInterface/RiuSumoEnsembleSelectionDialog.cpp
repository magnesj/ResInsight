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

#include "Cloud/RiaSumoConnector.h"
#include "Cloud/RiaSumoExplore.h"
#include "Cloud/RiaSumoExploreCache.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QScrollBar>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuSumoEnsembleSelectionDialog::RiuSumoEnsembleSelectionDialog( RiaSumoConnector&                                    connector,
                                                                RiaSumoExploreCache&                                 cache,
                                                                const std::set<RiuSumoCaseTableModel::CaseEnsemble>& existing,
                                                                QWidget*                                             parent )
    : QDialog( parent )
    , m_connector( connector )
    , m_cache( cache )
    , m_pendingRequests( 0 )
    , m_updatingList( false )
{
    setWindowTitle( "Select Sumo Ensembles" );
    const auto* screen = parent ? parent->screen() : QGuiApplication::primaryScreen();
    const int   width  = screen ? static_cast<int>( screen->availableGeometry().width() * 0.8 ) : 1000;
    resize( width, 600 );

    m_model.setExistingEnsembles( existing );
    m_proxy.setSourceModel( &m_model );

    m_assetCombo = new QComboBox;
    m_assetCombo->setMinimumWidth( 220 );
    m_assetCombo->setSizeAdjustPolicy( QComboBox::AdjustToContents );
    m_myCasesCheckBox = new QCheckBox( "Only my cases" );
    m_myCasesCheckBox->setEnabled( false );
    m_myCasesCheckBox->setToolTip( "Available when the signed-in user is known to the cloud service" );
    m_officialCheckBox = new QCheckBox( "Only official cases" );
    m_refreshButton    = new QPushButton( "Refresh" );
    m_statusLabel      = new QLabel;
    m_ensembleFilter   = new QLineEdit;
    m_ensembleFilter->setPlaceholderText( "Filter ensembles" );

    m_caseView = new QTableView;
    m_caseView->setModel( &m_proxy );
    m_caseView->setSortingEnabled( true );
    m_caseView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_caseView->setSelectionMode( QAbstractItemView::SingleSelection );
    m_caseView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    m_caseView->horizontalHeader()->setStretchLastSection( true );
    m_caseView->setColumnWidth( RiuSumoCaseTableModel::CaseName, 450 );
    m_caseView->verticalHeader()->hide();
    m_caseView->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background-color: palette(midlight); padding: 4px; border: none; "
        "border-right: 1px solid palette(mid); }" );

    m_ensembleList = new QListWidget;

    auto* addAllButton    = new QPushButton( "Add all" );
    auto* removeAllButton = new QPushButton( "Remove all" );

    auto* buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    buttons->button( QDialogButtonBox::Ok )->setText( "Apply" );

    auto* topRow = new QHBoxLayout;
    topRow->addWidget( new QLabel( "Asset" ) );
    topRow->addWidget( m_assetCombo );
    topRow->addWidget( m_myCasesCheckBox );
    topRow->addWidget( m_officialCheckBox );
    topRow->addStretch( 1 );
    topRow->addWidget( m_refreshButton );

    auto* ensembleButtons = new QHBoxLayout;
    ensembleButtons->addWidget( addAllButton );
    ensembleButtons->addWidget( removeAllButton );

    auto* ensembleColumn = new QVBoxLayout;
    ensembleColumn->addWidget( new QLabel( "Ensembles" ) );
    ensembleColumn->addWidget( m_ensembleFilter );
    ensembleColumn->addWidget( m_ensembleList );
    ensembleColumn->addLayout( ensembleButtons );

    createFilterRow();

    auto* caseColumn = new QVBoxLayout;
    caseColumn->setSpacing( 0 );
    caseColumn->addWidget( m_filterRow );
    caseColumn->addWidget( m_caseView );

    auto* middle = new QHBoxLayout;
    middle->addLayout( caseColumn, 3 );
    middle->addLayout( ensembleColumn, 1 );

    auto* layout = new QVBoxLayout( this );
    layout->addLayout( topRow );
    layout->addLayout( middle );
    layout->addWidget( m_statusLabel );
    layout->addWidget( buttons );

    connect( m_assetCombo, &QComboBox::currentIndexChanged, this, &RiuSumoEnsembleSelectionDialog::assetChanged );
    connect( m_myCasesCheckBox, &QCheckBox::toggled, this, &RiuSumoEnsembleSelectionDialog::applyFilters );
    connect( m_officialCheckBox, &QCheckBox::toggled, this, &RiuSumoEnsembleSelectionDialog::applyFilters );
    connect( m_refreshButton, &QPushButton::clicked, this, &RiuSumoEnsembleSelectionDialog::refresh );
    connect( m_ensembleFilter, &QLineEdit::textChanged, this, &RiuSumoEnsembleSelectionDialog::currentCaseChanged );
    connect( m_caseView->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &RiuSumoEnsembleSelectionDialog::currentCaseChanged );
    connect( m_ensembleList, &QListWidget::itemChanged, this, &RiuSumoEnsembleSelectionDialog::ensembleItemChanged );
    connect( addAllButton, &QPushButton::clicked, this, [this]() { setAllEnsembles( true ); } );
    connect( removeAllButton, &QPushButton::clicked, this, [this]() { setAllEnsembles( false ); } );
    connect( buttons, &QDialogButtonBox::accepted, this, &QDialog::accept );
    connect( buttons, &QDialogButtonBox::rejected, this, &QDialog::reject );

    loadAssets();

    // The user name comes separately and only enables the 'my cases' filter, so it never holds up the lists
    if ( m_cache.userName().isEmpty() )
    {
        QPointer<RiuSumoEnsembleSelectionDialog> self( this );
        m_connector.explore().userNameAsync(
            [self]( const QString& userName )
            {
                if ( !self || userName.isEmpty() ) return;
                self->m_cache.setUserName( userName );
                self->m_myCasesCheckBox->setEnabled( true );
            },
            this );
    }
    else
    {
        m_myCasesCheckBox->setEnabled( true );
    }
}

//--------------------------------------------------------------------------------------------------
/// Abort what is still in flight, as the results have nowhere to go once the dialog is closed
//--------------------------------------------------------------------------------------------------
RiuSumoEnsembleSelectionDialog::~RiuSumoEnsembleSelectionDialog()
{
    m_connector.cancelGroup( this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::updateStatus()
{
    QString text;
    if ( m_pendingRequests > 0 )
    {
        text = "Loading from Sumo...";
    }
    else if ( m_assetCombo->count() == 0 )
    {
        text = "No assets received. Check that you are authenticated and that the cloud service is running.";
    }
    else if ( auto updated = m_cache.casesUpdated( m_assetCombo->currentText() ) )
    {
        text = QString( "Last updated %1, %2 cases" ).arg( updated->toString( "yyyy-MM-dd hh:mm:ss" ) ).arg( m_model.rowCount() );
    }

    m_statusLabel->setText( text );
    m_refreshButton->setEnabled( m_pendingRequests == 0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::loadAssets()
{
    auto showAssets = [this]()
    {
        const auto* assets = m_cache.assets();
        if ( !assets ) return;

        m_assetCombo->blockSignals( true );
        m_assetCombo->clear();
        for ( const auto& asset : *assets )
            m_assetCombo->addItem( asset.name );
        m_assetCombo->blockSignals( false );

        if ( m_assetCombo->count() > 0 ) assetChanged();
    };

    if ( m_cache.assets() )
    {
        showAssets();
        updateStatus();
        return;
    }

    m_pendingRequests++;
    updateStatus();

    QPointer<RiuSumoEnsembleSelectionDialog> self( this );
    m_connector.explore().assetsAsync(
        [self, showAssets]( const std::vector<SumoAsset>& assets )
        {
            if ( !self ) return;
            self->m_pendingRequests--;
            if ( !assets.empty() ) self->m_cache.setAssets( assets );
            // showAssets captured the dialog, which is alive as self is
            showAssets();
            self->updateStatus();
        },
        this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::assetChanged()
{
    loadCases( false );
}

//--------------------------------------------------------------------------------------------------
/// Show the cases of the asset from the cache, or request them. Refreshing drops what is cached first.
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::loadCases( bool forceRefresh )
{
    const QString assetName = m_assetCombo->currentText();
    if ( assetName.isEmpty() ) return;

    if ( forceRefresh ) m_cache.clearCases( assetName );

    if ( m_cache.cases( assetName ) )
    {
        showCases( assetName );
        return;
    }

    // Switching asset while a request is out lets that one finish into the cache, and it is only shown if it
    // is still the asset in view
    m_model.setCases( {} );
    m_pendingRequests++;
    updateStatus();

    QPointer<RiuSumoEnsembleSelectionDialog> self( this );
    m_connector.explore().casesAsync(
        assetName,
        [self, assetName]( const std::vector<SumoCase>& cases )
        {
            if ( !self ) return;
            self->m_pendingRequests--;

            // An empty answer cannot be told from a failed request, so it is not cached
            if ( !cases.empty() ) self->m_cache.setCases( assetName, cases );

            if ( self->m_assetCombo->currentText() == assetName ) self->showCases( assetName );
            self->updateStatus();
        },
        this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::showCases( const QString& assetName )
{
    const auto* cases = m_cache.cases( assetName );
    if ( !cases ) return;

    for ( const auto& sumoCase : *cases )
    {
        m_caseInfo[sumoCase.caseId.get()] = { assetName, sumoCase.name };
    }

    m_model.setCases( *cases );
    updateValueFilters();
    updateStatus();
    currentCaseChanged();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::refresh()
{
    m_cache.clearEnsembleNames();
    loadCases( true );
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
/// Show the ensembles of the current case. Names that did not come with the case list are requested, and
/// the list is rebuilt when they arrive.
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
            if ( const auto* cached = m_cache.ensembleNames( caseId ) )
            {
                names = *cached;
            }
            else
            {
                m_pendingRequests++;
                updateStatus();

                QPointer<RiuSumoEnsembleSelectionDialog> self( this );
                m_connector.explore().ensembleNamesAsync(
                    SumoCaseId( caseId ),
                    [self, caseId]( const std::vector<QString>& ensembleNames )
                    {
                        if ( !self ) return;
                        self->m_pendingRequests--;
                        if ( !ensembleNames.empty() ) self->m_cache.setEnsembleNames( caseId, ensembleNames );
                        if ( self->currentCaseId() == caseId ) self->currentCaseChanged();
                        self->updateStatus();
                    },
                    this );
            }
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
    applyColumnFilters();
    m_proxy.setOnlyOfficial( m_officialCheckBox->isChecked() );
    m_proxy.setOnlyUser( m_myCasesCheckBox->isChecked() ? m_cache.userName() : QString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::applyColumnFilters()
{
    for ( const auto& [column, edit] : m_textFilters )
    {
        m_proxy.setColumnFilter( column, edit->text() );
    }
    for ( const auto& [column, values] : m_checkedValues )
    {
        m_proxy.setAllowedValues( column, values );
    }
}

//--------------------------------------------------------------------------------------------------
/// A filter row above the table, with a widget for each column kept aligned to the header sections
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::createFilterRow()
{
    m_filterRow = new QWidget;
    m_filterRow->setFixedHeight( 28 );

    for ( int column = RiuSumoCaseTableModel::CaseName; column < RiuSumoCaseTableModel::ColumnCount; column++ )
    {
        const bool isValueColumn = column == RiuSumoCaseTableModel::Status || column == RiuSumoCaseTableModel::Model ||
                                   column == RiuSumoCaseTableModel::Revision;
        if ( isValueColumn )
        {
            auto* button = new QToolButton( m_filterRow );
            button->setPopupMode( QToolButton::InstantPopup );
            button->setMenu( new QMenu( button ) );
            button->setText( "All" );
            m_valueFilters[column] = button;
        }
        else
        {
            auto* edit = new QLineEdit( m_filterRow );
            edit->setPlaceholderText( "Filter ..." );
            edit->setClearButtonEnabled( true );
            connect( edit, &QLineEdit::textChanged, this, &RiuSumoEnsembleSelectionDialog::applyFilters );
            m_textFilters[column] = edit;
        }
    }

    auto* header = m_caseView->horizontalHeader();
    connect( header, &QHeaderView::sectionResized, this, [this]() { layoutFilterRow(); } );
    connect( header, &QHeaderView::geometriesChanged, this, [this]() { layoutFilterRow(); } );
    connect( m_caseView->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this]() { layoutFilterRow(); } );
    header->installEventFilter( this );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::layoutFilterRow()
{
    auto* header = m_caseView->horizontalHeader();

    const int offset = m_caseView->viewport()->mapTo( this, QPoint( 0, 0 ) ).x() - m_filterRow->mapTo( this, QPoint( 0, 0 ) ).x();

    auto place = [&]( int column, QWidget* widget )
    {
        const int x = header->sectionViewportPosition( column ) + offset;
        widget->setGeometry( x, 2, header->sectionSize( column ), m_filterRow->height() - 4 );
        widget->setVisible( !header->isSectionHidden( column ) && x + header->sectionSize( column ) > offset );
    };

    for ( const auto& [column, edit] : m_textFilters )
        place( column, edit );
    for ( const auto& [column, button] : m_valueFilters )
        place( column, button );
}

//--------------------------------------------------------------------------------------------------
/// Offers the values found in the loaded cases, keeping the ones checked before when still present
//--------------------------------------------------------------------------------------------------
void RiuSumoEnsembleSelectionDialog::updateValueFilters()
{
    for ( const auto& [column, button] : m_valueFilters )
    {
        std::set<QString> values;
        for ( const auto& sumoCase : m_model.cases() )
        {
            const auto text = RiuSumoCaseTableModel::columnText( sumoCase, column );
            if ( !text.isEmpty() ) values.insert( text );
        }

        auto&             checked = m_checkedValues[column];
        std::set<QString> kept;
        for ( const auto& value : checked )
        {
            if ( values.count( value ) ) kept.insert( value );
        }
        checked = kept;

        auto* menu = button->menu();
        menu->clear();
        for ( const auto& value : values )
        {
            auto* action = menu->addAction( value );
            action->setCheckable( true );
            action->setChecked( checked.count( value ) > 0 );
            connect( action,
                     &QAction::toggled,
                     this,
                     [this, column, value, button]( bool on )
                     {
                         auto& set = m_checkedValues[column];
                         if ( on )
                             set.insert( value );
                         else
                             set.erase( value );
                         button->setText( set.empty() ? QString( "All" ) : QString( "%1 selected" ).arg( set.size() ) );
                         applyFilters();
                     } );
        }
        button->setText( checked.empty() ? QString( "All" ) : QString( "%1 selected" ).arg( checked.size() ) );
    }

    applyFilters();
    layoutFilterRow();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiuSumoEnsembleSelectionDialog::eventFilter( QObject* watched, QEvent* event )
{
    if ( watched == m_caseView->horizontalHeader() && ( event->type() == QEvent::Resize || event->type() == QEvent::Show ) )
    {
        layoutFilterRow();
    }
    return QDialog::eventFilter( watched, event );
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
