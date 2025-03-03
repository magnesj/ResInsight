#include "RimOpmDeckGui.h"

#include "cafPdmUiListEditor.h"
#include "cafPdmUiPushButtonEditor.h"
#include "cafPdmUiTableViewEditor.h"
#include "cafPdmUiTextEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"
#include "cafSelectionManager.h"

#include "RiaLogging.h"
#include "opm/input/eclipse/Deck/Deck.hpp"
#include "opm/input/eclipse/Deck/DeckItem.hpp"
#include "opm/input/eclipse/Deck/DeckKeyword.hpp"
#include "opm/input/eclipse/Deck/DeckRecord.hpp"
#include "opm/input/eclipse/Parser/Parser.hpp"

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

    CAF_PDM_InitField( &m_dataType, "DataType", QString(), "Data Type", "", "", "" );
    CAF_PDM_InitField( &m_valueRepresentation, "ValueRepresentation", QString(), "Representation", "", "", "" );
    CAF_PDM_InitField( &m_dataSize, "DataSize", size_t( 0 ), "Size", "", "", "" );
    CAF_PDM_InitField( &m_defaultApplied, "DefaultApplied", false, "Default Applied", "", "", "" );
    CAF_PDM_InitField( &m_stringValues, "StringValues", {}, "String Values", "", "", "" );
    CAF_PDM_InitField( &m_doubleValues, "DoubleValues", std::vector<double>(), "Double Values", "", "", "" );
    m_doubleValues.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_intValues, "IntValues", std::vector<int>(), "Int Values", "", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_fractureCreationSummary, "FractureCreationSummary", "Generated Fractures" );
    m_fractureCreationSummary.registerGetMethod( this, &PdmDeckItem::fieldText );
    m_fractureCreationSummary.registerSetMethod( this, &PdmDeckItem::setFieldText );
    m_fractureCreationSummary.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::TOP );
    m_fractureCreationSummary.uiCapability()->setUiEditorTypeName( caf::PdmUiTextEditor::uiEditorTypeName() );
}

PdmDeckItem::PdmDeckItem( Opm::DeckItem& deckItem )
    : PdmDeckItem()
{
    setFromDeckItem( deckItem );
}

