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

#include <algorithm>
#include <vector>

namespace caf
{

// Forward declaration
template <class T>
class AppEnum;

//==================================================================================================
/// A private class to handle the instance of the mapping vector for AppEnum.
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

        bool isMatching( const QString& text ) const noexcept
        {
            return ( text == m_text || m_aliases.contains( text ) );
        }

        T           m_enumVal;
        QString     m_text;
        QString     m_uiText;
        QStringList m_aliases;
    };

public:
    void addItem( T enumVal, const QString& text, const QString& uiText, const QStringList& aliases )
    {
        // Pre-validate input to avoid partial state on failure
        const QString trimmedText = text.trimmed();
        if ( trimmedText.isEmpty() )
        {
            CAF_ASSERT( false && "Text cannot be empty" );
            return;
        }

        // Check for duplicate main text
        auto textExists = std::any_of( m_mapping.cbegin(),
                                       m_mapping.cend(),
                                       [&trimmedText]( const EnumData& enumData )
                                       { return trimmedText == enumData.m_text; } );
        CAF_ASSERT( !textExists && "Duplicate text found" );

        // Make sure the alias text is unique for enum
        for ( const auto& alias : aliases )
        {
            auto aliasExists = std::any_of( m_mapping.cbegin(),
                                            m_mapping.cend(),
                                            [&alias]( const EnumData& enumData ) { return enumData.isMatching( alias ); } );
            CAF_ASSERT( !aliasExists && "Duplicate alias found" );
        }

        // Check for duplicate enum value
        auto enumExists = std::any_of( m_mapping.cbegin(),
                                       m_mapping.cend(),
                                       [enumVal]( const EnumData& enumData ) { return enumVal == enumData.m_enumVal; } );
        CAF_ASSERT( !enumExists && "Duplicate enum value found" );

        // Reserve space to avoid potential reallocation during emplace_back
        if ( m_mapping.size() == m_mapping.capacity() )
        {
            m_mapping.reserve( m_mapping.size() * 2 + 1 );
        }

        // Make sure the text is trimmed, as this text is streamed to XML and will be trimmed when read back
        // from XML text https://github.com/OPM/ResInsight/issues/7829
        m_mapping.emplace_back( enumVal, trimmedText, uiText, aliases );
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

    void setDefault( T defaultEnumValue ) noexcept
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
        else if ( !m_mapping.empty() )
        {
            return m_mapping[0].m_enumVal;
        }
        else
        {
            CAF_ASSERT( false && "No mapping available and no default set" );
            return T();
        }
    }

    bool isValid( const QString& text ) const
    {
        auto it = std::find_if( m_mapping.cbegin(),
                                m_mapping.cend(),
                                [&text]( const EnumData& enumData ) { return text == enumData.m_text; } );
        return it != m_mapping.cend();
    }

    size_t size() const noexcept { return m_mapping.size(); }
    bool   empty() const noexcept { return m_mapping.empty(); }

    bool enumVal( T& value, const QString& text ) const
    {
        const QString trimmedText = text.trimmed();

        auto it = std::find_if( m_mapping.cbegin(),
                                m_mapping.cend(),
                                [&trimmedText]( const EnumData& enumData ) { return enumData.isMatching( trimmedText ); } );

        if ( it != m_mapping.cend() )
        {
            value = it->m_enumVal;
            return true;
        }
        else
        {
            value = defaultValue();
            return false;
        }
    }

    bool enumVal( T& value, size_t index ) const
    {
        if ( index < m_mapping.size() )
        {
            value = m_mapping[index].m_enumVal;
            return true;
        }
        else
        {
            value = defaultValue();
            return false;
        }
    }

    size_t index( T enumValue ) const
    {
        auto it = std::find_if( m_mapping.cbegin(),
                                m_mapping.cend(),
                                [enumValue]( const EnumData& enumData ) { return enumValue == enumData.m_enumVal; } );

        return it != m_mapping.cend() ? static_cast<size_t>( std::distance( m_mapping.cbegin(), it ) ) : m_mapping.size();
    }

    QString uiText( T value ) const
    {
        auto it = std::find_if( m_mapping.cbegin(),
                                m_mapping.cend(),
                                [value]( const EnumData& enumData ) { return value == enumData.m_enumVal; } );

        return it != m_mapping.cend() ? it->m_uiText : QString();
    }

    QStringList uiTexts() const
    {
        QStringList uiTextList;
        uiTextList.reserve( static_cast<int>( m_mapping.size() ) );

        for ( const auto& enumData : m_mapping )
        {
            uiTextList.append( enumData.m_uiText );
        }
        return uiTextList;
    }

    QString text( T value ) const
    {
        auto it = std::find_if( m_mapping.cbegin(),
                                m_mapping.cend(),
                                [value]( const EnumData& enumData ) { return value == enumData.m_enumVal; } );

        return it != m_mapping.cend() ? it->m_text : QString();
    }

private:
    AppEnumMapper()
        : m_defaultValue( T() )
        , m_defaultValueIsSet( false )
    {
        // Reserve some initial capacity to reduce allocations
        m_mapping.reserve( 8 );
    }

    std::vector<EnumData> m_mapping;
    T                     m_defaultValue;
    bool                  m_defaultValueIsSet;
};

} // namespace caf
