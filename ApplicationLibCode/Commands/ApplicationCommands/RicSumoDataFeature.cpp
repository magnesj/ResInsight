/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022     Equinor ASA
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

#include "RicSumoDataFeature.h"

#include "RiaApplication.h"
#include "RiaGuiApplication.h"
#include "RiaPreferences.h"
#include "RiaPreferencesSystem.h"

#include "Sumo/RimSumoConnector.h"

#include "RiaLogging.h"
#include <QAction>

CAF_CMD_SOURCE_INIT( RicSumoDataFeature, "RicSumoDataFeature" );

SimpleDialog::SimpleDialog( QWidget* parent )
    : QDialog( parent )
{
    setWindowTitle( "Simple Dialog" );

    QVBoxLayout* layout = new QVBoxLayout( this );

    label = new QLabel( "This is a simple dialog.", this );
    layout->addWidget( label );

    authButton = new QPushButton( "Authenticate", this );
    connect( authButton, &QPushButton::clicked, this, &SimpleDialog::onAuthClicked );
    layout->addWidget( authButton );

    fieldsButton = new QPushButton( "Field Names", this );
    connect( fieldsButton, &QPushButton::clicked, this, &SimpleDialog::onFieldsClicked );
    layout->addWidget( fieldsButton );

    okButton = new QPushButton( "OK", this );
    connect( okButton, &QPushButton::clicked, this, &SimpleDialog::onOkClicked );
    layout->addWidget( okButton );

    cancelButton = new QPushButton( "Cancel", this );
    connect( cancelButton, &QPushButton::clicked, this, &SimpleDialog::onCancelClicked );
    layout->addWidget( cancelButton );

    setLayout( layout );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SimpleDialog::~SimpleDialog()
{
    if ( m_sumoConnector )
    {
        m_sumoConnector->deleteLater();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SimpleDialog::createConnection()
{
    const QString server    = "https://main-sumo-prod.radix.equinor.com";
    const QString authority = "https://login.microsoftonline.com/3aa4a235-b6e2-48d5-9195-7fcf05b459b0";
    const QString scopes    = "9e5443dd-3431-4690-9617-31eed61cb55a/.default";
    const QString clientId  = "d57a8f87-4e28-4391-84d6-34356d5876a2";

    m_sumoConnector = new RimSumoConnector( qApp, server, authority, scopes, clientId );

    connect( m_sumoConnector, SIGNAL( tokenReady( const QString& ) ), this, SLOT( onTokenReady( const QString& ) ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SimpleDialog::onAuthClicked()
{
    createConnection();
    m_sumoConnector->requestToken();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SimpleDialog::onFieldsClicked()
{
    m_sumoConnector->requestCasesForField( "Drogon" );
    m_sumoConnector->fields();

    label->setText( "Requesting fields (see log for response" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SimpleDialog::onTokenReady( const QString& token )
{
    RiaLogging::info( "Token ready: " + token );
}

void SimpleDialog::onOkClicked()
{
    qDebug( "OK button clicked" );
    accept();
}

void SimpleDialog::onCancelClicked()
{
    qDebug( "Cancel button clicked" );
    reject();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicSumoDataFeature::onActionTriggered( bool isChecked )
{
    SimpleDialog dialog;
    dialog.exec();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicSumoDataFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "SUMO" );
}
