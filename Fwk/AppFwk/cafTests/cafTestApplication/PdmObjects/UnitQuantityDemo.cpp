#include "UnitQuantityDemo.h"

#include "cafPdmUiLineEditor.h"

template <>
void caf::AppEnum<qty::UnitSystem>::setUp()
{
    addItem( qty::UnitSystem::Metric, "METRIC", "Metric" );
    addItem( qty::UnitSystem::Field, "FIELD", "Field" );

    setDefault( qty::UnitSystem::Metric );
}

template <>
void caf::AppEnum<qty::Unit>::setUp()
{
    addItem( qty::Unit::Auto, "AUTO", "Follow unit system" );
    addItem( qty::Unit::Meter, "METER", "m" );
    addItem( qty::Unit::Millimeter, "MILLIMETER", "mm" );
    addItem( qty::Unit::Feet, "FEET", "ft" );
    addItem( qty::Unit::Inch, "INCH", "in" );
    addItem( qty::Unit::Bar, "BAR", "bar" );
    addItem( qty::Unit::Psi, "PSI", "psi" );
    addItem( qty::Unit::DegPerMeter, "DEG_PER_METER", "deg/m" );
    addItem( qty::Unit::DegPerFeet, "DEG_PER_FEET", "deg/ft" );

    setDefault( qty::Unit::Auto );
}

CAF_PDM_SOURCE_INIT( UnitQuantityDemo, "UnitQuantityDemo" );

namespace
{
const auto& kLengthUnits     = qty::presets::length;
const auto& kThicknessUnits  = qty::presets::wellDiameter;
const auto& kTubingUnits     = qty::presets::smallDiameter;
const auto& kPressureUnits   = qty::presets::pressure;
const auto& kBuildAngleUnits = qty::presets::buildAngle;
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
UnitQuantityDemo::UnitQuantityDemo()
{
    CAF_PDM_InitObject( "Unit Quantity Demo", "", "", "" );

    CAF_PDM_InitField( &m_unitSystem, "UnitSystem", caf::AppEnum<qty::UnitSystem>( qty::UnitSystem::Metric ), "Unit System" );

    CAF_PDM_InitField( &m_diameterUnit, "DiameterUnit", caf::AppEnum<qty::Unit>( qty::Unit::Auto ), "Diameter Unit" );

    CAF_PDM_InitField( &m_length, "Length", qty::Length::fromSi( 100.0 ), "Length" );
    CAF_PDM_InitField( &m_thickness, "Thickness", qty::Length::fromSi( 0.0127 ), "Thickness" );
    CAF_PDM_InitField( &m_diameter, "Diameter", qty::Diameter::fromSi( 0.216 ), "Diameter" );
    CAF_PDM_InitField( &m_tubingDiameter, "TubingDiameter", qty::Diameter::fromSi( 0.0508 ), "Tubing Diameter" );
    CAF_PDM_InitField( &m_pressure, "Pressure", qty::Pressure::fromSi( 250.0e5 ), "Pressure" );
    CAF_PDM_InitField( &m_buildAngle, "BuildAngle", qty::BuildAngle::fromSi( 0.05 ), "Build Angle" );

    // No default editor is registered for the quantity types, so ask for the line editor explicitly
    for ( auto* field : std::vector<caf::PdmUiFieldHandle*>{ m_length.uiCapability(),
                                                             m_thickness.uiCapability(),
                                                             m_diameter.uiCapability(),
                                                             m_tubingDiameter.uiCapability(),
                                                             m_pressure.uiCapability(),
                                                             m_buildAngle.uiCapability() } )
    {
        field->setUiEditorTypeName( caf::PdmUiLineEditor::uiEditorTypeName() );
    }

    auto unitSystemProvider = [this]() { return m_unitSystem().value(); };
    qty::setDisplayUnit<qty::Quantity::Diameter>( &m_diameter,
                                                  unitSystemProvider,
                                                  [this]() { return m_diameterUnit().value(); } );

    // Prescribed unit per unit system. Length and thickness are the same quantity: both m in Metric, but ft and in in
    // Field
    qty::setDisplayUnit( &m_length, unitSystemProvider, kLengthUnits );
    qty::setDisplayUnit( &m_thickness, unitSystemProvider, kThicknessUnits );
    qty::setDisplayUnit( &m_tubingDiameter, unitSystemProvider, kTubingUnits );
    qty::setDisplayUnit( &m_pressure, unitSystemProvider, kPressureUnits );
    qty::setDisplayUnit( &m_buildAngle, unitSystemProvider, kBuildAngleUnits );

    CAF_PDM_InitFieldNoDefault( &m_siReadout, "SiReadout", "Stored (SI)" );
    m_siReadout.uiCapability()->setUiReadOnly( true );
    updateSiReadout();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void UnitQuantityDemo::updateSiReadout()
{
    m_siReadout =
        QString( "Length = %1 m, Thickness = %2 m, Diameter = %3 m, Tubing = %4 m, Pressure = %5 Pa, Build = %6 rad/m" )
            .arg( m_length().si() )
            .arg( m_thickness().si() )
            .arg( m_diameter().si() )
            .arg( m_tubingDiameter().si() )
            .arg( m_pressure().si() )
            .arg( m_buildAngle().si() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void UnitQuantityDemo::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                         const QVariant&            oldValue,
                                         const QVariant&            newValue )
{
    updateSiReadout();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void UnitQuantityDemo::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    qty::setUnitLabel( &m_length, "Length", m_unitSystem(), kLengthUnits );
    qty::setUnitLabel( &m_thickness, "Thickness", m_unitSystem(), kThicknessUnits );
    qty::setUnitLabel( &m_diameter, "Diameter", m_unitSystem(), m_diameterUnit() );
    qty::setUnitLabel( &m_tubingDiameter, "Tubing Diameter", m_unitSystem(), kTubingUnits );
    qty::setUnitLabel( &m_pressure, "Pressure", m_unitSystem(), kPressureUnits );
    qty::setUnitLabel( &m_buildAngle, "Build Angle", m_unitSystem(), kBuildAngleUnits );

    uiOrdering.add( &m_unitSystem );
    uiOrdering.add( &m_diameterUnit );
    uiOrdering.add( &m_length );
    uiOrdering.add( &m_thickness );
    uiOrdering.add( &m_diameter );
    uiOrdering.add( &m_tubingDiameter );
    uiOrdering.add( &m_pressure );
    uiOrdering.add( &m_buildAngle );
    uiOrdering.add( &m_siReadout );
}
