#include "RimOsduWellPath.h"

#include "cafPdmObject.h"
#include "cafPdmObjectScriptingCapability.h"

CAF_PDM_SOURCE_INIT( RimOsduWellPath, "OsduWellPath" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimOsduWellPath::RimOsduWellPath()
{
    CAF_PDM_InitScriptableObjectWithNameAndComment( "Osdu Well Path", ":/Well.svg", "", "", "OsduWellPath", "Well Path Loaded From Osdu" );

    CAF_PDM_InitFieldNoDefault( &m_wellId, "WellId", "Well Id" );
    m_wellId.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_wellboreId, "WellboreId", "Wellbore Id" );
    m_wellboreId.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_wellboreTrajectoryId, "WellboreTrajectoryId", "Wellbore Trajectory Id" );
    m_wellboreTrajectoryId.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_existenceKind, "ExistenceKind", "Existence Kind" );
    m_existenceKind.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_datumElevationFromOsdu, "DatumElevationFromOsdu", 0.0, "Datum Elevation From OSDU" );
    m_datumElevationFromOsdu.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_surfaceEastingFromOsdu, "SurfaceEastingFromOsdu", 0.0, "Surface Easting From OSDU" );
    m_surfaceEastingFromOsdu.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_surfaceNorthingFromOsdu, "SurfaceNorthingFromOsdu", 0.0, "Surface Northing From OSDU" );
    m_surfaceNorthingFromOsdu.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_crsFromOsdu, "CrsFromOsdu", "CRS From OSDU" );
    m_crsFromOsdu.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_unitToMetersFromOsdu, "UnitToMetersFromOsdu", 1.0, "Unit-to-meters Factor From OSDU" );
    m_unitToMetersFromOsdu.uiCapability()->setUiReadOnly( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimOsduWellPath::~RimOsduWellPath()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setWellId( const QString& wellId )
{
    m_wellId = wellId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::wellId() const
{
    return m_wellId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setWellboreId( const QString& wellboreId )
{
    m_wellboreId = wellboreId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::wellboreId() const
{
    return m_wellboreId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setWellboreTrajectoryId( const QString& wellboreTrajectoryId )
{
    m_wellboreTrajectoryId = wellboreTrajectoryId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::wellboreTrajectoryId() const
{
    return m_wellboreTrajectoryId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setDatumElevationFromOsdu( double datumElevation )
{
    m_datumElevationFromOsdu = datumElevation;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RimOsduWellPath::datumElevationFromOsdu() const
{
    return m_datumElevationFromOsdu;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setSurfaceEastingFromOsdu( double surfaceEasting )
{
    m_surfaceEastingFromOsdu = surfaceEasting;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RimOsduWellPath::surfaceEastingFromOsdu() const
{
    return m_surfaceEastingFromOsdu;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setSurfaceNorthingFromOsdu( double surfaceNorthing )
{
    m_surfaceNorthingFromOsdu = surfaceNorthing;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RimOsduWellPath::surfaceNorthingFromOsdu() const
{
    return m_surfaceNorthingFromOsdu;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setCrsFromOsdu( const QString& crs )
{
    m_crsFromOsdu = crs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::crsFromOsdu() const
{
    return m_crsFromOsdu;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setUnitToMetersFromOsdu( double unitToMeters )
{
    m_unitToMetersFromOsdu = unitToMeters;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RimOsduWellPath::unitToMetersFromOsdu() const
{
    return m_unitToMetersFromOsdu;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::setExistenceKind( const QString& existenceKind )
{
    m_existenceKind = existenceKind;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::existenceKind() const
{
    return m_existenceKind;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimOsduWellPath::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    caf::PdmUiGroup* osduGroup = uiOrdering.addNewGroup( "OSDU" );
    osduGroup->add( &m_wellId );
    osduGroup->add( &m_wellboreId );
    osduGroup->add( &m_wellboreTrajectoryId );
    osduGroup->add( &m_existenceKind );
    osduGroup->add( &m_datumElevationFromOsdu );
    osduGroup->add( &m_surfaceEastingFromOsdu );
    osduGroup->add( &m_surfaceNorthingFromOsdu );
    osduGroup->add( &m_crsFromOsdu );
    osduGroup->add( &m_unitToMetersFromOsdu );

    RimWellPath::defineUiOrdering( uiConfigName, uiOrdering );
}
