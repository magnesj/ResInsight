// Out-of-line implementations of AppEnumMapperBase. By keeping the loops, lookups, and QString
// machinery here in one TU instead of in the AppEnum<T> class template, every method is compiled
// once instead of once per enum type.

#include "cafAppEnum.h"

#include "cafAssert.h"

namespace caf
{

bool AppEnumMapperBase::EnumData::isMatching( const QString& text ) const
{
    return text == m_text || m_aliases.contains( text );
}

void AppEnumMapperBase::addItem( int enumVal, const QString& text, QString uiText, const QStringList& aliases )
{
    // Make sure the alias text is unique for enum
    for ( const auto& alias : aliases )
    {
        for ( const auto& enumData : m_mapping )
        {
            CAF_ASSERT( !enumData.isMatching( alias ) );
        }
    }

    // Make sure the text is trimmed, as this text is streamed to XML and will be trimmed when read back
    // from XML text https://github.com/OPM/ResInsight/issues/7829
    m_mapping.push_back( { enumVal, text.trimmed(), uiText, aliases } );
}

void AppEnumMapperBase::setDefault( int defaultEnumValue )
{
    m_defaultValue      = defaultEnumValue;
    m_defaultValueIsSet = true;
}

int AppEnumMapperBase::defaultValue() const
{
    if ( m_defaultValueIsSet ) return m_defaultValue;
    // Matches prior behavior: undefined when m_mapping is empty.
    return m_mapping[0].m_enumVal;
}

bool AppEnumMapperBase::isValid( const QString& text ) const
{
    for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
    {
        if ( text == m_mapping[idx].m_text ) return true;
    }
    return false;
}

size_t AppEnumMapperBase::size() const
{
    return m_mapping.size();
}

bool AppEnumMapperBase::enumVal( int& value, const QString& text ) const
{
    value = defaultValue();

    QString trimmedText = text.trimmed();

    for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
    {
        if ( m_mapping[idx].isMatching( trimmedText ) )
        {
            value = m_mapping[idx].m_enumVal;
            return true;
        }
    }
    return false;
}

bool AppEnumMapperBase::enumVal( int& value, size_t index ) const
{
    value = defaultValue();
    if ( index < m_mapping.size() )
    {
        value = m_mapping[index].m_enumVal;
        return true;
    }
    return false;
}

size_t AppEnumMapperBase::index( int enumValue ) const
{
    size_t idx;
    for ( idx = 0; idx < m_mapping.size(); ++idx )
    {
        if ( enumValue == m_mapping[idx].m_enumVal ) return idx;
    }
    return idx; // returns size() if not found, matching prior behavior
}

QString AppEnumMapperBase::uiText( int value ) const
{
    for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
    {
        if ( value == m_mapping[idx].m_enumVal ) return m_mapping[idx].m_uiText;
    }
    return "";
}

QStringList AppEnumMapperBase::uiTexts() const
{
    QStringList uiTextList;
    for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
    {
        uiTextList.append( m_mapping[idx].m_uiText );
    }
    return uiTextList;
}

QString AppEnumMapperBase::text( int value ) const
{
    for ( size_t idx = 0; idx < m_mapping.size(); ++idx )
    {
        if ( value == m_mapping[idx].m_enumVal ) return m_mapping[idx].m_text;
    }
    return "";
}

} // namespace caf
