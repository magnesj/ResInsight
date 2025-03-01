#include "RimOpmDeckGui.h"

#include "cafPdmUiListEditor.h"
#include "cafPdmUiTableViewEditor.h"
#include "cafPdmUiTextEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"
#include "cafSelectionManager.h"

CAF_PDM_SOURCE_INIT( PdmDeckItem, "PdmDeckItem" );
CAF_PDM_SOURCE_INIT( PdmDeckRecord, "PdmDeckRecord" );
CAF_PDM_SOURCE_INIT( PdmDeckKeyword, "PdmDeckKeyword" );
CAF_PDM_SOURCE_INIT( PdmDeck, "PdmDeck" );

//--------------------------------------------------------------------------------------------------
/// PdmDeckItem Implementation
//--------------------------------------------------------------------------------------------------
PdmDeckItem::PdmDeckItem()
{
    CAF_PDM_InitObject( "Deck Item", "", "", "" );

    CAF_PDM_InitField( &m_name, "Name", QString(), "Name", "", "", "" );
    CAF_PDM_InitField( &m_dataType, "DataType", QString(), "Data Type", "", "", "" );
    CAF_PDM_InitField( &m_valueRepresentation, "ValueRepresentation", QString(), "Representation", "", "", "" );
    CAF_PDM_InitField( &m_dataSize, "DataSize", size_t( 0 ), "Size", "", "", "" );
    CAF_PDM_InitField( &m_defaultApplied, "DefaultApplied", false, "Default Applied", "", "", "" );
    CAF_PDM_InitField( &m_stringValues, "StringValues", {}, "String Values", "", "", "" );
    CAF_PDM_InitField( &m_doubleValues, "DoubleValues", std::vector<double>(), "Double Values", "", "", "" );
    CAF_PDM_InitField( &m_intValues, "IntValues", std::vector<int>(), "Int Values", "", "", "" );
}

PdmDeckItem::PdmDeckItem( const Opm::DeckItem& deckItem )
    : PdmDeckItem()
{
    setFromDeckItem( deckItem );
}

void PdmDeckItem::setFromDeckItem( const Opm::DeckItem& deckItem )
{
    /*
        m_name           = QString::fromStdString( deckItem.name() );
        m_dataSize       = deckItem.size();
        m_defaultApplied = deckItem.defaultApplied();

        // Determine type and extract values
        if ( deckItem.defaultApplied() )
        {
            m_valueRepresentation = "Default Applied";
        }

        // Handle different types of data
        if ( deckItem.dataSize() > 0 )
        {
            if ( deckItem.isInt() )
            {
                m_dataType = "Integer";
                m_intValues.v().clear();
                for ( size_t i = 0; i < deckItem.size(); ++i )
                {
                    m_intValues.v().push_back( deckItem.get<int>( i ) );
                }
            }
            else if ( deckItem.isDouble() )
            {
                m_dataType = "Double";
                m_doubleValues.v().clear();
                for ( size_t i = 0; i < deckItem.size(); ++i )
                {
                    m_doubleValues.v().push_back( deckItem.get<double>( i ) );
                }
            }
            else if ( deckItem.isString() )
            {
                m_dataType = "String";
                m_stringValues.v().clear();
                for ( size_t i = 0; i < deckItem.size(); ++i )
                {
                    m_stringValues.v().append( QString::fromStdString( deckItem.get<std::string>( i ) ) );
                }
            }
            else
            {
                m_dataType = "Unknown";
            }
        }
        else
        {
            m_dataType = "Empty";
        }
    */
}

void PdmDeckItem::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_name );
    uiOrdering.add( &m_dataType );
    uiOrdering.add( &m_dataSize );
    uiOrdering.add( &m_defaultApplied );

    // Add appropriate value field based on data type
    if ( m_dataType() == "String" )
    {
        uiOrdering.add( &m_stringValues );
    }
    else if ( m_dataType() == "Double" )
    {
        uiOrdering.add( &m_doubleValues );
    }
    else if ( m_dataType() == "Integer" )
    {
        uiOrdering.add( &m_intValues );
    }
}

QList<caf::PdmOptionItemInfo> PdmDeckItem::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;
    return options;
}

void PdmDeckItem::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    // Handle field changes if needed
}

//--------------------------------------------------------------------------------------------------
/// PdmDeckRecord Implementation
//--------------------------------------------------------------------------------------------------
PdmDeckRecord::PdmDeckRecord()
{
    CAF_PDM_InitObject( "Deck Record", "", "", "" );

    CAF_PDM_InitField( &m_itemCount, "ItemCount", size_t( 0 ), "Item Count", "", "", "" );
    CAF_PDM_InitFieldNoDefault( &m_items, "Items", "Items", "", "", "" );
}

PdmDeckRecord::PdmDeckRecord( const Opm::DeckRecord& deckRecord )
    : PdmDeckRecord()
{
    setFromDeckRecord( deckRecord );
}

void PdmDeckRecord::setFromDeckRecord( const Opm::DeckRecord& deckRecord )
{
    // Clear existing items
    m_items.deleteChildren();

    // Create new items
    for ( size_t i = 0; i < deckRecord.size(); ++i )
    {
        PdmDeckItem* pdmItem = new PdmDeckItem( deckRecord.getItem( i ) );
        m_items.push_back( pdmItem );
    }
}

void PdmDeckRecord::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_itemCount );
    uiOrdering.add( &m_items );
}

