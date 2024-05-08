#include "RimOsduWellPath.h"

#include "RicfCommandObject.h"

#include "RifWellPathImporter.h"

// #include "RimProject.h"
// #include "RimTools.h"

// #include "cafUtils.h"

// #include "QDir"
// #include "QFileInfo"

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

    CAF_PDM_InitFieldNoDefault( &m_fileId, "FileId", "File Id" );
    m_fileId.uiCapability()->setUiReadOnly( true );
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
void RimOsduWellPath::setFileId( const QString& fileId )
{
    m_fileId = fileId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimOsduWellPath::fileId() const
{
    return m_fileId;
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
    osduGroup->add( &m_fileId );

    RimWellPath::defineUiOrdering( uiConfigName, uiOrdering );
}

//--------------------------------------------------------------------------------------------------
/// Read JSON or ascii file containing well path data
//--------------------------------------------------------------------------------------------------
// bool RimOsduWellPath::readWellPathFile( QString* errorMessage, RifWellPathImporter* wellPathImporter, bool setWellNameForExport )
// {
//     if ( caf::Utils::fileExists( filePath() ) )
//     {
//         RifWellPathImporter::WellData wellData = wellPathImporter->readWellData( filePath(), m_wellPathIndexInFile() );

//         RifWellPathImporter::WellMetaData wellMetaData = RifWellPathImporter::readWellMetaData( filePath(), m_wellPathIndexInFile() );
//         // General well info

//         if ( setWellNameForExport )
//         {
//             setName( wellData.m_name );
//         }
//         else
//         {
//             setNameNoUpdateOfExportName( wellData.m_name );
//         }

//         id           = wellMetaData.m_id;
//         sourceSystem = wellMetaData.m_sourceSystem;
//         utmZone      = wellMetaData.m_utmZone;
//         updateUser   = wellMetaData.m_updateUser;
//         setSurveyType( wellMetaData.m_surveyType );
//         updateDate = wellMetaData.m_updateDate.toString( "d MMMM yyyy" );

//         if ( m_useAutoGeneratedPointAtSeaLevel )
//         {
//             ensureWellPathStartAtSeaLevel( wellData.m_wellPathGeometry.p() );
//         }

//         setWellPathGeometry( wellData.m_wellPathGeometry.p() );

//         // Now that the data is read, we know if this is an SSIHUB wellpath that needs to be stored in the
//         // cache folder along with the project file. If it is, move the pathfile reference to the m_filePathInCache
//         // in order to avoid it being handled as an externalFilePath by the RimProject class

//         if ( isStoredInCache() && !m_filePath().path().isEmpty() )
//         {
//             m_filePathInCache = m_filePath().path();
//             m_filePath        = QString( "" );
//         }

//         return true;
//     }
//     else
//     {
//         if ( errorMessage ) ( *errorMessage ) = "Could not find the well path file: " + filePath();
//         return false;
//     }
// }
