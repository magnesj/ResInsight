
#include "cafMockObjects.h"

namespace caf
{
template <>
void AppEnum<InheritedDemoObj::TestEnumType>::setUp()
{
    addItem( InheritedDemoObj::T1, "T1", "An A letter" );
    addItem( InheritedDemoObj::T2, "T2", "A B letter" );
    addItem( InheritedDemoObj::T3, "T3", "A B letter" );
    setDefault( InheritedDemoObj::T1 );
}

template <>
void AppEnum<MyEnum>::setUp()
{
    addItem( MyEnum::T1, "T1", "T1" );
    addItem( MyEnum::T2, "T2", "T2" );
    addItem( MyEnum::T3, "T3", "T3" );
    addItem( MyEnum::T4, "T4", "T4" );
    addItem( MyEnum::T5, "T5", "T5" );
    addItem( MyEnum::T6, "T6", "T6" );
    setDefault( MyEnum::T1 );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( SimpleObj, "SimpleObj" );

namespace caf
{
} // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SimpleObj::SimpleObj()
    : PdmObject()
    , m_doubleMember( 0.0 )
{
    CAF_PDM_InitObject( "SimpleObj", "", "Tooltip SimpleObj", "WhatsThis SimpleObj" );

    CAF_PDM_InitField( &m_position, "Position", 8765.2, "Position", "", "Tooltip", "WhatsThis" );
    CAF_PDM_InitField( &m_dir, "Dir", 123.56, "Direction", "", "Tooltip", "WhatsThis" );
    CAF_PDM_InitField( &m_up, "Up", 0.0, "Up value", "", "Tooltip", "WhatsThis" );
    CAF_PDM_InitFieldNoDefault( &m_numbers, "Numbers", "Important Numbers", "", "Tooltip", "WhatsThis" );
    m_proxyDouble.registerSetMethod( this, &SimpleObj::setDoubleMember );
    m_proxyDouble.registerGetMethod( this, &SimpleObj::doubleMember );
    addUiCapabilityToField( &m_proxyDouble );
    addXmlCapabilityToField( &m_proxyDouble );
    CAF_PDM_InitFieldNoDefault( &m_proxyDouble, "ProxyDouble", "ProxyDouble" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SimpleObj::SimpleObj( const SimpleObj& other )
    : PdmObject()
{
    CAF_PDM_InitField( &m_position, "Position", 8765.2, "Position", "", "", "WhatsThis" );
    CAF_PDM_InitField( &m_dir, "Dir", 123.56, "Direction", "", "", "WhatsThis" );
    CAF_PDM_InitField( &m_up, "Up", 0.0, "Up value", "", "", "WhatsThis" );

    CAF_PDM_InitFieldNoDefault( &m_numbers, "Numbers", "Important Numbers", "", "", "WhatsThis" );

    m_position     = other.m_position;
    m_dir          = other.m_dir;
    m_up           = other.m_up;
    m_numbers      = other.m_numbers;
    m_doubleMember = other.m_doubleMember;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SimpleObj::setDoubleMember( const double& d )
{
    m_doubleMember = d;
    std::cout << "setDoubleMember" << std::endl;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double SimpleObj::doubleMember() const
{
    std::cout << "doubleMember" << std::endl;
    return m_doubleMember;
}

CAF_PDM_SOURCE_INIT( DemoPdmObject, "DemoPdmObject" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
DemoPdmObject::DemoPdmObject()
{
    CAF_PDM_InitObject( "DemoPdmObject", "", "Tooltip DemoPdmObject", "WhatsThis DemoPdmObject" );

    CAF_PDM_InitField( &m_doubleField,
                       "BigNumber",
                       0.0,
                       "",
                       "",
                       "Enter a big number here",
                       "This is a place you can enter a big real value if you want" );

    CAF_PDM_InitField( &m_intField,
                       "IntNumber",
                       0,
                       "",
                       "",
                       "Enter some small number here",
                       "This is a place you can enter a small integer value if you want" );

    CAF_PDM_InitField( &m_textField, "TextField", QString( "��� Test text   end" ), "TextField", "", "Tooltip", "WhatsThis" );
    CAF_PDM_InitFieldNoDefault( &m_simpleObjPtrField, "SimpleObjPtrField", "SimpleObjPtrField", "", "Tooltip", "WhatsThis" );
    CAF_PDM_InitFieldNoDefault( &m_simpleObjPtrField2, "SimpleObjPtrField2", "SimpleObjPtrField2", "", "Tooltip", "WhatsThis" );
    m_simpleObjPtrField2 = new SimpleObj;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
DemoPdmObject::~DemoPdmObject()
{
    delete m_simpleObjPtrField();
    delete m_simpleObjPtrField2();
}

CAF_PDM_SOURCE_INIT( InheritedDemoObj, "InheritedDemoObj" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
InheritedDemoObj::InheritedDemoObj()
{
    CAF_PDM_InitScriptableObjectWithNameAndComment( "InheritedDemoObj",
                                                    "",
                                                    "ToolTip InheritedDemoObj",
                                                    "Whatsthis InheritedDemoObj",
                                                    "ScriptClassName_InheritedDemoObj",
                                                    "Script comment test" );

    CAF_PDM_InitScriptableFieldNoDefault( &m_texts, "Texts", "Some words" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_numbers, "Numbers", "Some words" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_optionalNumber, "OptionalNumber", "Optional Number" );
    {
        std::vector<QString> arguments = {};
        QString              iconResourceName;
        QString              toolTip;
        QString              whatsThis;
        if ( !arguments.empty() ) iconResourceName = arguments[0];
        if ( arguments.size() > 1 ) toolTip = arguments[1];
        if ( arguments.size() > 2 ) whatsThis = arguments[2];
        {
            const std::vector<QString> arguments = { iconResourceName,
                                                     caf::PdmAbstractFieldScriptingCapability::helpString( toolTip, "MyAppEnum" ),
                                                     whatsThis };
            QString                    iconResourceName;
            QString                    toolTip;
            QString                    whatsThis;
            if ( !arguments.empty() ) iconResourceName = arguments[0];
            if ( arguments.size() > 1 ) toolTip = arguments[1];
            if ( arguments.size() > 2 ) whatsThis = arguments[2];
            static_assert( isFirstCharacterValidInXmlKeyword( "MyAppEnum" ), "First character in keyword is invalid" );
            static_assert( !isFirstThreeCharactersXml( "MyAppEnum" ), "Keyword starts with invalid sequence xml" );
            static_assert( isValidXmlKeyword( "MyAppEnum" ), "Detected invalid character in keyword" );
            static bool checkingThePresenceOfHeaderAndSourceInitMacros =
                Error_You_forgot_to_add_the_macro_CAF_PDM_XML_HEADER_INIT_and_or_CAF_PDM_XML_SOURCE_INIT_to_your_cpp_file_for_this_class();
            (void)checkingThePresenceOfHeaderAndSourceInitMacros;
            this->isInheritedFromPdmUiObject();
            this->isInheritedFromPdmXmlSerializable();
            addXmlCapabilityToField( &m_myAppEnum );
            addUiCapabilityToField( &m_myAppEnum );
            configureCapabilities( &m_myAppEnum );
            registerClassWithField( classKeyword(), &m_myAppEnum );
            static caf::PdmUiItemInfo objDescr( "MyAppEnum", QString( iconResourceName ), toolTip, whatsThis, "MyAppEnum" );
            addFieldUiNoDefault( &m_myAppEnum, "MyAppEnum", &objDescr );
        };
        static_assert( isCamelCase( "MyAppEnum" ),
                       "Keyword used for scripting must be in compatible formatted camel casing" );
        ;
        caf::AddScriptingCapabilityToField( &m_myAppEnum, "MyAppEnum" );
    };

    {
        std::vector<QString> arguments = {};
        QString              iconResourceName;
        QString              toolTip;
        QString              whatsThis;
        if ( !arguments.empty() ) iconResourceName = arguments[0];
        if ( arguments.size() > 1 ) toolTip = arguments[1];
        if ( arguments.size() > 2 ) whatsThis = arguments[2];
        {
            const std::vector<QString> arguments = { iconResourceName,
                                                     caf::PdmAbstractFieldScriptingCapability::helpString( toolTip,
                                                                                                           "TestEnumVal"
                                                                                                           "ue" ),
                                                     whatsThis };
            QString                    iconResourceName;
            QString                    toolTip;
            QString                    whatsThis;
            if ( !arguments.empty() ) iconResourceName = arguments[0];
            if ( arguments.size() > 1 ) toolTip = arguments[1];
            if ( arguments.size() > 2 ) whatsThis = arguments[2];
            static_assert( isFirstCharacterValidInXmlKeyword( "TestEnumValue" ), "First character in keyword is invalid" );
            static_assert( !isFirstThreeCharactersXml( "TestEnumValue" ), "Keyword starts with invalid sequence xml" );
            static_assert( isValidXmlKeyword( "TestEnumValue" ), "Detected invalid character in keyword" );
            static bool checkingThePresenceOfHeaderAndSourceInitMacros =
                Error_You_forgot_to_add_the_macro_CAF_PDM_XML_HEADER_INIT_and_or_CAF_PDM_XML_SOURCE_INIT_to_your_cpp_file_for_this_class();
            (void)checkingThePresenceOfHeaderAndSourceInitMacros;
            this->isInheritedFromPdmUiObject();
            this->isInheritedFromPdmXmlSerializable();
            addXmlCapabilityToField( &m_testEnumField );
            addUiCapabilityToField( &m_testEnumField );
            configureCapabilities( &m_testEnumField );
            registerClassWithField( classKeyword(), &m_testEnumField );
            static caf::PdmUiItemInfo objDescr( "An Enum", QString( iconResourceName ), toolTip, whatsThis, "TestEnumValue" );
            addFieldUiNoDefault( &m_testEnumField, "TestEnumValue", &objDescr );
        };
        static_assert( isCamelCase( "TestEnumValue" ),
                       "Keyword used for scripting must be in compatible formatted camel casing" );
        ;
        caf::AddScriptingCapabilityToField( &m_testEnumField, "TestEnumValue" );
    };
    CAF_PDM_InitFieldNoDefault( &m_simpleObjectsField,
                                "SimpleObjects",
                                "SimpleObjectsField",
                                "",
                                "ToolTip SimpleObjectsField",
                                "Whatsthis SimpleObjectsField" );
}

CAF_PDM_SOURCE_INIT( MyPdmDocument, "MyPdmDocument" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
MyPdmDocument::MyPdmDocument()
{
    CAF_PDM_InitObject( "PdmObjectCollection" );
    CAF_PDM_InitFieldNoDefault( &objects, "PdmObjects", "", "", "", "" )
}
