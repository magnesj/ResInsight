//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2014 Ceetron Solutions AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "cafPdmUiToolButtonCallbackEditor.h"

#include "cafPdmFieldHandle.h"
#include "cafPdmLogger.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiObjectHandle.h"

#include <QVariant>

#include <set>

namespace caf
{
CAF_PDM_UI_FIELD_EDITOR_SOURCE_INIT( PdmUiToolButtonCallbackEditor );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static std::set<QString> supportedAttributes()
{
    return { "callback" };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static void validateAttributes( const std::map<QString, QVariant>& attributes )
{
    for ( const auto& [key, value] : attributes )
    {
        if ( supportedAttributes().find( key ) == supportedAttributes().end() )
        {
            PdmLogger::warning(
                QString( "PdmUiToolButtonCallbackEditor: Unsupported attribute '%1' set on field.\n"
                         "Supported attributes are: %2" )
                    .arg( key )
                    .arg( QStringList( supportedAttributes().begin(), supportedAttributes().end() ).join( ", " ) ) );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static void extractAttributes( const std::map<QString, QVariant>&      attributes,
                               PdmUiToolButtonCallbackEditorAttribute* targetAttr )
{
    validateAttributes( attributes );

    auto it = attributes.find( "callback" );
    if ( it != attributes.end() )
    {
        if ( it->second.canConvert<std::function<void()>>() )
        {
            targetAttr->m_onClickedCallback = it->second.value<std::function<void()>>();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmUiToolButtonCallbackEditor::configureAndUpdateUi( const QString& uiConfigName )
{
    CAF_ASSERT( !m_toolButton.isNull() );

    if ( auto ic = uiField()->uiIcon( uiConfigName ) )
    {
        m_toolButton->setIcon( *ic );
    }

    QString buttonText = uiField()->uiName( uiConfigName );
    m_toolButton->setText( buttonText );

    m_toolButton->setEnabled( !uiField()->isUiReadOnly( uiConfigName ) );
    m_toolButton->setToolTip( uiField()->uiToolTip( uiConfigName ) );

    // First try to get attributes from the map-based system
    auto attributes = uiField()->fieldHandle()->uiCapability()->attributes( uiConfigName );
    extractAttributes( attributes, &m_attributes );

    // Fall back to old defineEditorAttribute method if callback not set via map
    if ( !m_attributes.m_onClickedCallback )
    {
        if ( auto pdmUiOjectHandle = uiObj( uiField()->fieldHandle()->ownerObject() ) )
        {
            pdmUiOjectHandle->editorAttribute( uiField()->fieldHandle(), uiConfigName, &m_attributes );
        }
    }

    m_toolButton->setCheckable( false );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* PdmUiToolButtonCallbackEditor::createEditorWidget( QWidget* parent )
{
    m_toolButton = new QToolButton( parent );
    connect( m_toolButton, SIGNAL( clicked( bool ) ), this, SLOT( slotClicked( bool ) ) );
    return m_toolButton;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* PdmUiToolButtonCallbackEditor::createLabelWidget( QWidget* parent )
{
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmUiToolButtonCallbackEditor::slotClicked( bool checked )
{
    if ( m_attributes.m_onClickedCallback )
    {
        m_attributes.m_onClickedCallback();
    }
}

} // end namespace caf
