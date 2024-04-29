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

#pragma once

#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QItemSelection>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QUrl>
#include <QWizard>

#include "RiaOsduConnector.h"

class QFile;
class QProgressDialog;
class QLabel;
class QTextEdit;
class QTableView;

class RimWellPathImport;
class RimOilFieldEntry;
class RimWellPathEntry;

namespace caf
{
class PdmUiTreeView;
class PdmUiListView;
class PdmUiPropertyView;
class PdmObjectCollection;
} // namespace caf

class OsduFieldTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit OsduFieldTableModel( QObject* parent = nullptr )
        : QAbstractTableModel( parent )
    {
    }

    // ~OsduFieldTableModel() override {}

    int rowCount( const QModelIndex& parent = QModelIndex() ) const override
    {
        Q_UNUSED( parent );
        return m_osduFields.size();
    }

    int columnCount( const QModelIndex& parent = QModelIndex() ) const override
    {
        Q_UNUSED( parent );
        // Assuming you have three fields: id, kind, and name
        return 3;
    }

    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override
    {
        if ( !index.isValid() ) return QVariant();

        if ( index.row() >= static_cast<int>( m_osduFields.size() ) || index.row() < 0 ) return QVariant();

        if ( role == Qt::DisplayRole )
        {
            const OsduField& field = m_osduFields.at( index.row() );
            switch ( index.column() )
            {
                case 0:
                    return field.id;
                case 1:
                    return field.kind;
                case 2:
                    return field.name;
                default:
                    return QVariant();
            }
        }

        return QVariant();
    }

    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override
    {
        if ( role != Qt::DisplayRole ) return QVariant();

        if ( orientation == Qt::Horizontal )
        {
            switch ( section )
            {
                case 0:
                    return tr( "ID" );
                case 1:
                    return tr( "Kind" );
                case 2:
                    return tr( "Name" );
                default:
                    return QVariant();
            }
        }
        return QVariant();
    }

    void setOsduFields( const std::vector<OsduField>& osduFields )
    {
        beginInsertRows( QModelIndex(), 0, 0 ); // notify views and proxy models that a line will be inserted

        m_osduFields = osduFields;
        // auto topLeft     = createIndex( 0, 0 );
        // auto bottomRight = createIndex( m_osduFields.size(), 3 );
        // m_data.prepend( somedata ); // do the modification to the model data
        endInsertRows();

        // emit dataChanged( topLeft, bottomRight );
    }

private:
    std::vector<OsduField> m_osduFields;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class AuthenticationPage : public QWizardPage
{
    Q_OBJECT

public:
    AuthenticationPage( RiaOsduConnector* osduConnector, QWidget* parent = nullptr );

    void initializePage() override;
    bool isComplete() const override;

private slots:
    void accessOk();

private:
    bool m_accessOk;
};

class OsduFieldTableModel;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class FieldSelectionPage : public QWizardPage
{
    Q_OBJECT

public:
    FieldSelectionPage( RimWellPathImport* wellPathImport, RiaOsduConnector* m_osduConnector, QWidget* parent = nullptr );
    ~FieldSelectionPage() override;

    void initializePage() override;
    bool isComplete() const override;
private slots:
    void wellsFinished();

private:
    caf::PdmUiPropertyView* m_propertyView;
    RiaOsduConnector*       m_osduConnector;
    QTableView*             m_tableView;
    OsduFieldTableModel*    m_osduFieldsModel;
};

//--------------------------------------------------------------------------------------------------
/// Container class used to define column headers
//--------------------------------------------------------------------------------------------------
class ObjectGroupWithHeaders : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    ObjectGroupWithHeaders()
    {
        CAF_PDM_InitFieldNoDefault( &objects, "PdmObjects", "" );

        CAF_PDM_InitField( &m_isChecked, "IsChecked", true, "Active" );
        m_isChecked.uiCapability()->setUiHidden( true );
    };

    void defineObjectEditorAttribute( QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;

public:
    caf::PdmChildArrayField<PdmObjectHandle*> objects;

protected:
    caf::PdmFieldHandle* objectToggleField() override { return &m_isChecked; }

protected:
    caf::PdmField<bool> m_isChecked;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class DownloadEntity
{
public:
    QString name;
    QString requestUrl;
    QString responseFilename;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class SummaryPageDownloadEntity : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    SummaryPageDownloadEntity();

    caf::PdmField<QString> name;
    caf::PdmField<QString> requestUrl;
    caf::PdmField<QString> responseFilename;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class WellSelectionPage : public QWizardPage
{
    Q_OBJECT

public:
    WellSelectionPage( RimWellPathImport* wellPathImport, QWidget* parent = nullptr );
    ~WellSelectionPage() override;

    void initializePage() override;

    void selectedWellPathEntries( std::vector<DownloadEntity>& downloadEntities, caf::PdmObjectHandle* objHandle );

private:
    void sortObjectsByDescription( caf::PdmObjectCollection* objects );

private slots:
    void customMenuRequested( const QPoint& pos );

private:
    ObjectGroupWithHeaders* m_regionsWithVisibleWells;
    RimWellPathImport*      m_wellPathImportObject;
    caf::PdmUiTreeView*     m_wellSelectionTreeView;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class WellSummaryPage : public QWizardPage
{
    Q_OBJECT

public:
    WellSummaryPage( RimWellPathImport* wellPathImport, QWidget* parent = nullptr );

    void initializePage() override;

    void updateSummaryPage();

private slots:
    void slotShowDetails();

private:
    RimWellPathImport*        m_wellPathImportObject;
    QTextEdit*                m_textEdit;
    caf::PdmUiListView*       m_listView;
    caf::PdmObjectCollection* m_objectGroup;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class RiuWellImportWizard : public QWizard
{
    Q_OBJECT

public:
    enum DownloadState
    {
        DOWNLOAD_FIELDS,
        DOWNLOAD_WELLS,
        DOWNLOAD_WELL_PATH,
        DOWNLOAD_UNDEFINED
    };

public:
    RiuWellImportWizard( const QString&     downloadFolder,
                         RiaOsduConnector*  osduConnector,
                         RimWellPathImport* wellPathImportObject,
                         QWidget*           parent = nullptr );
    ~RiuWellImportWizard() override;

    QStringList absoluteFilePathsToWellPaths() const;

    // Methods used from the wizard pages
    void resetAuthenticationCount();

public slots:
    void downloadWellPaths();
    void downloadWells( const QString& fieldId );
    void downloadFields();

    void httpFinished();

    void slotAuthenticationRequired( QNetworkReply* networkReply, QAuthenticator* authenticator );

    int wellSelectionPageId();

private slots:
    void slotCurrentIdChanged( int currentId );

private:
    void updateFieldsModel();
    void parseWellsResponse( RimOilFieldEntry* oilFieldEntry );

    QString getValue( const QString& key, const QString& stringContent );

    QProgressDialog* progressDialog();
    void             hideProgressDialog();

private:
    RiaOsduConnector* m_osduConnector;

    QString m_destinationFolder;

    RimWellPathImport*  m_wellPathImportObject;
    caf::PdmUiTreeView* m_pdmTreeView;

    QProgressDialog* m_myProgressDialog;

    bool m_httpRequestAborted;

    bool m_firstTimeRequestingAuthentication;

    QList<DownloadEntity> m_wellRequestQueue;

    DownloadState m_currentDownloadState;

    int m_fieldSelectionPageId;
    int m_wellSelectionPageId;
    int m_wellSummaryPageId;
};
