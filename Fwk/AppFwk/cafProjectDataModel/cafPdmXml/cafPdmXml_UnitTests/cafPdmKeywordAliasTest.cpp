
#include "gtest/gtest.h"

#include "cafPdmChildArrayField.h"
#include "cafPdmDataValueField.h"
#include "cafPdmDefaultObjectFactory.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmPtrArrayField.h"
#include "cafPdmPtrField.h"
#include "cafPdmReferenceHelper.h"
#include "cafPdmXmlObjectHandle.h"
#include "cafPdmXmlObjectHandleMacros.h"

#include <memory>

// Compatibility baseline for class keyword aliases and field keyword aliases. These tests describe the behavior that
// must be preserved when the CAF_PDM_Init* macros are replaced by functions.

namespace
{
//==================================================================================================
/// Base class owning an aliased field, to verify alias lookup through an inherited field
//==================================================================================================
class AliasBaseObj : public caf::PdmObjectHandle, public caf::PdmXmlObjectHandle
{
    CAF_PDM_XML_HEADER_INIT;

public:
    AliasBaseObj()
        : PdmObjectHandle()
        , PdmXmlObjectHandle( this, false )
    {
        CAF_PDM_XML_InitField( &m_value, "Value" );
        m_value.registerKeywordAlias( "OldValue" );
        m_value.registerKeywordAlias( "OlderValue" );
        m_value = 0.0;
    }

    caf::PdmDataValueField<double> m_value;
};

//==================================================================================================
/// Class with two class keyword aliases
//==================================================================================================
class AliasItemObj : public AliasBaseObj
{
    CAF_PDM_XML_HEADER_INIT;

public:
    AliasItemObj() { CAF_PDM_XML_InitField( &m_name, "Name" ); }

    caf::PdmDataValueField<QString> m_name;
};

//==================================================================================================
/// Container with aliased child array field, and pointer fields referring to the children by path
//==================================================================================================
class AliasContainerObj : public caf::PdmObjectHandle, public caf::PdmXmlObjectHandle
{
    CAF_PDM_XML_HEADER_INIT;

public:
    AliasContainerObj()
        : PdmObjectHandle()
        , PdmXmlObjectHandle( this, false )
    {
        CAF_PDM_XML_InitField( &m_items, "Items" );
        m_items.registerKeywordAlias( "OldItems" );

        CAF_PDM_XML_InitField( &m_selectedItem, "SelectedItem" );
        CAF_PDM_XML_InitField( &m_selectedItems, "SelectedItems" );
    }

    caf::PdmChildArrayField<AliasItemObj*> m_items;
    caf::PdmPtrField<AliasItemObj*>        m_selectedItem;
    caf::PdmPtrArrayField<AliasItemObj*>   m_selectedItems;
};

//==================================================================================================
/// Alias collisions. The framework does not reject these, the tests document the lookup order.
//==================================================================================================
class AliasCollisionObj : public caf::PdmObjectHandle, public caf::PdmXmlObjectHandle
{
    CAF_PDM_XML_HEADER_INIT;

public:
    AliasCollisionObj()
        : PdmObjectHandle()
        , PdmXmlObjectHandle( this, false )
    {
        CAF_PDM_XML_InitField( &m_first, "First" );
        CAF_PDM_XML_InitField( &m_second, "Second" );
        CAF_PDM_XML_InitField( &m_third, "Third" );

        // Alias equal to the keyword of another field
        m_first.registerKeywordAlias( "Second" );

        // Same alias on two fields
        m_second.registerKeywordAlias( "Shared" );
        m_third.registerKeywordAlias( "Shared" );

        m_first  = 0;
        m_second = 0;
        m_third  = 0;
    }

