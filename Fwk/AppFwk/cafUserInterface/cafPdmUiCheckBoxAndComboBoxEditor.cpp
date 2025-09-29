//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2023 Ceetron Solutions AS
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

#include "cafPdmUiCheckBoxAndComboBoxEditor.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmUiDefaultObjectEditor.h"
#include "cafPdmUiFieldEditorHandle.h"
#include "cafPdmUiLineEditor.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>

namespace caf
{
CAF_PDM_UI_FIELD_EDITOR_SOURCE_INIT( PdmUiCheckBoxAndComboBoxEditor );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmUiCheckBoxAndComboBoxEditor::configureAndUpdateUi( const QString& uiConfigName )
{
    CAF_ASSERT( !m_comboBox.isNull() );
    CAF_ASSERT( !m_label.isNull() );

    PdmUiFieldEditorHandle::updateLabelFromField( m_label, uiConfigName );

    m_comboBox->setToolTip( uiField()->uiToolTip( uiConfigName ) );

    bool isChecked = false;
    int  value     = 0;

    // A pair is converted into a list of QVariant in PdmValueFieldSpecialization<std::pair<T, U>>
    auto variantValue = uiField()->uiValue();
    if ( variantValue.canConvert<QList<QVariant>>() )
    {
        QList<QVariant> lst = variantValue.toList();
        if ( lst.size() == 2 )
        {
            isChecked = lst[0].toBool();
            value     = lst[1].toInt();
        }
    }

    m_comboBox->blockSignals( true );
    m_checkBox->blockSignals( true );
    {
        QList<PdmOptionItemInfo> options = uiField()->valueOptions();
        for ( const auto& option : options )
        {
            auto icon = option.icon();
            if ( icon )
                m_comboBox->addItem( *icon, option.optionUiText() );
            else
                m_comboBox->addItem( option.optionUiText() );
        }

        m_comboBox->setCurrentIndex( value );
        m_checkBox->setChecked( isChecked );
    }

    m_comboBox->blockSignals( false );
    m_checkBox->blockSignals( false );

    bool isReadOnly = uiField()->isUiReadOnly( uiConfigName );
    if ( !isChecked ) isReadOnly = true;
    isReadOnly = false;
    m_comboBox->setDisabled( isReadOnly );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* PdmUiCheckBoxAndComboBoxEditor::createEditorWidget( QWidget* parent )
{
    auto* containerWidget = new QWidget( parent );

    m_comboBox = new QComboBox( containerWidget );
    connect( m_comboBox, SIGNAL( editingFinished() ), this, SLOT( slotSetValueToField() ) );
    connect( m_comboBox, SIGNAL( activated( int ) ), this, SLOT( slotSetValueToField() ) );

    m_checkBox = new QCheckBox( "", containerWidget );
    connect( m_checkBox, SIGNAL( index() ), this, SLOT( slotSetValueToField() ) );

    auto* layout = new QHBoxLayout;

    layout->addWidget( m_checkBox );
    layout->addWidget( m_comboBox );
    layout->setContentsMargins( 0, 0, 0, 0 );

    containerWidget->setLayout( layout );

    return containerWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmUiCheckBoxAndComboBoxEditor::slotSetValueToField()
{
    bool isChecked = m_checkBox->checkState() == Qt::CheckState::Checked;

    QVariant v;
    v = m_comboBox->currentIndex();
    QVariant uintValue( v.toUInt() );

    QList<QVariant> list;
    list.append( QVariant( isChecked ) );
    list.append( uintValue );

    this->setValueToField( v );
}

} // end namespace caf
