#include "InitFunctionDemo.h"

CAF_PDM_SOURCE_INIT( InitMacroDemo, "InitMacroDemo" );
CAF_PDM_SOURCE_INIT( InitFunctionDemo, "InitFunctionDemo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
InitMacroDemo::InitMacroDemo()
{
    CAF_PDM_InitObject( "Init With Macros", "", "Object tool tip", "Object initialized with macros" );

    CAF_PDM_InitField( &m_name, "Name", QString( "Well A" ), "Name" );
    CAF_PDM_InitField( &m_radius, "Radius", 0.15, "Radius", "", "Radius of the well", "The radius is given in meters" );
    CAF_PDM_InitField( &m_count, "Count", 3, "Count" );
    CAF_PDM_InitField( &m_isActive, "IsActive", true, "Active" );
    CAF_PDM_InitFieldNoDefault( &m_optionalValue, "OptionalValue", "Optional Value" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
InitFunctionDemo::InitFunctionDemo()
{
    initObject( "Init With Functions" ).withToolTip( "Object tool tip" ).withWhatsThis( "Object initialized with functions" );

    initField( m_name, "Name", QString( "Well A" ), "Name" );
    initField( m_radius, "Radius", 0.15, "Radius" ).withToolTip( "Radius of the well" ).withWhatsThis( "The radius is given in meters" );
    initField( m_count, "Count", "Count" ).withDefault( 3 );
    initField( m_isActive, "IsActive", true, "Active" );
    initField( m_optionalValue, "OptionalValue", "Optional Value" );
}