    caf::PdmDataValueField<int> m_first;
    caf::PdmDataValueField<int> m_second;
    caf::PdmDataValueField<int> m_third;
};

CAF_PDM_XML_SOURCE_INIT( AliasBaseObj, "AliasBaseObj" );
CAF_PDM_XML_SOURCE_INIT( AliasItemObj, "AliasItemObj", "OldAliasItemObj", "OlderAliasItemObj" );
CAF_PDM_XML_SOURCE_INIT( AliasContainerObj, "AliasContainerObj", "OldAliasContainerObj" );
CAF_PDM_XML_SOURCE_INIT( AliasCollisionObj, "AliasCollisionObj" );

std::unique_ptr<AliasContainerObj> createContainerWithThreeItems()
{
    auto container = std::make_unique<AliasContainerObj>();

    for ( int i = 0; i < 3; i++ )
    {
        auto item     = new AliasItemObj;
        item->m_name  = QString( "Item %1" ).arg( i );
        item->m_value = 10.0 * ( i + 1 );
        container->m_items.push_back( item );
    }

    container->m_selectedItem = container->m_items[1];
    container->m_selectedItems.push_back( container->m_items[2] );
    container->m_selectedItems.push_back( container->m_items[0] );

    return container;
}

} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, ClassKeywordIsCanonical )
{
    EXPECT_EQ( "AliasItemObj", AliasItemObj::classKeywordStatic().toStdString() );

    auto aliases = AliasItemObj::classKeywordAliases();
    ASSERT_EQ( 3u, aliases.size() );
    EXPECT_EQ( "AliasItemObj", aliases[0].toStdString() );
    EXPECT_EQ( "OldAliasItemObj", aliases[1].toStdString() );
    EXPECT_EQ( "OlderAliasItemObj", aliases[2].toStdString() );

    AliasItemObj obj;
    EXPECT_EQ( "AliasItemObj", obj.classKeyword().toStdString() );
    EXPECT_TRUE( obj.matchesClassKeyword( "AliasItemObj" ) );
    EXPECT_TRUE( obj.matchesClassKeyword( "OldAliasItemObj" ) );
    EXPECT_TRUE( obj.matchesClassKeyword( "OlderAliasItemObj" ) );

    // Aliases of a base class are not inherited
    EXPECT_FALSE( obj.matchesClassKeyword( "AliasBaseObj" ) );
    EXPECT_FALSE( obj.matchesClassKeyword( "aliasitemobj" ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, FactoryCreatesObjectFromCanonicalKeywordAndAliases )
{
    auto factory = caf::PdmDefaultObjectFactory::instance();

    for ( const QString keyword : { "AliasItemObj", "OldAliasItemObj", "OlderAliasItemObj" } )
    {
        std::unique_ptr<caf::PdmObjectHandle> obj( factory->create( keyword ) );
        ASSERT_TRUE( obj != nullptr ) << keyword.toStdString();

        auto item = dynamic_cast<AliasItemObj*>( obj.get() );
        ASSERT_TRUE( item != nullptr ) << keyword.toStdString();
        EXPECT_EQ( "AliasItemObj", item->classKeyword().toStdString() );
    }

    EXPECT_TRUE( factory->create( "UnknownAliasItemObj" ) == nullptr );

    // Canonical keyword and aliases are all registered as class keywords in the factory
    auto classKeywords = factory->classKeywords();
    for ( const QString keyword : { "AliasItemObj", "OldAliasItemObj", "OlderAliasItemObj" } )
    {
        EXPECT_EQ( 1, std::count( classKeywords.begin(), classKeywords.end(), keyword ) ) << keyword.toStdString();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, FieldLookupByAlias )
{
    AliasItemObj obj;

    // The field is declared in the base class
    EXPECT_EQ( &obj.m_value, obj.findField( "Value" ) );
    EXPECT_EQ( &obj.m_value, obj.findField( "OldValue" ) );
    EXPECT_EQ( &obj.m_value, obj.findField( "OlderValue" ) );
    EXPECT_TRUE( obj.findField( "value" ) == nullptr );

    EXPECT_EQ( "Value", obj.m_value.keyword().toStdString() );
    EXPECT_TRUE( obj.m_value.matchesKeyword( "Value" ) );
    EXPECT_TRUE( obj.m_value.matchesKeyword( "OldValue" ) );
    EXPECT_FALSE( obj.m_value.matchesKeywordAlias( "Value" ) );
    EXPECT_TRUE( obj.m_value.matchesKeywordAlias( "OldValue" ) );

    auto aliases = obj.m_value.keywordAliases();
    ASSERT_EQ( 2u, aliases.size() );
    EXPECT_EQ( "OldValue", aliases[0].toStdString() );
    EXPECT_EQ( "OlderValue", aliases[1].toStdString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, ReadObjectUsingAliasesAndWriteCanonical )
{
    QString xmlText = "<OldAliasItemObj>"
                      "<OldValue>2.5</OldValue>"
                      "<Name>abc</Name>"
                      "</OldAliasItemObj>";

    std::unique_ptr<caf::PdmObjectHandle> obj(
        caf::PdmXmlObjectHandle::readUnknownObjectFromXmlString( xmlText, caf::PdmDefaultObjectFactory::instance(), false ) );

    auto item = dynamic_cast<AliasItemObj*>( obj.get() );
    ASSERT_TRUE( item != nullptr );
    EXPECT_DOUBLE_EQ( 2.5, item->m_value() );
    EXPECT_EQ( "abc", item->m_name().toStdString() );

    QString written = item->writeObjectToXmlString();
    EXPECT_TRUE( written.contains( "<AliasItemObj>" ) );
    EXPECT_TRUE( written.contains( "<Value>2.5</Value>" ) );
    EXPECT_FALSE( written.contains( "Old" ) );
}

//--------------------------------------------------------------------------------------------------
/// If both the alias and the canonical keyword are present, the fields are read in file order
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, LastOccurrenceWinsWhenAliasAndCanonicalArePresent )
{
    {
        AliasItemObj obj;
        obj.readObjectFromXmlString( "<AliasItemObj><OldValue>1</OldValue><Value>2</Value></AliasItemObj>",
                                     caf::PdmDefaultObjectFactory::instance() );
        EXPECT_DOUBLE_EQ( 2.0, obj.m_value() );
    }

    {
        AliasItemObj obj;
        obj.readObjectFromXmlString( "<AliasItemObj><Value>2</Value><OldValue>1</OldValue></AliasItemObj>",
                                     caf::PdmDefaultObjectFactory::instance() );
        EXPECT_DOUBLE_EQ( 1.0, obj.m_value() );
    }
}

//--------------------------------------------------------------------------------------------------
/// A file written with old class keywords, old field keywords and old path references is read into
/// the same object structure, and is written back using canonical names only.
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, RoundTripWithAliasedChildrenAndPathReferences )
{
    auto container = createContainerWithThreeItems();

    QString canonicalXml = container->writeObjectToXmlString();
    EXPECT_FALSE( canonicalXml.contains( "Old" ) );

    // Path references are written with the canonical field keyword
    EXPECT_TRUE( canonicalXml.contains( "<SelectedItem>$ROOT$ Items 1</SelectedItem>" ) ) << canonicalXml.toStdString();

    // Construct the legacy version of the same file
    QString legacyXml = canonicalXml;
    legacyXml.replace( "AliasContainerObj", "OldAliasContainerObj" );
    legacyXml.replace( "AliasItemObj", "OlderAliasItemObj" );
    legacyXml.replace( "<Items>", "<OldItems>" );
    legacyXml.replace( "</Items>", "</OldItems>" );
    legacyXml.replace( "$ROOT$ Items ", "$ROOT$ OldItems " );
    legacyXml.replace( "Value>", "OldValue>" );
    ASSERT_NE( canonicalXml.toStdString(), legacyXml.toStdString() );
    ASSERT_TRUE( legacyXml.contains( "<SelectedItem>$ROOT$ OldItems 1</SelectedItem>" ) ) << legacyXml.toStdString();

    std::unique_ptr<caf::PdmObjectHandle> obj(
        caf::PdmXmlObjectHandle::readUnknownObjectFromXmlString( legacyXml, caf::PdmDefaultObjectFactory::instance(), false ) );

    auto loaded = dynamic_cast<AliasContainerObj*>( obj.get() );
    ASSERT_TRUE( loaded != nullptr );
    loaded->resolveReferencesRecursively();

    ASSERT_EQ( 3u, loaded->m_items.size() );
    EXPECT_DOUBLE_EQ( 20.0, loaded->m_items[1]->m_value() );
    EXPECT_EQ( "Item 2", loaded->m_items[2]->m_name().toStdString() );

    EXPECT_EQ( loaded->m_items[1], loaded->m_selectedItem() );
    ASSERT_EQ( 2u, loaded->m_selectedItems.size() );
    EXPECT_EQ( loaded->m_items[2], loaded->m_selectedItems[0] );
    EXPECT_EQ( loaded->m_items[0], loaded->m_selectedItems[1] );

    EXPECT_EQ( canonicalXml.toStdString(), loaded->writeObjectToXmlString().toStdString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, ReferenceHelperResolvesAliasesAndWritesCanonical )
{
    auto container = createContainerWithThreeItems();
    auto item      = container->m_items[2];

    EXPECT_EQ( "Items 2", caf::PdmReferenceHelper::referenceFromRootToObject( container.get(), item ).toStdString() );
    EXPECT_EQ( "Value Items 2",
               caf::PdmReferenceHelper::referenceFromRootToField( container.get(), &item->m_value ).toStdString() );

    EXPECT_EQ( item, caf::PdmReferenceHelper::objectFromReference( container.get(), "Items 2" ) );
    EXPECT_EQ( item, caf::PdmReferenceHelper::objectFromReference( container.get(), "OldItems 2" ) );
    EXPECT_TRUE( caf::PdmReferenceHelper::objectFromReference( container.get(), "UnknownItems 2" ) == nullptr );

    EXPECT_EQ( &item->m_value, caf::PdmReferenceHelper::fieldFromReference( container.get(), "Value Items 2" ) );
    EXPECT_EQ( &item->m_value, caf::PdmReferenceHelper::fieldFromReference( container.get(), "OldValue OldItems 2" ) );

    EXPECT_EQ( item, caf::PdmReferenceHelper::objectFromFieldReference( &container->m_selectedItem, "OldItems 2" ) );
}

//--------------------------------------------------------------------------------------------------
/// Collisions are not rejected. A canonical keyword takes precedence over an alias, and for equal
/// aliases the field registered first is found.
//--------------------------------------------------------------------------------------------------
TEST( KeywordAliasTest, CollisionLookupOrder )
{
    AliasCollisionObj obj;

    EXPECT_EQ( &obj.m_second, obj.findField( "Second" ) );
    EXPECT_EQ( &obj.m_second, obj.findField( "Shared" ) );

    obj.readObjectFromXmlString( "<AliasCollisionObj><Second>7</Second><Shared>9</Shared></AliasCollisionObj>",
                                 caf::PdmDefaultObjectFactory::instance() );

    EXPECT_EQ( 0, obj.m_first() );
    EXPECT_EQ( 9, obj.m_second() );
    EXPECT_EQ( 0, obj.m_third() );
}
