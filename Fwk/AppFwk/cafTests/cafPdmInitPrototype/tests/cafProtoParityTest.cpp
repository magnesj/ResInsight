#include "gtest/gtest.h"

#include "cafProtoScripting.h"
#include "cafProtoTestObjects.h"

#include "cafPdmDefaultObjectFactory.h"
#include "cafPdmPythonGenerator.h"
#include "cafPdmUiFieldHandle.h"

#include <QIcon>

// Parity between objects initialized with the CAF_PDM_Init* macros and objects initialized with the prototype
// functions. The macro version is the reference.

namespace
{
QString renamed( QString text )
{
    text.replace( "ParityFn", "ParityMacro" );
    text.replace( "parity_fn", "parity_macro" );
    return text;
}

QString scriptKeyword( caf::PdmFieldHandle* field )
{
    auto capability = field->capability<caf::PdmAbstractFieldScriptingCapability>();
    return capability ? capability->scriptFieldName() : QString( "<not scriptable>" );
}

QString generatedClassCode( const QString& generatedCode, const QString& className )
{
    int start = generatedCode.indexOf( QString( "class %1(" ).arg( className ) );
    if ( start == -1 ) return {};

    int end = generatedCode.indexOf( "\nclass ", start );
    if ( end == -1 ) end = generatedCode.indexOf( "\ndef ", start );
    if ( end == -1 ) end = generatedCode.size();

    return generatedCode.mid( start, end - start );
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, FieldsKeywordsAndOwnerClass )
{
    ParityMacroObj macroObj;
    ParityFnObj    fnObj;

    auto macroFields = macroObj.fields();
    auto fnFields    = fnObj.fields();
    ASSERT_EQ( macroFields.size(), fnFields.size() );
    ASSERT_EQ( 11u, fnFields.size() );

    for ( size_t i = 0; i < macroFields.size(); i++ )
    {
        SCOPED_TRACE( macroFields[i]->keyword().toStdString() );

        EXPECT_EQ( macroFields[i]->keyword().toStdString(), fnFields[i]->keyword().toStdString() );
        EXPECT_EQ( macroFields[i]->ownerClass().toStdString(), renamed( fnFields[i]->ownerClass() ).toStdString() );
        EXPECT_EQ( macroFields[i]->keywordAliases(), fnFields[i]->keywordAliases() );
        EXPECT_EQ( &fnObj, fnFields[i]->ownerObject() );

        EXPECT_EQ( macroFields[i]->uiCapability() != nullptr, fnFields[i]->uiCapability() != nullptr );
        EXPECT_EQ( macroFields[i]->xmlCapability() != nullptr, fnFields[i]->xmlCapability() != nullptr );
        EXPECT_EQ( macroFields[i]->capabilities().size(), fnFields[i]->capabilities().size() );
        EXPECT_EQ( scriptKeyword( macroFields[i] ).toStdString(), scriptKeyword( fnFields[i] ).toStdString() );
    }

    // The field declared in the base class is owned by the base class
    EXPECT_EQ( "ParityFnBase", fnObj.m_name.ownerClass().toStdString() );
    EXPECT_EQ( "ParityFnObj", fnObj.m_count.ownerClass().toStdString() );

    EXPECT_EQ( macroObj.classInheritanceStack().size(), fnObj.classInheritanceStack().size() );
    EXPECT_TRUE( fnObj.inheritsClassWithKeyword( "ParityFnBase" ) );
    EXPECT_TRUE( fnObj.inheritsClassWithKeyword( "ParityFnObj" ) );

    EXPECT_TRUE( fnObj.m_custom.m_isConfigured );
    EXPECT_EQ( macroObj.m_custom.m_isConfigured, fnObj.m_custom.m_isConfigured );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, UiInformation )
{
    ParityMacroObj macroObj;
    ParityFnObj    fnObj;

    EXPECT_EQ( macroObj.uiName().toStdString(), fnObj.uiName().toStdString() );
    EXPECT_EQ( macroObj.uiToolTip().toStdString(), fnObj.uiToolTip().toStdString() );
    EXPECT_EQ( macroObj.uiWhatsThis().toStdString(), fnObj.uiWhatsThis().toStdString() );
    EXPECT_EQ( macroObj.uiIconProvider().valid(), fnObj.uiIconProvider().valid() );
    EXPECT_EQ( "Parity Object", fnObj.uiName().toStdString() );
    EXPECT_EQ( "Object whats this", fnObj.uiWhatsThis().toStdString() );

    auto macroFields = macroObj.fields();
    auto fnFields    = fnObj.fields();

    for ( size_t i = 0; i < macroFields.size(); i++ )
    {
        SCOPED_TRACE( macroFields[i]->keyword().toStdString() );

        auto macroUi = macroFields[i]->uiCapability();
        auto fnUi    = fnFields[i]->uiCapability();

        EXPECT_EQ( macroUi->uiName().toStdString(), fnUi->uiName().toStdString() );
        EXPECT_EQ( macroUi->uiToolTip().toStdString(), fnUi->uiToolTip().toStdString() );
        EXPECT_EQ( macroUi->uiWhatsThis().toStdString(), fnUi->uiWhatsThis().toStdString() );
        EXPECT_EQ( macroUi->uiIconProvider().valid(), fnUi->uiIconProvider().valid() );

        // The static information is used for the static configuration name
        QString staticConfig = caf::PdmUiItem::uiConfigNameForStaticData();
        EXPECT_EQ( macroUi->uiName( staticConfig ).toStdString(), fnUi->uiName( staticConfig ).toStdString() );
    }

    // Tool tip of scriptable fields is extended with the Python attribute name
    EXPECT_EQ( "Radius tool tip\n\nAvailable through python/rips as the attribute 'radius'",
               fnObj.m_radius.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "Available through python/rips as the attribute 'legacy_count'",
               fnObj.m_legacy.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "", fnObj.m_count.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "Flag whats this", fnObj.m_flag.uiCapability()->uiWhatsThis().toStdString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, DefaultValues )
{
    ParityMacroObj macroObj;
    ParityFnObj    fnObj;

    EXPECT_EQ( macroObj.m_name().toStdString(), fnObj.m_name().toStdString() );
    EXPECT_EQ( macroObj.m_count(), fnObj.m_count() );
    EXPECT_EQ( macroObj.m_radius(), fnObj.m_radius() );
    EXPECT_EQ( macroObj.m_flag(), fnObj.m_flag() );
    EXPECT_EQ( macroObj.m_text().toStdString(), fnObj.m_text().toStdString() );
    EXPECT_EQ( macroObj.m_enum(), fnObj.m_enum() );
    EXPECT_EQ( macroObj.m_values(), fnObj.m_values() );
    EXPECT_EQ( macroObj.m_legacy(), fnObj.m_legacy() );
    EXPECT_EQ( macroObj.m_custom(), fnObj.m_custom() );

    EXPECT_EQ( macroObj.m_name.defaultValue().toStdString(), fnObj.m_name.defaultValue().toStdString() );
    EXPECT_EQ( macroObj.m_count.defaultValue(), fnObj.m_count.defaultValue() );
    EXPECT_EQ( macroObj.m_radius.defaultValue(), fnObj.m_radius.defaultValue() );
    EXPECT_EQ( macroObj.m_flag.defaultValue(), fnObj.m_flag.defaultValue() );
    EXPECT_EQ( macroObj.m_enum.defaultValue(), fnObj.m_enum.defaultValue() );
    EXPECT_EQ( macroObj.m_legacy.defaultValue(), fnObj.m_legacy.defaultValue() );
    EXPECT_EQ( macroObj.m_custom.defaultValue(), fnObj.m_custom.defaultValue() );

    EXPECT_EQ( 3, fnObj.m_count() );
    EXPECT_EQ( 1.5, fnObj.m_radius.defaultValue() );
    EXPECT_TRUE( fnObj.m_flag() );
    EXPECT_EQ( ParityEnum::B, fnObj.m_enum() );
    EXPECT_EQ( 7, fnObj.m_legacy.defaultValue() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, XmlRoundTripAndAliases )
{
    auto configure = []( auto& obj, auto child )
    {
        obj.m_name    = "Parent";
        obj.m_count   = 12;
        obj.m_radius  = 0.25;
        obj.m_enum    = ParityEnum::C;
        obj.m_values  = { 1.0, 2.0, 3.0 };
        child->m_name = "Child";
        obj.m_children.push_back( child );
        obj.m_selected = child;
    };

    ParityMacroObj macroObj;
    ParityFnObj    fnObj;
    configure( macroObj, new ParityMacroBase );
    configure( fnObj, new ParityFnBase );

    QString macroXml = macroObj.writeObjectToXmlString();
    QString fnXml    = fnObj.writeObjectToXmlString();
    EXPECT_EQ( macroXml.toStdString(), renamed( fnXml ).toStdString() );

    // Read a legacy file using the class alias and the field alias
    QString legacyXml = fnXml;
    legacyXml.replace( "ParityFnObj", "OldParityFnObj" );
    legacyXml.replace( "Name>", "OldName>" );
    ASSERT_TRUE( legacyXml.contains( "<OldName>Parent</OldName>" ) );

    std::unique_ptr<caf::PdmObjectHandle> loaded(
        caf::PdmXmlObjectHandle::readUnknownObjectFromXmlString( legacyXml, caf::PdmDefaultObjectFactory::instance(), false ) );
    auto loadedObj = dynamic_cast<ParityFnObj*>( loaded.get() );
    ASSERT_TRUE( loadedObj != nullptr );
    loadedObj->resolveReferencesRecursively();

    EXPECT_EQ( "Parent", loadedObj->m_name().toStdString() );
    EXPECT_EQ( 12, loadedObj->m_count() );
    EXPECT_EQ( ParityEnum::C, loadedObj->m_enum() );
    ASSERT_EQ( 1u, loadedObj->m_children.size() );
    EXPECT_EQ( "Child", loadedObj->m_children[0]->m_name().toStdString() );
    EXPECT_EQ( loadedObj->m_children[0], loadedObj->m_selected() );

    EXPECT_EQ( fnXml.toStdString(), loadedObj->writeObjectToXmlString().toStdString() );

    // Copy by XML serialization
    std::unique_ptr<ParityFnObj> copy( fnObj.copyObject<ParityFnObj>() );
    ASSERT_TRUE( copy != nullptr );
    EXPECT_EQ( 12, copy->m_count() );
    EXPECT_EQ( 3, copy->m_count.defaultValue() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, ScriptingAndGeneratedPython )
{
    ParityMacroObj macroObj;
    ParityFnObj    fnObj;

    using Register = caf::PdmObjectScriptingCapabilityRegister;

    EXPECT_TRUE( Register::isScriptable( &fnObj ) );
    EXPECT_EQ( "ParityFnObj", Register::scriptClassNameFromClassKeyword( "ParityFnObj" ).toStdString() );
    EXPECT_EQ( "Object whats this", Register::scriptClassComment( "ParityFnObj" ).toStdString() );
    EXPECT_EQ( "ParityFnBaseScript", Register::scriptClassNameFromClassKeyword( "ParityFnBase" ).toStdString() );
    EXPECT_EQ( "Base script comment", Register::scriptClassComment( "ParityFnBase" ).toStdString() );
    EXPECT_EQ( Register::scriptClassComment( "ParityMacroObj" ).toStdString(),
               Register::scriptClassComment( "ParityFnObj" ).toStdString() );

    EXPECT_EQ( "Radius", scriptKeyword( &fnObj.m_radius ).toStdString() );
    EXPECT_EQ( "LegacyCount", scriptKeyword( &fnObj.m_legacy ).toStdString() );
    EXPECT_EQ( "<not scriptable>", scriptKeyword( &fnObj.m_count ).toStdString() );

    caf::PdmPythonGenerator generator;
    std::vector<QString>    errorMessages;
    QString generatedCode = generator.generate( caf::PdmDefaultObjectFactory::instance(), errorMessages );
    EXPECT_TRUE( errorMessages.empty() );

    for ( const QString name : { "Obj", "BaseScript" } )
    {
        QString macroCode = generatedClassCode( generatedCode, "ParityMacro" + name );
        QString fnCode    = generatedClassCode( generatedCode, "ParityFn" + name );
        ASSERT_FALSE( macroCode.isEmpty() ) << name.toStdString();
        ASSERT_FALSE( fnCode.isEmpty() ) << name.toStdString();
        EXPECT_EQ( macroCode.toStdString(), renamed( fnCode ).toStdString() );
    }

    EXPECT_TRUE( generatedClassCode( generatedCode, "ParityFnObj" ).contains( "legacy_count" ) );
}

//--------------------------------------------------------------------------------------------------
/// Static UI information is shared between instances, and a change for one instance does not affect
/// other instances.
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, StaticInformationIsSharedAndInstanceChangesAreLocal )
{
    ParityFnObj first;
    size_t      countAfterFirst = cafproto::metadataObjectCount();
    EXPECT_GT( countAfterFirst, 0u );

    ParityFnObj second;
    ParityFnObj third;
    EXPECT_EQ( countAfterFirst, cafproto::metadataObjectCount() );

    first.m_count.uiCapability()->setUiName( "Changed" );
    first.m_count.uiCapability()->setUiToolTip( "Changed tool tip" );
    first.setUiName( "Changed object" );

    EXPECT_EQ( "Changed", first.m_count.uiCapability()->uiName().toStdString() );
    EXPECT_EQ( "Count", second.m_count.uiCapability()->uiName().toStdString() );
    EXPECT_EQ( "", second.m_count.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "Parity Object", second.uiName().toStdString() );

    // The static information of the changed instance is still available
    EXPECT_EQ( "Count", first.m_count.uiCapability()->uiName( caf::PdmUiItem::uiConfigNameForStaticData() ).toStdString() );
}

//--------------------------------------------------------------------------------------------------
/// The fixed string alternative, and text computed at runtime
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, FixedStringKeywordAndDynamicText )
{
    ParityFnExtraObj obj;
    ParityFnExtraObj other( "Other tool tip" );

    EXPECT_EQ( "Radius", obj.m_radius.keyword().toStdString() );
    EXPECT_EQ( 1.5, obj.m_radius() );
    EXPECT_EQ( "Radius", scriptKeyword( &obj.m_radius ).toStdString() );
    EXPECT_EQ( "Radius tool tip\n\nAvailable through python/rips as the attribute 'radius'",
               obj.m_radius.uiCapability()->uiToolTip().toStdString() );

    EXPECT_EQ( "Dynamic tool tip", obj.m_dynamic.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "Other tool tip", other.m_dynamic.uiCapability()->uiToolTip().toStdString() );
    EXPECT_EQ( "Dynamic", other.m_dynamic.uiCapability()->uiName().toStdString() );
}

//--------------------------------------------------------------------------------------------------
/// All storage alternatives give the same result
//--------------------------------------------------------------------------------------------------
TEST( ProtoParity, MetadataStorageAlternatives )
{
    for ( auto storage : { cafproto::MetadataStorage::CLASS_REGISTRY, cafproto::MetadataStorage::PER_INSTANCE } )
    {
        cafproto::setMetadataStorage( storage );

        ParityMacroObj macroObj;
        ParityFnObj    fnObj;
        ParityFnObj    fnObj2;

        auto macroFields = macroObj.fields();
        auto fnFields    = fnObj.fields();
        for ( size_t i = 0; i < macroFields.size(); i++ )
        {
            EXPECT_EQ( macroFields[i]->uiCapability()->uiName().toStdString(),
                       fnFields[i]->uiCapability()->uiName().toStdString() );
            EXPECT_EQ( macroFields[i]->uiCapability()->uiToolTip().toStdString(),
                       fnFields[i]->uiCapability()->uiToolTip().toStdString() );
        }
    }

    cafproto::setMetadataStorage( cafproto::MetadataStorage::INTERNED );
}