void PdmDeckItem::setFromDeckItem( Opm::DeckItem& deckItem )
{
    try
    {
        setName( QString::fromStdString( deckItem.name() ) );
        m_dataSize = deckItem.data_size();
        // m_defaultApplied = deckItem.defaultApplied();

        // assign the deck item to the member variable

        // Determine type and extract values
        /*
            if ( deckItem.defaultApplied() )
            {
                m_valueRepresentation = "Default Applied";
            }
        */

        // Handle different types of data
        if ( deckItem.data_size() > 0 )
        {
            if ( deckItem.is_int() )
            {
                m_dataType = "Integer";
                m_intValues.v().clear();
                for ( size_t i = 0; i < deckItem.data_size(); ++i )
                {
                    if ( !deckItem.hasValue( i ) ) continue;
                    m_intValues.v().push_back( deckItem.get<int>( i ) );
                }
            }
            else if ( deckItem.is_double() )
            {
                m_dataType = "Double";
                m_doubleValues.v().clear();
                for ( size_t i = 0; i < deckItem.data_size(); ++i )
                {
                    if ( !deckItem.hasValue( i ) ) continue;
                    m_doubleValues.v().push_back( deckItem.get<double>( i ) );
                }
            }
            else if ( deckItem.is_string() )
            {
                m_dataType = "String";
                m_stringValues.v().clear();
                for ( size_t i = 0; i < deckItem.data_size(); ++i )
                {
                    if ( !deckItem.hasValue( i ) ) continue;

                    m_stringValues.v().push_back( QString::fromStdString( deckItem.get<std::string>( i ) ) );
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
    }
    catch ( ... )
    {
        // QString error = QString( "Error: %1" ).arg( e->what() );
        QString error = "exception";
        RiaLogging::error( error );
    }
}

void PdmDeckItem::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
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

    uiOrdering.add( &m_fractureCreationSummary );

    uiOrdering.skipRemainingFields();
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
///
//--------------------------------------------------------------------------------------------------
QString PdmDeckItem::fieldText() const
{
    // TODO: Find a way to store the reference to the deck item, and read out the values from it
    // This is currently not possible, as the deck item is not stored in the PdmDeckItem object

    QString text;
    /*
        if ( m_deckItem )
        {
            if ( m_deckItem->data_size() > 0 )
            {
                if ( m_deckItem->is_int() )
                {
                    for ( size_t i = 0; i < m_deckItem->data_size(); ++i )
                    {
                        if ( !m_deckItem->hasValue( i ) ) continue;
                        text += QString( " %1" ).arg( m_deckItem->get<int>( i ) );
                    }
                }
                else if ( m_deckItem->is_double() )
                {
                    for ( size_t i = 0; i < m_deckItem->data_size(); ++i )
                    {
                        if ( !m_deckItem->hasValue( i ) ) continue;
                        text += QString( " %1" ).arg( m_deckItem->get<double>( i ) );
                    }
                }
                else if ( m_deckItem->is_string() )
                {
                    for ( size_t i = 0; i < m_deckItem->data_size(); ++i )
                    {
                        if ( !m_deckItem->hasValue( i ) ) continue;
                        text += QString( " %1" ).arg( QString::fromStdString( m_deckItem->get<std::string>( i ) ) );
                    }
                }

                / *
                else if ( nonConst.is_double() )
                {
                    m_dataType = "Double";
                    m_doubleValues.v().clear();
                    for ( size_t i = 0; i < deckItem.data_size(); ++i )
                    {
                        if ( !deckItem.hasValue( i ) ) continue;
                        m_doubleValues.v().push_back( deckItem.get<double>( i ) );
                    }
                }
                else if ( nonConst.is_string() )
                {
                    m_dataType = "String";
                    m_stringValues.v().clear();
                    for ( size_t i = 0; i < deckItem.data_size(); ++i )
                    {
                        if ( !deckItem.hasValue( i ) ) continue;

                        m_stringValues.v().push_back( QString::fromStdString( deckItem.get<std::string>( i ) ) );
                    }
                }
    * /
            }
        }
    */

    return QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmDeckItem::setFieldText( const QString& text )
{
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

PdmDeckRecord::PdmDeckRecord( Opm::DeckRecord& deckRecord )
    : PdmDeckRecord()
{
    setFromDeckRecord( deckRecord );
}

void PdmDeckRecord::setFromDeckRecord( Opm::DeckRecord& deckRecord )
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
    setName( QString::fromStdString( keyword.name() ) );
    /*
        m_fileName      = QString::fromStdString( keyword.getFileName() );
        m_lineNumber    = keyword.getLineNumber();
    */
    m_isDataKeyword = keyword.isDataKeyword();
    m_recordCount   = keyword.size();

    m_records.deleteChildren();

    auto nonConst = const_cast<Opm::DeckKeyword&>( keyword );

    // Create new records
    for ( size_t i = 0; i < keyword.size(); ++i )
    {
        PdmDeckRecord* pdmRecord = new PdmDeckRecord( nonConst.getRecord( i ) );
        m_records.push_back( pdmRecord );
    }
}

void PdmDeckKeyword::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // Header section
    caf::PdmUiGroup* headerGroup = uiOrdering.addNewGroup( "Keyword Info" );
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

    CAF_PDM_InitField( &m_loadData, "LoadData", false, "Active Keywords", "", "", "" );
    caf::PdmUiPushButtonEditor::configureEditorLabelLeft( &m_loadData );
}

void PdmDeck::setFromDeck()
{
    m_keywords.deleteChildren();

    m_keywordCount = m_deck.size();

    // Create keyword objects
    for ( size_t i = 0; i < m_deck.size(); ++i )
    {
        auto pdmKeyword = new PdmDeckKeyword( m_deck[i] );
        m_keywords.push_back( pdmKeyword );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmDeck::loadDeck( const QString& fileName )
{
    if ( fileName.isEmpty() ) return;

    // Parse deck using OPM parser
    std::string fileNameStd = fileName.toStdString();
    Opm::Parser parser;

    m_deck = parser.parseFile( fileNameStd );
    setFromDeck();
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
    else if ( changedField == &m_loadData )
    {
        loadDeck( m_fileName() );
        m_loadData = false;
    }
}
