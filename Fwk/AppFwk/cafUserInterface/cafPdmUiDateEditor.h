//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2017 Ceetron Solutions AS
//
//   This library may be used under the terms of the GNU General Public License or
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

#pragma once

#include "cafPdmUiFieldLabelEditorHandle.h"

#include <QDateTimeEdit>
#include <QLabel>
#include <QPointer>
#include <QString>
#include <QWidget>

namespace caf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class PdmUiDateEditor : public PdmUiFieldLabelEditorHandle
{
    Q_OBJECT
    CAF_PDM_UI_FIELD_EDITOR_HEADER_INIT;

public:
    PdmUiDateEditor() {}
    ~PdmUiDateEditor() override {}

    // Attribute key constants for compile-time safety and discoverability
    struct Keys
    {
        // std::function<QString()>. Use setFormat() to set either a static string or a callback
        static inline const QString DATE_FORMAT = QStringLiteral( "dateFormat" );
    };

    // Set of all supported attributes for validation
    inline static const std::set<QString> SUPPORTED_ATTRIBUTES = { Keys::DATE_FORMAT };

    // Set a static date format, e.g. "dd MMM yyyy"
    static void setFormat( PdmUiItem* uiItem, const QString& format );

    // Set a callback producing the date format, for formats that depend on the current state
    static void setFormat( PdmUiItem* uiItem, std::function<QString()> formatCallback );

    // Returns the date format configured by attributes, or an empty string if none is set
    static QString dateFormat( const PdmUiItem* uiItem, const QString& uiConfigName );

protected:
    QWidget* createEditorWidget( QWidget* parent ) override;
    void     configureAndUpdateUi( const QString& uiConfigName ) override;

protected slots:
    void slotEditingFinished();

private:
    QPointer<QDateTimeEdit> m_dateEdit;
};

} // end namespace caf
