/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-2012 Statoil ASA, Ceetron AS
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

#include "RiuWellImportWizard.h"

#include "RiaFeatureCommandContext.h"
#include "RiaOsduConnector.h"

#include "RimWellPathImport.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmDocument.h"
#include "cafPdmObject.h"
#include "cafPdmObjectGroup.h"
#include "cafPdmUiListView.h"
#include "cafPdmUiPropertyView.h"
#include "cafPdmUiTreeAttributes.h"
#include "cafPdmUiTreeView.h"
#include "cafPdmUiTreeViewEditor.h"
#include "cafUtils.h"

#include <QAbstractTableModel>
#include <QMenu>
#include <QObject>
#include <QString>

#include <QtNetwork>
#include <QtWidgets>

#include <algorithm>
#include <vector>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuWellImportWizard::RiuWellImportWizard( const QString&     downloadFolder,
                                          RiaOsduConnector*  osduConnector,
                                          RimWellPathImport* wellPathImportObject,
                                          QWidget*           parent /*= 0*/ )
    : QWizard( parent )
{
    m_wellPathImportObject = wellPathImportObject;

    m_osduConnector     = osduConnector;
    m_destinationFolder = downloadFolder;

    m_myProgressDialog                  = nullptr;
    m_firstTimeRequestingAuthentication = true;

    addPage( new AuthenticationPage( m_osduConnector, this ) );
    m_fieldSelectionPageId = addPage( new FieldSelectionPage( m_wellPathImportObject, m_osduConnector, this ) );
    m_wellSelectionPageId  = addPage( new WellSelectionPage( m_wellPathImportObject, m_osduConnector, this ) );
    m_wellSummaryPageId    = addPage( new WellSummaryPage( m_wellPathImportObject, this ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuWellImportWizard::~RiuWellImportWizard()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::downloadFields()
{
    m_currentDownloadState = DOWNLOAD_FIELDS;

    m_osduConnector->requestFieldsByName( "IVAR" );
}

//--------------------------------------------------------------------------------------------------
/// This slot will be called for the first network reply that will need authentication.
/// If the authentication is successful, the username/password is cached in the QNetworkAccessManager
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::slotAuthenticationRequired( QNetworkReply* networkReply, QAuthenticator* authenticator )
{
    if ( m_firstTimeRequestingAuthentication )
    {
        m_firstTimeRequestingAuthentication = false;
    }
    else
    {
        QMessageBox::information( this,
                                  "Authentication failed",
                                  "Failed to authenticate. You will now be directed back to the first wizard page." );
        m_firstTimeRequestingAuthentication = true;

        restart();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QProgressDialog* RiuWellImportWizard::progressDialog()
{
    if ( !m_myProgressDialog )
    {
        m_myProgressDialog = new QProgressDialog( this );
    }

    return m_myProgressDialog;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::hideProgressDialog()
{
    if ( m_myProgressDialog )
    {
        m_myProgressDialog->hide();

        QCoreApplication::processEvents();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::downloadWells( const QString& fieldId )
{
    m_osduConnector->requestWellsByFieldId( fieldId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::downloadWellPaths()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::resetAuthenticationCount()
{
    m_firstTimeRequestingAuthentication = true;
    QTimer::singleShot( 0, m_osduConnector, SLOT( requestToken() ) );
    //    m_osduConnector->requestToken();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiuWellImportWizard::wellSelectionPageId()
{
    return m_wellSelectionPageId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
AuthenticationPage::AuthenticationPage( RiaOsduConnector* osduConnector, QWidget* parent /*= 0*/ )
    : QWizardPage( parent )
    , m_accessOk( false )
{
    setTitle( "OSDU - Login" );

    QVBoxLayout* layout = new QVBoxLayout;

    QLabel* label = new QLabel( "Checking OSDU connection..." );
    layout->addWidget( label );

    QFormLayout* formLayout = new QFormLayout;
    layout->addLayout( formLayout );

    QLineEdit* serverLineEdit    = new QLineEdit( osduConnector->server(), this );
    QLineEdit* partitionLineEdit = new QLineEdit( osduConnector->dataPartition(), this );

    formLayout->addRow( "Server:", serverLineEdit );
    formLayout->addRow( "Data Partition:", partitionLineEdit );

    setLayout( layout );

    connect( osduConnector, SIGNAL( tokenReady( const QString& ) ), this, SLOT( accessOk() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void AuthenticationPage::initializePage()
{
    RiuWellImportWizard* wiz = dynamic_cast<RiuWellImportWizard*>( wizard() );
    wiz->resetAuthenticationCount();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool AuthenticationPage::isComplete() const
{
    return m_accessOk;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void AuthenticationPage::accessOk()
{
    m_accessOk = true;
    emit( completeChanged() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
FieldSelectionPage::FieldSelectionPage( RimWellPathImport* wellPathImport, RiaOsduConnector* osduConnector, QWidget* parent /*= 0*/ )
{
    setTitle( "Field Selection" );

    QVBoxLayout* layout = new QVBoxLayout;
    setLayout( layout );

    QLabel* label = new QLabel( "Select fields" );
    layout->addWidget( label );

    m_tableView = new QTableView( this );
    m_tableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_osduFieldsModel = new OsduFieldTableModel;
    m_tableView->setModel( m_osduFieldsModel );
    layout->addWidget( m_tableView );
    layout->setStretchFactor( m_tableView, 10 );

    // Tree view
    caf::PdmUiTreeView* treeView = new caf::PdmUiTreeView( this );
    treeView->setPdmItem( wellPathImport );
    layout->addWidget( treeView );
    layout->setStretchFactor( treeView, 10 );

    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );

    m_osduConnector = osduConnector;
    connect( m_osduConnector, SIGNAL( fieldsFinished() ), SLOT( fieldsFinished() ) );

    connect( m_tableView->selectionModel(),
             SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
             SLOT( selectField( const QItemSelection&, const QItemSelection& ) ) );

    connect( m_tableView->selectionModel(),
             SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
             SIGNAL( completeChanged() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void FieldSelectionPage::initializePage()
{
    RiuWellImportWizard* wiz = dynamic_cast<RiuWellImportWizard*>( wizard() );
    wiz->downloadFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void FieldSelectionPage::fieldsFinished()
{
    std::vector<OsduField> fields = m_osduConnector->fields();
    m_osduFieldsModel->setOsduFields( fields );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void FieldSelectionPage::selectField( const QItemSelection& newSelection, const QItemSelection& oldSelection )
{
    qDebug() << "Select field: " << newSelection.indexes().size();
    if ( !newSelection.indexes().empty() )
    {
        QModelIndex          index   = newSelection.indexes()[0];
        int                  column  = 0;
        QString              fieldId = m_osduFieldsModel->data( index.siblingAtColumn( column ) ).toString();
        RiuWellImportWizard* wiz     = dynamic_cast<RiuWellImportWizard*>( wizard() );
        wiz->setSelectedFieldId( fieldId );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool FieldSelectionPage::isComplete() const
{
    QItemSelectionModel* select = m_tableView->selectionModel();
    return select->selectedRows().size() == 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
FieldSelectionPage::~FieldSelectionPage()
{
    m_propertyView->showProperties( nullptr );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
WellSelectionPage::WellSelectionPage( RimWellPathImport* wellPathImport, RiaOsduConnector* osduConnector, QWidget* parent /*= 0*/ )
{
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout( layout );

    QLabel* label = new QLabel( "Select wells" );
    layout->addWidget( label );

    m_tableView = new QTableView( this );
    m_tableView->setSelectionBehavior( QAbstractItemView::SelectRows );
    m_osduWellboresModel = new OsduWellboreTableModel;
    m_tableView->setModel( m_osduWellboresModel );
    layout->addWidget( m_tableView );
    layout->setStretchFactor( m_tableView, 10 );

    m_wellPathImportObject = wellPathImport;

    m_osduConnector = osduConnector;
    connect( m_osduConnector, SIGNAL( wellsFinished() ), SLOT( wellsFinished() ) );
    connect( m_osduConnector, SIGNAL( wellboresFinished( const QString& ) ), SLOT( wellboresFinished( const QString& ) ) );

    connect( m_tableView->selectionModel(),
             SIGNAL( selectionChanged( const QItemSelection&, const QItemSelection& ) ),
             SIGNAL( completeChanged() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void WellSelectionPage::initializePage()
{
    RiuWellImportWizard* wiz = dynamic_cast<RiuWellImportWizard*>( wizard() );
    if ( !wiz ) return;

    QString fieldId = wiz->selectedFieldId();
    qDebug() << "FIELD ID: " << fieldId;
    wiz->downloadWells( fieldId );

    setButtonText( QWizard::NextButton, "Import" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
WellSelectionPage::~WellSelectionPage()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void WellSelectionPage::wellsFinished()
{
    std::vector<OsduWell> wells = m_osduConnector->wells();

    qDebug() << "WELLS FINISHED: " << wells.size();

    for ( auto w : wells )
    {
        m_osduConnector->requestWellboresByWellId( w.id );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void WellSelectionPage::wellboresFinished( const QString& wellId )
{
    std::vector<OsduWellbore> wellbores = m_osduConnector->wellbores( wellId );
    qDebug() << "Wellbores for " << wellId << ": " << wellbores.size();
    // TODO: change to an append operation
    if ( !wellbores.empty() ) m_osduWellboresModel->setOsduWellbores( wellbores );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool WellSelectionPage::isComplete() const
{
    QItemSelectionModel* select = m_tableView->selectionModel();
    return select->selectedRows().size() == 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
WellSummaryPage::WellSummaryPage( RimWellPathImport* wellPathImport, QWidget* parent /*= 0*/ )
{
    m_wellPathImportObject = wellPathImport;
    m_wellPathImportObject->setUiHidden( true );

    QVBoxLayout* layout = new QVBoxLayout;
    setLayout( layout );

    m_textEdit = new QTextEdit( this );
    m_textEdit->setReadOnly( true );
    layout->addWidget( m_textEdit );

    QPushButton* button = new QPushButton( "Show/hide details", this );
    connect( button, SIGNAL( clicked() ), this, SLOT( slotShowDetails() ) );
    layout->addWidget( button );

    m_listView = new caf::PdmUiListView( this );
    layout->setStretchFactor( m_listView, 10 );
    layout->addWidget( m_listView );
    m_listView->hide();

    m_objectGroup = new caf::PdmObjectCollection;

    setButtonText( QWizard::FinishButton, "Import" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void WellSummaryPage::initializePage()
{
    RiuWellImportWizard* wiz = dynamic_cast<RiuWellImportWizard*>( wizard() );
    wiz->downloadWellPaths();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void WellSummaryPage::updateSummaryPage()
{
    // m_textEdit->setText( "Summary of imported wells\n\n" );

    // size_t  wellPathCount = 0;
    // QString errorString;

    // RiuWellImportWizard*        wiz               = dynamic_cast<RiuWellImportWizard*>( wizard() );
    // WellSelectionPage*          wellSelectionPage = dynamic_cast<WellSelectionPage*>( wiz->page( wiz->wellSelectionPageId() ) );
    // std::vector<DownloadEntity> downloadEntities;
    // wellSelectionPage->selectedWellPathEntries( downloadEntities, nullptr );

    // for ( size_t i = 0; i < downloadEntities.size(); i++ )
    // {
    //     if ( caf::Utils::fileExists( downloadEntities[i].responseFilename ) )
    //     {
    //         wellPathCount++;
    //     }
    //     else
    //     {
    //         errorString +=
    //             QString( "Failed to get file '%1' from well '%2'\n" ).arg( downloadEntities[i].responseFilename ).arg(
    //             downloadEntities[i].name );
    //     }

    //     SummaryPageDownloadEntity* sumPageEntity = new SummaryPageDownloadEntity;
    //     sumPageEntity->name                      = downloadEntities[i].name;
    //     sumPageEntity->responseFilename          = downloadEntities[i].responseFilename;
    //     sumPageEntity->requestUrl                = downloadEntities[i].requestUrl;

    //     m_objectGroup->objects().push_back( sumPageEntity );
    // }

    // m_textEdit->setText( QString( "Downloaded successfully %1 well paths.\nPlease push 'Import' button to import well "
    //                               "paths into ResInsight.\n\n" )
    //                          .arg( wellPathCount ) );
    // if ( !errorString.isEmpty() )
    // {
    //     m_textEdit->append( "Detected following errors during well path download. See details below." );
    //     m_textEdit->append( errorString );
    // }

    // m_listView->setPdmObject( m_objectGroup );
    // m_objectGroup->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuWellImportWizard::setSelectedFieldId( const QString& fieldId )
{
    m_selectedFieldId = fieldId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuWellImportWizard::selectedFieldId() const
{
    return m_selectedFieldId;
}
