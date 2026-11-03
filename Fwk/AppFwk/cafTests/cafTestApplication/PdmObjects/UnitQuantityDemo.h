#pragma once

#include "UnitQuantity.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

//==================================================================================================
/// Demonstrates SI-stored quantity fields with display unit following the selected unit system.
/// Switching unit system changes only what is shown; the stored (and saved) values are SI.
//==================================================================================================
class UnitQuantityDemo : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    UnitQuantityDemo();

private:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void updateSiReadout();

private:
    caf::PdmField<caf::AppEnum<qty::UnitSystem>> m_unitSystem;
    caf::PdmField<caf::AppEnum<qty::Unit>>       m_diameterUnit;
    caf::PdmField<qty::Length>                   m_length;
    caf::PdmField<qty::Length>                   m_thickness;
    caf::PdmField<qty::Diameter>                 m_tubingDiameter;
    caf::PdmField<qty::Pressure>                 m_pressure;
    caf::PdmField<qty::BuildAngle>               m_buildAngle;
    caf::PdmField<qty::Diameter>                 m_diameter;
    caf::PdmField<QString>                       m_siReadout;
};
