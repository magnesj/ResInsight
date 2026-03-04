/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RicImportGridAndSummaryEnsembleDialog.h"

#include "RiaApplication.h"
#include "RiaEnsembleNameTools.h"
#include "RiaFilePathTools.h"
#include "RiaFileSearchTools.h"
#include "RiaStdStringTools.h"
#include "RiaStringListSerializer.h"

#include "RiuFileDialogTools.h"
#include "RiuTools.h"

#include "cafAppEnum.h"

#include <QCheckBox>
#include <QCollator>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardItem>
#include <QTreeView>
#include <QVBoxLayout>

namespace
{

// Get all first level items (usually ensemble items)
QList<QStandardItem*> firstLevelItems( QStandardItem* rootItem )
{
    QList<QStandardItem*> items;
    for ( int i = 0; i < rootItem->rowCount(); ++i )
    {
        QStandardItem* item = rootItem->child( i );
        if ( item ) items.append( item );
    }
    return items;
}

void setCheckedStateChildItems( QStandardItem* parentItem, Qt::CheckState checkState )
{
    if ( !parentItem ) return;
    for ( int i = 0; i < parentItem->rowCount(); ++i )
    {
        auto childItem = parentItem->child( i );
        if ( childItem && childItem->isCheckable() )
        {
            childItem->setCheckState( checkState );
        }
        setCheckedStateChildItems( childItem, checkState );
    }
}

void findItemsMatching( QStandardItem* parentItem, const QString& substring, QList<QStandardItem*>& matchingItems )
{
    if ( !parentItem ) return;
    for ( int i = 0; i < parentItem->rowCount(); ++i )
    {
        auto searchString = substring + "/";
        auto childItem    = parentItem->child( i );
        if ( childItem )
        {
            auto textToMatch = childItem->text();
            textToMatch.replace( '\\', '/' );
            if ( textToMatch.contains( searchString, Qt::CaseInsensitive ) )
            {
                matchingItems.append( childItem );
            }
        }
        findItemsMatching( childItem, substring, matchingItems );
    }
}

} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicImportGridAndSummaryEnsembleDialogResult RicImportGridAndSummaryEnsembleDialog::runDialog( QWidget* parent )
{
    const QString pathRegistryKey   = "RicImportGridAndSummaryEnsembleDialog_path";
    const QString filterRegistryKey = "RicImportGridAndSummaryEnsembleDialog_filter";

    auto* app = RiaApplication::instance();

    RicImportGridAndSummaryEnsembleDialog dialog( parent );
    {
        QSignalBlocker blocker1( dialog.m_pathFilterField );
        QSignalBlocker blocker2( dialog.m_fileFilterField );

        dialog.setWindowTitle( "Import Grid and Summary Ensemble" );

        // Default path
        QString defaultDir = app->lastUsedDialogDirectory( "GRID_SUMMARY_ENSEMBLE" );
        RiaFilePathTools::appendSeparatorIfNo( defaultDir );
        defaultDir += "*";
        dialog.m_pathFilterField->addItem( QDir::toNativeSeparators( defaultDir ) );

        populateComboBoxFromRegistry( dialog.m_pathFilterField, pathRegistryKey );
        populateComboBoxFromRegistry( dialog.m_fileFilterField, filterRegistryKey );

        dialog.m_pathFilterField->setEditable( true );
        dialog.m_fileFilterField->setEditable( true );

        for ( const auto& s : caf::AppEnum<RiaDefines::EnsembleGroupingMode>::uiTexts() )
        {
            dialog.m_ensembleGroupingMode->addItem( s );
        }

        dialog.updateEffectiveFilter();
        dialog.clearFileList();
        dialog.setOkButtonEnabled( false );

        dialog.resize( 800, 150 );
    }

    if ( dialog.exec() != QDialog::Accepted )
    {
        return {};
    }

    // Save registry history
    const int maxItemsInRegistry = 10;
    {
        RiaStringListSerializer s( pathRegistryKey );
        s.addString( dialog.m_pathFilterField->currentText(), maxItemsInRegistry );
    }
    {
        RiaStringListSerializer s( filterRegistryKey );
        s.addString( dialog.m_fileFilterField->currentText(), maxItemsInRegistry );
    }

    // Save last used directory
    QString rootDir = dialog.rootDirWithSeparator();
    if ( !rootDir.isEmpty() )
    {
        QString dirToSave = rootDir;
        if ( dirToSave.endsWith( RiaFilePathTools::separator() ) ) dirToSave.chop( 1 );
        app->setLastUsedDialogDirectory( "GRID_SUMMARY_ENSEMBLE", dirToSave );
    }

    // Collect checked realization items
    RicImportGridAndSummaryEnsembleDialogResult result;
    result.ok                    = true;
    result.rootDir               = dialog.rootDirWithSeparator();
    result.pathFilter            = dialog.pathFilterWithoutRoot();
    result.groupingMode          = dialog.ensembleGroupingMode();
    result.createGridEnsemble    = dialog.m_createGridEnsembleCheckBox->isChecked();
    result.createSummaryEnsemble = dialog.m_createSummaryEnsembleCheckBox->isChecked();

    auto* rootItem = dialog.m_filePathModel.invisibleRootItem();
    for ( int i = 0; i < rootItem->rowCount(); ++i )
    {
        auto ensembleItem = rootItem->child( i );
        if ( !ensembleItem ) continue;

        for ( int j = 0; j < ensembleItem->rowCount(); ++j )
        {
            auto childItem = ensembleItem->child( j );
            if ( !childItem || !childItem->isCheckable() ) continue;
            if ( childItem->checkState() != Qt::Checked ) continue;

            QString basePath = childItem->data( Qt::UserRole ).toString();
            if ( basePath.isEmpty() ) continue;

            auto it = dialog.m_foundRealizations.find( basePath );
            if ( it == dialog.m_foundRealizations.end() ) continue;

            if ( !it->gridFile.isEmpty() ) result.gridFiles.append( it->gridFile );
            if ( !it->summaryFile.isEmpty() ) result.summaryFiles.append( it->summaryFile );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicImportGridAndSummaryEnsembleDialog::RicImportGridAndSummaryEnsembleDialog( QWidget* parent )
    : QDialog( parent, RiuTools::defaultDialogFlags() )
    , m_isCancelPressed( false )
    , m_blockItemUpdates( false )
{
    // Create widgets
    m_pathFilterField  = new QComboBox();
    m_browseButton     = new QPushButton( "..." );
    m_fileFilterField  = new QComboBox();
    m_effectiveFilterLabel = new QLabel();
    m_effectiveFilterLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_searchButton         = new QPushButton( "Search" );
    m_searchButton->setFixedWidth( 75 );

    m_useRealizationStarCheckBox = new QCheckBox( "Use 'realization-*' in filter" );
    m_ensembleGroupingMode       = new QComboBox();

    m_createGridEnsembleCheckBox    = new QCheckBox( "Create Grid Ensemble" );
    m_createSummaryEnsembleCheckBox = new QCheckBox( "Create Summary Ensemble" );
    m_createGridEnsembleCheckBox->setChecked( true );
    m_createSummaryEnsembleCheckBox->setChecked( true );

    m_outputGroup       = new QGroupBox( "Files Found" );
    m_treeFilterLineEdit = new QLineEdit();
    m_treeFilterLineEdit->setPlaceholderText( "Select Realizations: 1, 5-7, !4, 9-18:3" );
    m_treeFilterButton = new QPushButton( "Apply" );
    m_fileTreeView     = new QTreeView();

    m_buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );

    // Tree view setup
    m_fileTreeView->setModel( &m_filePathModel );
    m_fileTreeView->setHeaderHidden( true );
    m_fileTreeView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    m_fileTreeView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_fileTreeView->setVisible( false );
    m_fileTreeView->setMinimumHeight( 350 );

    m_browseButton->setFixedWidth( 25 );

    // Initially hide filter widgets
    m_treeFilterLineEdit->setVisible( false );
    m_treeFilterButton->setVisible( false );

    // Connect signals
    connect( m_pathFilterField,
             SIGNAL( currentTextChanged( const QString& ) ),
             this,
             SLOT( slotPathFilterChanged( const QString& ) ) );
    connect( m_pathFilterField,
             SIGNAL( editTextChanged( const QString& ) ),
             this,
             SLOT( slotPathFilterChanged( const QString& ) ) );

    connect( m_fileFilterField,
             SIGNAL( currentTextChanged( const QString& ) ),
             this,
             SLOT( slotFileFilterChanged( const QString& ) ) );
    connect( m_fileFilterField,
             SIGNAL( editTextChanged( const QString& ) ),
             this,
             SLOT( slotFileFilterChanged( const QString& ) ) );

    connect( m_browseButton, SIGNAL( clicked() ), this, SLOT( slotBrowseClicked() ) );
    connect( m_useRealizationStarCheckBox, SIGNAL( clicked() ), this, SLOT( slotUseRealizationStarClicked() ) );
    connect( m_searchButton, SIGNAL( clicked() ), this, SLOT( slotSearchClicked() ) );
    connect( m_treeFilterButton, SIGNAL( clicked() ), this, SLOT( slotFilterTreeViewClicked() ) );
    connect( m_treeFilterLineEdit, &QLineEdit::returnPressed, m_treeFilterButton, &QPushButton::click );
    connect( m_treeFilterLineEdit, &QLineEdit::textEdited, m_treeFilterButton, &QPushButton::click );

    connect( m_buttons, SIGNAL( accepted() ), this, SLOT( slotOkClicked() ) );
    connect( m_buttons, SIGNAL( rejected() ), this, SLOT( slotCancelClicked() ) );

    m_buttons->button( QDialogButtonBox::Ok )->setDefault( true );

    // itemChanged: propagate check state to children and upward
    QObject::connect( &m_filePathModel,
                      &QStandardItemModel::itemChanged,
                      [this]( QStandardItem* item )
                      {
                          if ( m_blockItemUpdates ) return;

                          if ( item->isCheckable() )
                          {
                              setCheckedStateChildItems( item, item->checkState() );
                          }

                          if ( item->checkState() == Qt::Checked )
                          {
                              auto parentItem = item->parent();
                              if ( parentItem )
                              {
                                  m_blockItemUpdates = true;
                                  parentItem->setCheckState( Qt::Checked );
                                  m_blockItemUpdates = false;
                              }
                          }
                      } );

    // Build layout
    QVBoxLayout* dialogLayout = new QVBoxLayout();

    // Search group
    QGroupBox*   searchGroup      = new QGroupBox( "Search" );
    QGridLayout* searchGridLayout = new QGridLayout();
    int          row              = 0;

    searchGridLayout->addWidget( new QLabel( "Path pattern" ), row, 0 );
    searchGridLayout->addWidget( m_pathFilterField, row, 1, 1, 2 );
    searchGridLayout->addWidget( m_browseButton, row, 3 );

    row++;
    searchGridLayout->addWidget( new QLabel( "File pattern" ), row, 0 );
    searchGridLayout->addWidget( m_fileFilterField, row, 1, 1, 2 );

    row++;
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->addWidget( m_useRealizationStarCheckBox );
        hLayout->addWidget( new QLabel( "Ensemble Grouping" ) );
        hLayout->addWidget( m_ensembleGroupingMode );
        hLayout->addStretch( 1 );
        searchGridLayout->addLayout( hLayout, row, 1 );
    }

    row++;
    searchGridLayout->addWidget( new QLabel( "Effective filter" ), row, 0 );
    searchGridLayout->addWidget( m_effectiveFilterLabel, row, 1, 1, 2 );
    searchGridLayout->addWidget( m_searchButton, row, 3 );

    searchGroup->setLayout( searchGridLayout );

    // Import group
    QGroupBox*   importGroup  = new QGroupBox( "Import" );
    QHBoxLayout* importLayout = new QHBoxLayout();
    importLayout->addWidget( m_createGridEnsembleCheckBox );
    importLayout->addWidget( m_createSummaryEnsembleCheckBox );
    importLayout->addStretch( 1 );
    importGroup->setLayout( importLayout );

    // Files Found group
    QGridLayout* outputGridLayout = new QGridLayout();
    outputGridLayout->addWidget( m_treeFilterLineEdit, 0, 0 );
    outputGridLayout->addWidget( m_treeFilterButton, 0, 1 );
    outputGridLayout->addWidget( m_fileTreeView, 1, 0, 1, 2 );
    m_outputGroup->setLayout( outputGridLayout );

    dialogLayout->addWidget( searchGroup );
    dialogLayout->addWidget( importGroup );
    dialogLayout->addWidget( m_outputGroup );
    dialogLayout->addWidget( m_buttons );

    setLayout( dialogLayout );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicImportGridAndSummaryEnsembleDialog::cleanPathFilter() const
{
    QString pathFilterText = m_pathFilterField->currentText().trimmed();
    pathFilterText         = RiaFilePathTools::toInternalSeparator( pathFilterText );
    pathFilterText         = RiaFilePathTools::removeDuplicatePathSeparators( pathFilterText );
    pathFilterText.replace( QString( "**" ), QString( "*" ) );

    if ( m_useRealizationStarCheckBox->isChecked() )
    {
        const QString      pattern = "realization-\\d+";
        QRegularExpression regexp( pattern, QRegularExpression::CaseInsensitiveOption );
        pathFilterText.replace( regexp, "realization-*" );
    }

    return pathFilterText;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicImportGridAndSummaryEnsembleDialog::rootDirWithSeparator() const
{
    QString rootDir = RiaFilePathTools::rootSearchPathFromSearchFilter( cleanPathFilter() );
    return RiaFilePathTools::appendSeparatorIfNo( rootDir );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicImportGridAndSummaryEnsembleDialog::pathFilterWithoutRoot() const
{
    QString pathFilter = cleanPathFilter();
    QString rootDir    = RiaFilePathTools::rootSearchPathFromSearchFilter( pathFilter );

    pathFilter.remove( 0, rootDir.size() );
    if ( pathFilter.startsWith( RiaFilePathTools::separator() ) ) pathFilter.remove( 0, 1 );
    return pathFilter;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicImportGridAndSummaryEnsembleDialog::fileNameFilter() const
{
    return m_fileFilterField->currentText().trimmed();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::updateEffectiveFilter()
{
    QString pathFilter = pathFilterWithoutRoot();
    if ( pathFilter == "*" || pathFilter.endsWith( QString( RiaFilePathTools::separator() ) + "*" ) )
    {
        pathFilter.chop( 1 );
        pathFilter = pathFilter + "...";
    }

    QString fileFilter = fileNameFilter();
    if ( fileFilter.isEmpty() ) fileFilter = "*";

    QString extensions = "EGRID|SMSPEC|ESMRY";

    QString effFilter = QString( "%1%2/%3.%4" ).arg( rootDirWithSeparator() ).arg( pathFilter ).arg( fileFilter ).arg( extensions );
    effFilter         = RiaFilePathTools::removeDuplicatePathSeparators( effFilter );

    m_effectiveFilterLabel->setText( QDir::toNativeSeparators( effFilter ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::setOkButtonEnabled( bool enabled )
{
    m_buttons->button( QDialogButtonBox::Ok )->setEnabled( enabled );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RicImportGridAndSummaryEnsembleDialog::findMatchingFiles( const QStringList& extensions )
{
    if ( cleanPathFilter().isEmpty() ) return {};

    QString rootDir    = rootDirWithSeparator();
    QString pathFilter = pathFilterWithoutRoot();
    if ( rootDir.size() > 1 && rootDir.endsWith( RiaFilePathTools::separator() ) ) rootDir.chop( 1 );

    QStringList matchingFolders;
    RiaFileSearchTools::findMatchingFoldersRecursively( rootDir, pathFilter, matchingFolders );

    QString fileFilter = fileNameFilter();
    if ( fileFilter.isEmpty() ) fileFilter = "*";

    QStringList nameFilters;
    for ( const auto& ext : extensions )
    {
        nameFilters.append( fileFilter + "." + ext );
    }

    return RiaFileSearchTools::findFilesInFolders( matchingFolders, nameFilters );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::updateFileListWidget()
{
    m_filePathModel.clear();
    m_foundRealizations.clear();

    QStringList gridFiles    = findMatchingFiles( { "EGRID" } );
    QStringList summaryFiles = findMatchingFiles( { "SMSPEC", "ESMRY" } );

    // Sort so SMSPEC comes before ESMRY
    std::sort( summaryFiles.begin(), summaryFiles.end() );

    QString rootDir = rootDirWithSeparator();
    int     rootLen = rootDir.size();

    // Build m_foundRealizations map using relative base path (no extension) as key
    for ( const auto& filePath : gridFiles )
    {
        QString rel      = filePath.mid( rootLen );
        QFileInfo fi( rel );
        QString   basePath = QDir::cleanPath( fi.path() + "/" + fi.completeBaseName() );
        if ( !m_foundRealizations.contains( basePath ) )
        {
            m_foundRealizations[basePath].gridFile = filePath;
        }
        else if ( m_foundRealizations[basePath].gridFile.isEmpty() )
        {
            m_foundRealizations[basePath].gridFile = filePath;
        }
    }

    for ( const auto& filePath : summaryFiles )
    {
        QString rel      = filePath.mid( rootLen );
        QFileInfo fi( rel );
        QString   basePath = QDir::cleanPath( fi.path() + "/" + fi.completeBaseName() );
        if ( !m_foundRealizations.contains( basePath ) )
        {
            m_foundRealizations[basePath].summaryFile = filePath;
        }
        else if ( m_foundRealizations[basePath].summaryFile.isEmpty() )
        {
            // Only store first match per base (SMSPEC wins over ESMRY since list is sorted)
            m_foundRealizations[basePath].summaryFile = filePath;
        }
    }

    if ( m_foundRealizations.isEmpty() ) return;

    auto mode = ensembleGroupingMode();

    if ( mode == RiaDefines::EnsembleGroupingMode::NONE )
    {
        // Single "Files" ensemble item, one child per base path
        auto rootItem     = m_filePathModel.invisibleRootItem();
        auto ensembleItem = new QStandardItem( "Files" );
        ensembleItem->setCheckable( true );
        ensembleItem->setCheckState( Qt::Checked );
        rootItem->appendRow( ensembleItem );

        for ( auto it = m_foundRealizations.begin(); it != m_foundRealizations.end(); ++it )
        {
            auto childItem = new QStandardItem( QDir::toNativeSeparators( it.key() ) );
            childItem->setCheckable( true );
            childItem->setCheckState( Qt::Checked );
            childItem->setData( it.key(), Qt::UserRole );
            ensembleItem->appendRow( childItem );
        }
    }
    else
    {
        // Build list of representative files for grouping (grid if available, else summary)
        QStringList representativeFiles;
        QMap<QString, QString> repFileToBasePath;
        for ( auto it = m_foundRealizations.begin(); it != m_foundRealizations.end(); ++it )
        {
            QString rep = it.value().gridFile.isEmpty() ? it.value().summaryFile : it.value().gridFile;
            if ( !rep.isEmpty() )
            {
                representativeFiles.append( rep );
                repFileToBasePath[rep] = it.key();
            }
        }

        // Sort numerically
        QCollator collator;
        collator.setNumericMode( true );
        std::sort( representativeFiles.begin(), representativeFiles.end(), collator );

        auto grouping = RiaEnsembleNameTools::groupFilesByEnsembleName( representativeFiles, mode );

        auto rootItem = m_filePathModel.invisibleRootItem();
        bool isFirst  = true;
        for ( const auto& [groupName, groupFiles] : grouping )
        {
            auto ensembleItem = new QStandardItem( QDir::toNativeSeparators( groupName ) );
            ensembleItem->setCheckable( true );
            ensembleItem->setCheckState( Qt::Checked );
            rootItem->appendRow( ensembleItem );

            for ( const auto& repFile : groupFiles )
            {
                QString basePath = repFileToBasePath.value( repFile );
                if ( basePath.isEmpty() ) continue;

                auto childItem = new QStandardItem( QDir::toNativeSeparators( basePath ) );
                childItem->setCheckable( true );
                childItem->setCheckState( Qt::Checked );
                childItem->setData( basePath, Qt::UserRole );
                ensembleItem->appendRow( childItem );
            }

            if ( isFirst )
            {
                QModelIndex index = m_filePathModel.index( 0, 0 );
                m_fileTreeView->expand( index );
                isFirst = false;
            }
        }
    }

    // Expand first item
    if ( m_filePathModel.rowCount() > 0 )
    {
        QModelIndex index = m_filePathModel.index( 0, 0 );
        m_fileTreeView->expand( index );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::clearFileList()
{
    m_foundRealizations.clear();
    m_filePathModel.clear();
    m_outputGroup->setTitle( "Files Found" );
    setOkButtonEnabled( false );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaDefines::EnsembleGroupingMode RicImportGridAndSummaryEnsembleDialog::ensembleGroupingMode() const
{
    if ( m_ensembleGroupingMode->currentIndex() == 0 ) return RiaDefines::EnsembleGroupingMode::FMU_FOLDER_STRUCTURE;
    if ( m_ensembleGroupingMode->currentIndex() == 1 ) return RiaDefines::EnsembleGroupingMode::EVEREST_FOLDER_STRUCTURE;
    if ( m_ensembleGroupingMode->currentIndex() == 2 ) return RiaDefines::EnsembleGroupingMode::NONE;
    if ( m_ensembleGroupingMode->currentIndex() == 3 ) return RiaDefines::EnsembleGroupingMode::RESINSIGHT_OPMFLOW_STRUCTURE;
    return RiaDefines::EnsembleGroupingMode::FMU_FOLDER_STRUCTURE;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::populateComboBoxFromRegistry( QComboBox* comboBox, const QString& key )
{
    RiaStringListSerializer serializer( key );
    QStringList             items = serializer.textStrings();

    const int maxItemsInRegistry = 10;
    int       numItems           = std::min( (int)items.size(), maxItemsInRegistry );
    for ( int i = 0; i < numItems; i++ )
    {
        comboBox->addItem( items[i] );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotPathFilterChanged( const QString& /*text*/ )
{
    updateEffectiveFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotFileFilterChanged( const QString& /*text*/ )
{
    clearFileList();
    updateEffectiveFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotBrowseClicked()
{
    QString folder = RiuFileDialogTools::getExistingDirectory( this, "Select folder", rootDirWithSeparator() );
    RiaFilePathTools::appendSeparatorIfNo( folder );
    folder += "*";
    if ( !folder.isEmpty() )
    {
        m_pathFilterField->addItem( QDir::toNativeSeparators( folder ) );
        m_pathFilterField->setCurrentText( QDir::toNativeSeparators( folder ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotUseRealizationStarClicked()
{
    updateEffectiveFilter();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotSearchClicked()
{
    clearFileList();

    m_treeFilterLineEdit->setVisible( true );
    m_treeFilterButton->setVisible( true );

    if ( !m_fileTreeView->isVisible() )
    {
        m_fileTreeView->setVisible( true );
        if ( height() < 550 ) resize( width(), 550 );
    }

    updateFileListWidget();

    int realizationCount = m_foundRealizations.size();
    if ( realizationCount > 0 )
    {
        m_outputGroup->setTitle( QString( "Files Found (%1)" ).arg( realizationCount ) );
        setOkButtonEnabled( true );
        m_buttons->button( QDialogButtonBox::Ok )->setFocus();
    }
    else
    {
        m_outputGroup->setTitle( "Files Found" );

        // Add "No files found" status item
        auto rootItem     = m_filePathModel.invisibleRootItem();
        auto statusItem   = new QStandardItem( "No files found" );
        rootItem->appendRow( statusItem );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotFilterTreeViewClicked()
{
    QString filterText = m_treeFilterLineEdit->text();
    auto    values     = RiaStdStringTools::valuesFromRangeSelection( filterText.toStdString() );

    auto items = firstLevelItems( m_filePathModel.invisibleRootItem() );
    for ( auto item : items )
    {
        if ( item->checkState() == Qt::Unchecked ) continue;

        if ( filterText.isEmpty() )
        {
            setCheckedStateChildItems( item, Qt::Checked );
        }
        else
        {
            setCheckedStateChildItems( item, Qt::Unchecked );

            for ( auto val : values )
            {
                QString searchString = "realization-" + QString::number( val );

                QList<QStandardItem*> matchingItems;
                findItemsMatching( item, searchString, matchingItems );
                for ( auto matchedItem : matchingItems )
                {
                    matchedItem->setCheckState( Qt::Checked );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotOkClicked()
{
    accept();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::slotCancelClicked()
{
    m_isCancelPressed = true;
    reject();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleDialog::showEvent( QShowEvent* event )
{
    m_searchButton->setFocus();
    QDialog::showEvent( event );
}
