
#include "gtest/gtest.h"

#include "../../cafTests/cafTestApplication/PdmObjects/UnitQuantity.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmUiCommandSystemProxy.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmXmlObjectHandle.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

class QuantityTestObj : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    QuantityTestObj()
    {
        CAF_PDM_InitObject( "QuantityTestObj" );
        CAF_PDM_InitField( &m_length, "Length", qty::Length::fromSi( 100.0 ), "Length" );
        CAF_PDM_InitField( &m_diameter, "Diameter", qty::Diameter::fromSi( 0.216 ), "Diameter" );
        CAF_PDM_InitField( &m_diameter2, "Diameter2", qty::Length::fromSi( 0.1 ), "Diameter2" );
    }

    caf::PdmField<qty::Length>   m_length;
    caf::PdmField<qty::Diameter> m_diameter;
    caf::PdmField<qty::Length>   m_diameter2;
};
CAF_PDM_SOURCE_INIT( QuantityTestObj, "QuantityTestObj" );

TEST( UnitQuantityTest, ConversionRoundTrip )
{
    EXPECT_NEAR( 0.3048, qty::Length::fromUnit( 1.0, qty::UnitSystem::Field ).si(), 1e-12 );
    EXPECT_NEAR( 0.0254, qty::Diameter::fromUnit( 1.0, qty::UnitSystem::Field ).si(), 1e-12 );

    auto length = qty::Length::fromUnit( 328.0, qty::UnitSystem::Field );
    EXPECT_NEAR( 328.0, length.value( qty::UnitSystem::Field ), 1e-9 );
    EXPECT_NEAR( 99.9744, length.value( qty::UnitSystem::Metric ), 1e-9 );
}

TEST( UnitQuantityTest, UiShowsAndAcceptsDisplayUnitWhileStoringSi )
{
    QuantityTestObj obj;
    auto            unitSystem = qty::UnitSystem::Metric;
    qty::setDisplayUnit( &obj.m_length, [&unitSystem]() { return unitSystem; } );

    auto* uiCap = obj.m_length.uiCapability();
    EXPECT_NEAR( 100.0, uiCap->uiValue().toDouble(), 1e-9 );

    unitSystem = qty::UnitSystem::Field;
    EXPECT_NEAR( 328.0839895, uiCap->uiValue().toDouble(), 1e-6 );

    // The editors hand over text; the field must store SI
    caf::PdmUiCommandSystemProxy::instance()->setUiValueToField( uiCap, QVariant( QString( "1000" ) ) );
    EXPECT_NEAR( 304.8, obj.m_length().si(), 1e-9 );

    // Changing unit system does not touch the stored value
    unitSystem = qty::UnitSystem::Metric;
    EXPECT_NEAR( 304.8, obj.m_length().si(), 1e-9 );
    EXPECT_NEAR( 304.8, uiCap->uiValue().toDouble(), 1e-9 );

    // A per-field unit overrides the unit system; Auto falls back to it
    auto unit = qty::Unit::Millimeter;
    qty::setDisplayUnit<
        qty::Quantity::Length>( &obj.m_length, [&unitSystem]() { return unitSystem; }, [&unit]() { return unit; } );
    EXPECT_NEAR( 304800.0, uiCap->uiValue().toDouble(), 1e-6 );
    caf::PdmUiCommandSystemProxy::instance()->setUiValueToField( uiCap, QVariant( QString( "2000" ) ) );
    EXPECT_NEAR( 2.0, obj.m_length().si(), 1e-12 );
    unit       = qty::Unit::Auto;
    unitSystem = qty::UnitSystem::Field;
    EXPECT_NEAR( 6.561679790, uiCap->uiValue().toDouble(), 1e-6 );

    // Prescribed unit per unit system: same quantity, ft or in in Field, m in Metric
    qty::setDisplayUnit( &obj.m_length, [&unitSystem]() { return unitSystem; }, qty::UnitPerSystem{ qty::Unit::Meter, qty::Unit::Feet } );
    qty::setDisplayUnit( &obj.m_diameter2, [&unitSystem]() { return unitSystem; }, qty::UnitPerSystem{ qty::Unit::Meter, qty::Unit::Inch } );
    obj.m_length    = qty::Length::fromSi( 0.3048 );
    obj.m_diameter2 = qty::Length::fromSi( 0.3048 );
    EXPECT_NEAR( 1.0, obj.m_length.uiCapability()->uiValue().toDouble(), 1e-9 );
    EXPECT_NEAR( 12.0, obj.m_diameter2.uiCapability()->uiValue().toDouble(), 1e-9 );
    unitSystem = qty::UnitSystem::Metric;
    EXPECT_NEAR( 0.3048, obj.m_length.uiCapability()->uiValue().toDouble(), 1e-9 );
    EXPECT_NEAR( 0.3048, obj.m_diameter2.uiCapability()->uiValue().toDouble(), 1e-9 );

    // Pressure and build angle presets from ResInsight
    EXPECT_NEAR( 14.5037738, qty::Pressure::fromUnit( 1.0, qty::Unit::Bar ).value( qty::Unit::Psi ), 1e-6 );
    EXPECT_NEAR( 0.3048 * 1.0,
                 qty::BuildAngle::fromUnit( 1.0, qty::Unit::DegPerMeter ).value( qty::Unit::DegPerFeet ),
                 1e-12 );
    EXPECT_NEAR( 25.4, qty::Diameter::fromUnit( 1.0, qty::Unit::Inch ).value( qty::Unit::Millimeter ), 1e-12 );
}

TEST( UnitQuantityTest, XmlStoresSi )
{
    QuantityTestObj source;
    source.m_length   = qty::Length::fromUnit( 1000.0, qty::UnitSystem::Field );
    source.m_diameter = qty::Diameter::fromUnit( 8.5, qty::UnitSystem::Field );

    QString xml;
    {
        QXmlStreamWriter writer( &xml );
        writer.writeStartDocument();
        writer.writeStartElement( "Obj" );
        source.xmlCapability()->writeFields( writer );
        writer.writeEndElement();
        writer.writeEndDocument();
    }

    EXPECT_TRUE( xml.contains( "<Length>304.8</Length>" ) ) << xml.toStdString();

    QuantityTestObj  target;
    QXmlStreamReader reader( xml );
    while ( !reader.atEnd() && reader.readNext() != QXmlStreamReader::StartElement )
    {
    }
    target.xmlCapability()->readFields( reader, nullptr, false );

    EXPECT_NEAR( 304.8, target.m_length().si(), 1e-9 );
    EXPECT_NEAR( 8.5, target.m_diameter().value( qty::UnitSystem::Field ), 1e-9 );
}
