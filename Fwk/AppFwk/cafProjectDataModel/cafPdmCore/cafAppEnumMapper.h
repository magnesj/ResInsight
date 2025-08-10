//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
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

#pragma once

#include "cafAssert.h"

#include <QString>
#include <QStringList>

#include <vector>

namespace caf
{

// Forward declaration
template <class T>
class AppEnum;

//==================================================================================================
/// A private class to handle the instance of the mapping vector for AppEnum.
/// All access methods could have been placed directly in the AppEnum class,
/// but AppEnum implementation gets nicer this way.
/// The real core of this class is the vector map member and the static instance method
//==================================================================================================
template <class T>
class AppEnumMapper
{
private:
    class EnumData
    {
    public:
        EnumData( T enumVal, const QString& text, const QString& uiText, const QStringList& aliases )
            : m_enumVal( enumVal )
            , m_text( text )
            , m_uiText( uiText )
            , m_aliases( aliases )
        {
        }

        bool isMatching( const QString& text ) const { return ( text == m_text || m_aliases.contains( text ) ); }

        T           m_enumVal;
        QString     m_text;
        QString     m_uiText;
        QStringList m_aliases;
    };

public:
    void addItem( T enumVal, const QString& text, QString uiText, const QStringList& aliases )
    {
        // Make sure the alias text is unique for enum
        for ( const auto& alias : aliases )
        {
            for ( const auto& enumData : instance()->m_mapping )
            {
                CAF_ASSERT( !enumData.isMatching( alias ) );
            }
        }

        // Make sure the text is trimmed, as this text is streamed to XML and will be trimmed when read back
        // from XML text https://github.com/OPM/ResInsight/issues/7829
        instance()->m_mapping.push_back( EnumData( enumVal, text.trimmed(), uiText, aliases ) );
    }

    static AppEnumMapper* instance()
    {
        static AppEnumMapper storedInstance;
        static bool          isInitialized = false;
        if ( !isInitialized )
        {
            isInitialized = true;
            AppEnum<T>::setUp();
        }
        return &storedInstance;
    }

    void setDefault( T defaultEnumValue )
    {
        m_defaultValue      = defaultEnumValue;
        m_defaultValueIsSet = true;
    }

    T defaultValue() const
    {
        if ( m_defaultValueIsSet )
        {
            return m_defaultValue;
        }
        else
        {
            // CAF_ASSERT(m_mapping.size());
            return m_mapping[0].m_enumVal;
        }
    }

    bool isValid( const QString& text ) const
    {
        size_t idx;
        for ( idx = 0; idx < m_mapping.size(); ++idx )
        {
            if ( text == m_mapping[idx].m_text ) return true;
        }

        return false;
    }

    size_t size() const { return m_mapping.size(); }

    bool enumVal( T& value, const QString& text ) const
    {
        value = defaultValue();

        QString trimmedText = text.trimmed();

        for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
        {
            // Make sure the text parsed from a text stream is trimmed
            // https://github.com/OPM/ResInsight/issues/7829
            if ( m_mapping[idx].isMatching( trimmedText ) )
            {
                value = m_mapping[idx].m_enumVal;
                return true;
            }
        }
        return false;
    }

    bool enumVal( T& value, size_t index ) const
    {
        value = defaultValue();
        if ( index < m_mapping.size() )
        {
            value = m_mapping[index].m_enumVal;
            return true;
        }
        else
            return false;
    }

    size_t index( T enumValue ) const
    {
        size_t idx;
        for ( idx = 0; idx < m_mapping.size(); ++idx )
        {
            if ( enumValue == m_mapping[idx].m_enumVal ) return idx;
        }

        return idx;
    }

    QString uiText( T value ) const
    {
        size_t idx;
        for ( idx = 0; idx < m_mapping.size(); ++idx )
        {
            if ( value == m_mapping[idx].m_enumVal ) return m_mapping[idx].m_uiText;
        }
        return "";
    }

    QStringList uiTexts() const
    {
        QStringList uiTextList;
        size_t      idx;
        for ( idx = 0; idx < m_mapping.size(); ++idx )
        {
            uiTextList.append( m_mapping[idx].m_uiText );
        }
        return uiTextList;
    }

    QString text( T value ) const
    {
        size_t idx;
        for ( idx = 0; idx < m_mapping.size(); ++idx )
        {
            if ( value == m_mapping[idx].m_enumVal ) return m_mapping[idx].m_text;
        }
        return "";
    }

private:
    AppEnumMapper()
        : m_defaultValue( T() )
        , m_defaultValueIsSet( false )
    {
    }

    std::vector<EnumData> m_mapping;
    T                     m_defaultValue;
    bool                  m_defaultValueIsSet;
};

} // namespace caf
