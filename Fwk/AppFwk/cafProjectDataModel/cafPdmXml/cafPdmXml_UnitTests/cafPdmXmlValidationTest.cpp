#include "gtest/gtest.h"

#include "cafPdmDataValueField.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmXmlObjectHandle.h"
#include "cafPdmXmlObjectHandleMacros.h"

#include <QXmlStreamWriter>

/// Test object with range validation on fields
class ValidationTestObjectXml : public caf::PdmObjectHandle, public caf::PdmXmlObjectHandle
{
    CAF_PDM_XML_HEADER_INIT;

public:
    ValidationTestObjectXml()
        : PdmObjectHandle()
        , PdmXmlObjectHandle( this, false )
    {
        CAF_PDM_XML_InitField( &m_temperature, "temperature" );
        m_temperature = 20.0;
        m_temperature.setRange( -273.15, 1000.0 );

        CAF_PDM_XML_InitField( &m_age, "age" );
        m_age = 25;
        m_age.setRange( 0, 150 );

        CAF_PDM_XML_InitField( &m_count, "count" );
        m_count = 10;
        m_count.setMinValue( 0 );

        CAF_PDM_XML_InitField( &m_percentage, "percentage" );
        m_percentage = 50.0;
        m_percentage.setMaxValue( 100.0 );
    }

    caf::PdmField<double> m_temperature;
    caf::PdmField<int>    m_age;
    caf::PdmField<int>    m_count;
    caf::PdmField<double> m_percentage;
};

CAF_PDM_XML_SOURCE_INIT( ValidationTestObjectXml, "ValidationTestObjectXml" );

//--------------------------------------------------------------------------------------------------
/// Test that values are clamped when loaded from XML
//--------------------------------------------------------------------------------------------------
TEST( XmlValidationTest, ClampValuesOnLoad )
{
    // Create XML with out-of-range values
    QString xmlDoc = R"(
<?xml version="1.0"?>
<ValidationTestObjectXml>
    <temperature>-500.0</temperature>
    <age>200</age>
    <count>-10</count>
    <percentage>150.0</percentage>
</ValidationTestObjectXml>
)";

    // Parse the XML
    QXmlStreamReader xmlStream( xmlDoc );
    xmlStream.readNext(); // Skip XML declaration
    xmlStream.readNext(); // Read root element

    ValidationTestObjectXml testObj;
    testObj.readFields( xmlStream );

    // Verify values are clamped to valid ranges
    EXPECT_EQ( -273.15, testObj.m_temperature() ); // Clamped to min
    EXPECT_EQ( 150, testObj.m_age() );             // Clamped to max
    EXPECT_EQ( 0, testObj.m_count() );             // Clamped to min
    EXPECT_EQ( 100.0, testObj.m_percentage() );    // Clamped to max
}

//--------------------------------------------------------------------------------------------------
/// Test that valid values are preserved when loaded from XML
//--------------------------------------------------------------------------------------------------
TEST( XmlValidationTest, PreserveValidValuesOnLoad )
{
    // Create XML with valid values
    QString xmlDoc = R"(
<?xml version="1.0"?>
<ValidationTestObjectXml>
    <temperature>25.0</temperature>
    <age>30</age>
    <count>5</count>
    <percentage>75.0</percentage>
</ValidationTestObjectXml>
)";

    // Parse the XML
    QXmlStreamReader xmlStream( xmlDoc );
    xmlStream.readNext(); // Skip XML declaration
    xmlStream.readNext(); // Read root element

    ValidationTestObjectXml testObj;
    testObj.readFields( xmlStream );

    // Verify values are preserved
    EXPECT_EQ( 25.0, testObj.m_temperature() );
    EXPECT_EQ( 30, testObj.m_age() );
    EXPECT_EQ( 5, testObj.m_count() );
    EXPECT_EQ( 75.0, testObj.m_percentage() );
}

//--------------------------------------------------------------------------------------------------
/// Test that setValue clamps values
//--------------------------------------------------------------------------------------------------
TEST( XmlValidationTest, SetValueClamps )
{
    ValidationTestObjectXml testObj;

    // Set out-of-range values
    testObj.m_temperature.setValue( -500.0 );
    testObj.m_age.setValue( 200 );
    testObj.m_count.setValue( -10 );
    testObj.m_percentage.setValue( 150.0 );

    // Verify values are clamped
    EXPECT_EQ( -273.15, testObj.m_temperature() );
    EXPECT_EQ( 150, testObj.m_age() );
    EXPECT_EQ( 0, testObj.m_count() );
    EXPECT_EQ( 100.0, testObj.m_percentage() );
}

//--------------------------------------------------------------------------------------------------
/// Test that assignment operator clamps values
//--------------------------------------------------------------------------------------------------
TEST( XmlValidationTest, AssignmentOperatorClamps )
{
    ValidationTestObjectXml testObj;

    // Assign out-of-range values
    testObj.m_temperature = -500.0;
    testObj.m_age         = 200;
    testObj.m_count       = -10;
    testObj.m_percentage  = 150.0;

    // Verify values are clamped
    EXPECT_EQ( -273.15, testObj.m_temperature() );
    EXPECT_EQ( 150, testObj.m_age() );
    EXPECT_EQ( 0, testObj.m_count() );
    EXPECT_EQ( 100.0, testObj.m_percentage() );
}