//--------------------------------------------------------------------------------------------------
/// PdmDeckKeyword Implementation
//--------------------------------------------------------------------------------------------------
PdmDeckKeyword::PdmDeckKeyword()
{
    CAF_PDM_InitObject( "Deck Keyword", "", "", "" );

    CAF_PDM_InitField( &m_name, "Name", QString(), "Name", "", "", "" );
    CAF_PDM_InitField( &m_fileName, "FileName", QString(), "File", "", "", "" );
    CAF_PDM_InitField( &m_lineNumber, "LineNumber", -1, "Line", "", "", "" );
    CAF_PDM_InitField( &m_isDataKeyword, "IsDataKeyword", false, "Is Data Keyword", "", "", "" );
    CAF_PDM_InitField( &m_recordCount, "RecordCount", size_t( 0 ), "Record Count", "", "", "" );
    CAF_PDM_InitFieldNoDefault( &m_records, "Records", "Records", "", "", "" );
}

PdmDeckKeyword::PdmDeckKeyword( const Opm::DeckKeyword& keyword )
    : PdmDeckKeyword()
{
    setFromDeckKeyword( keyword );
}

void PdmDeckKeyword::setFromDeckKeyword( const Opm::DeckKeyword& keyword )
{
    m_name = QString::fromStdString( keyword.name() );
    /*
        m_fileName      = QString::fromStdString( keyword.getFileName() );
        m_lineNumber    = keyword.getLineNumber();
    */
    m_isDataKeyword = keyword.isDataKeyword();
    m_recordCount   = keyword.size();

    m_records.deleteChildren();

    // Create new records
    for ( size_t i = 0; i < keyword.size(); ++i )
    {
        PdmDeckRecord* pdmRecord = new PdmDeckRecord( keyword.getRecord( i ) );
        m_records.push_back( pdmRecord );
    }
}

void PdmDeckKeyword::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // Header section
    caf::PdmUiGroup* headerGroup = uiOrdering.addNewGroup( "Keyword Info" );
    headerGroup->add( &m_name );
    headerGroup->add( &m_fileName );
    headerGroup->add( &m_lineNumber );
    headerGroup->add( &m_isDataKeyword );
    headerGroup->add( &m_recordCount );

    // Records section
    uiOrdering.add( &m_records );
}

void PdmDeckKeyword::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_records )
    {
        caf::PdmUiTreeSelectionEditorAttribute* attr = dynamic_cast<caf::PdmUiTreeSelectionEditorAttribute*>( attribute );
        if ( attr )
        {
            attr->showTextFilter        = true;
            attr->showToggleAllCheckbox = true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// PdmDeck Implementation
//--------------------------------------------------------------------------------------------------
PdmDeck::PdmDeck()
{
    CAF_PDM_InitObject( "Simulation Deck", "", "", "" );

    CAF_PDM_InitField( &m_fileName, "FileName", QString(), "File Name", "", "", "" );
    CAF_PDM_InitField( &m_keywordCount, "KeywordCount", size_t( 0 ), "Keyword Count", "", "", "" );
    CAF_PDM_InitFieldNoDefault( &m_keywords, "Keywords", "Keywords", "", "", "" );
    CAF_PDM_InitField( &m_activeKeywords, "ActiveKeywords", {}, "Active Keywords", "", "", "" );
}

PdmDeck::PdmDeck( const Opm::Deck& deck )
    : PdmDeck()
{
    setFromDeck( deck );
}

void PdmDeck::setFromDeck( const Opm::Deck& deck )
{
    m_keywords.deleteChildren();

    // Get file name from any keyword if available
    /*
        if ( !deck.getKeywordList().empty() )
        {
            const auto& firstKeyword = deck.getKeywordList().front();
            if ( !firstKeyword.getFileName().empty() )
            {
                m_fileName = QString::fromStdString( firstKeyword.getFileName() );
            }
        }
    */

    m_keywordCount = deck.size();

    // Create keyword objects
    for ( size_t i = 0; i < deck.size(); ++i )
    {
        PdmDeckKeyword* pdmKeyword = new PdmDeckKeyword( deck[i] );
        m_keywords.push_back( pdmKeyword );
    }

    // Create list of available keywords for filtering
    /*
        QStringList keywordNames;
        for ( const auto& keyword : deck.getKeywordList() )
        {
            QString keywordName = QString::fromStdString( keyword.name() );
            if ( !keywordNames.contains( keywordName ) )
            {
                keywordNames.append( keywordName );
            }
        }
    keywordNames.sort();
    m_activeKeywords = keywordNames;
    */
}

void PdmDeck::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // Header section
    caf::PdmUiGroup* headerGroup = uiOrdering.addNewGroup( "Deck Information" );
    headerGroup->add( &m_fileName );
    headerGroup->add( &m_keywordCount );
    headerGroup->add( &m_activeKeywords );

    // Keywords section
    uiOrdering.add( &m_keywords );
}

void PdmDeck::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_keywords )
    {
        caf::PdmUiTreeSelectionEditorAttribute* attr = dynamic_cast<caf::PdmUiTreeSelectionEditorAttribute*>( attribute );
        if ( attr )
        {
            attr->showTextFilter        = true;
            attr->showToggleAllCheckbox = true;
        }
    }
    else if ( field == &m_activeKeywords )
    {
        /*
                caf::PdmUiListEditorAttribute* attr = dynamic_cast<caf::PdmUiListEditorAttribute*>( attribute );
                if ( attr )
                {
                    attr->m_heightHint = 100;
                }
        */
    }
}

QList<caf::PdmOptionItemInfo> PdmDeck::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    return options;
}

void PdmDeck::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_activeKeywords )
    {
        // Could implement filtering based on active keywords
    }
}
