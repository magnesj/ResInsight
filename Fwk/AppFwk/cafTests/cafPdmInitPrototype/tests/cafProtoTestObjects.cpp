#include "cafProtoTestObjects.h"

#include "cafProtoScripting.h"

namespace caf
{
template <>
void AppEnum<ParityEnum>::setUp()
{
    addItem( ParityEnum::A, "A", "Value A" );
    addItem( ParityEnum::B, "B", "Value B" );
    addItem( ParityEnum::C, "C", "Value C" );
    setDefault( ParityEnum::A );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( ParityMacroBase, "ParityMacroBase" );
CAF_PDM_SOURCE_INIT( ParityMacroObj, "ParityMacroObj", "OldParityMacroObj" );
CAF_PDM_SOURCE_INIT( ParityFnBase, "ParityFnBase" );
CAF_PDM_SOURCE_INIT( ParityFnObj, "ParityFnObj", "OldParityFnObj" );
CAF_PDM_SOURCE_INIT( ParityFnExtraObj, "ParityFnExtraObj" );
CAF_PDM_SOURCE_INIT( BenchPlainMacroObj, "BenchPlainMacroObj" );
CAF_PDM_SOURCE_INIT( BenchPlainFnObj, "BenchPlainFnObj" );
CAF_PDM_SOURCE_INIT( BenchScriptMacroObj, "BenchScriptMacroObj" );
CAF_PDM_SOURCE_INIT( BenchScriptFnObj, "BenchScriptFnObj" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ParityMacroBase::ParityMacroBase()
{
    CAF_PDM_InitScriptableObjectWithNameAndComment( "Parity Base",
                                                    ":/Base.svg",
                                                    "Base tool tip",
                                                    "Base whats this",
                                                    "ParityMacroBaseScript",
                                                    "Base script comment" );

    CAF_PDM_InitScriptableField( &m_name, "Name", QString( "Unnamed" ), "Name", "", "Name tool tip", "Name whats this" );
    m_name.registerKeywordAlias( "OldName" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ParityMacroObj::ParityMacroObj()
{
    CAF_PDM_InitScriptableObject( "Parity Object", ":/Object.svg", "Object tool tip", "Object whats this" );

    CAF_PDM_InitField( &m_count, "Count", 3, "Count" );
    CAF_PDM_InitScriptableField( &m_radius, "Radius", 1.5, "Radius", ":/Radius.svg", "Radius tool tip" );
    CAF_PDM_InitField( &m_flag, "Flag", true, "Flag", "", "", "Flag whats this" );
    CAF_PDM_InitFieldNoDefault( &m_text, "Text", "Text" );
    CAF_PDM_InitScriptableField( &m_enum, "EnumValue", ParityEnum::B, "Enum" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_values, "Values", "Values" );
    CAF_PDM_InitScriptableFieldWithScriptKeyword( &m_legacy, "legacy_count", "LegacyCount", 7, "Legacy" );
    CAF_PDM_InitFieldNoDefault( &m_children, "Children", "Children" );
    CAF_PDM_InitFieldNoDefault( &m_selected, "Selected", "Selected" );
    CAF_PDM_InitField( &m_custom, "Custom", 2.5, "Custom" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ParityFnBase::ParityFnBase()
{
    initObject( "Parity Base" )
        .withIcon( ":/Base.svg" )
        .withToolTip( "Base tool tip" )
        .withWhatsThis( "Base whats this" )
        .withScripting( "ParityFnBaseScript", "Base script comment" );

    initScriptableField( m_name, "Name", QString( "Unnamed" ), "Name" )
        .withToolTip( "Name tool tip" )
        .withWhatsThis( "Name whats this" )
        .withKeywordAlias( "OldName" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ParityFnObj::ParityFnObj()
{
    initObject( "Parity Object" )
        .withIcon( ":/Object.svg" )
        .withToolTip( "Object tool tip" )
        .withWhatsThis( "Object whats this" )
        .withScripting();

    initField( m_count, "Count", 3, "Count" );
    initScriptableField( m_radius, "Radius", "Radius" ).withDefault( 1.5 ).withIcon( ":/Radius.svg" ).withToolTip( "Radius tool tip" );
    initField( m_flag, "Flag", "Flag" ).withWhatsThis( "Flag whats this" ).withDefault( true );
    initField( m_text, "Text", "Text" );
    initScriptableField( m_enum, "EnumValue", ParityEnum::B, "Enum" );
    initScriptableField( m_values, "Values", "Values" );
    initField( m_legacy, "legacy_count", 7, "Legacy" ).withScriptKeyword( "LegacyCount" );
    initField( m_children, "Children", "Children" );
    initField( m_selected, "Selected", "Selected" );
    initField( m_custom, "Custom", 2.5, "Custom" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ParityFnExtraObj::ParityFnExtraObj( const QString& toolTip )
{
    initObject( "Parity Extra" );

    initFieldN<"Radius">( m_radius, "Radius" ).withDefault( 1.5 ).withToolTip( "Radius tool tip" ).withScripting();
    initField( m_dynamic, "Dynamic", 1, "Dynamic" ).withToolTip( toolTip );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
BenchPlainMacroObj::BenchPlainMacroObj()
{
    CAF_PDM_InitObject( "Bench Plain" );

    CAF_PDM_InitField( &m_intVal, "IntVal", 0, "Int" );
    CAF_PDM_InitField( &m_dblVal, "DblVal", 0.0, "Double" );
    CAF_PDM_InitField( &m_boolVal, "BoolVal", false, "Bool" );
    CAF_PDM_InitField( &m_strVal, "StrVal", QString(), "String" );
    CAF_PDM_InitField( &m_enumVal, "EnumVal", ParityEnum::A, "Enum" );
    CAF_PDM_InitFieldNoDefault( &m_vecDbl, "VecDbl", "Vector of doubles" );
    CAF_PDM_InitFieldNoDefault( &m_vecStr, "VecStr", "Vector of strings" );
    CAF_PDM_InitFieldNoDefault( &m_optDbl, "OptDbl", "Optional double" );
    CAF_PDM_InitFieldNoDefault( &m_fileA, "FileA", "File A" );
    auto pairInit = std::make_pair( false, 0.0 );
    CAF_PDM_InitField( &m_pairVal, "PairVal", pairInit, "Pair" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
BenchPlainFnObj::BenchPlainFnObj()
{
    initObject( "Bench Plain" );

    initField( m_intVal, "IntVal", 0, "Int" );
    initField( m_dblVal, "DblVal", 0.0, "Double" );
    initField( m_boolVal, "BoolVal", false, "Bool" );
    initField( m_strVal, "StrVal", QString(), "String" );
    initField( m_enumVal, "EnumVal", ParityEnum::A, "Enum" );
    initField( m_vecDbl, "VecDbl", "Vector of doubles" );
    initField( m_vecStr, "VecStr", "Vector of strings" );
    initField( m_optDbl, "OptDbl", "Optional double" );
    initField( m_fileA, "FileA", "File A" );
    auto pairInit = std::make_pair( false, 0.0 );
    initField( m_pairVal, "PairVal", pairInit, "Pair" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
BenchScriptMacroObj::BenchScriptMacroObj()
{
    CAF_PDM_InitScriptableObject( "Bench Script" );

    CAF_PDM_InitScriptableField( &m_intVal, "IntVal", 0, "Int" );
    CAF_PDM_InitScriptableField( &m_dblVal, "DblVal", 0.0, "Double" );
    CAF_PDM_InitScriptableField( &m_boolVal, "BoolVal", false, "Bool" );
    CAF_PDM_InitScriptableField( &m_strVal, "StrVal", QString(), "String", "", "A tool tip" );
    CAF_PDM_InitScriptableField( &m_fltVal, "FltVal", 0.0f, "Float" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_vecDbl, "VecDbl", "Vector of doubles" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_vecStr, "VecStr", "Vector of strings" );
    CAF_PDM_InitScriptableFieldNoDefault( &m_vecInt, "VecInt", "Vector of ints" );
    CAF_PDM_InitScriptableField( &m_intVal2, "IntVal2", 0, "Int 2" );
    CAF_PDM_InitScriptableField( &m_dblVal2, "DblVal2", 0.0, "Double 2" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
BenchScriptFnObj::BenchScriptFnObj()
{
    initObject( "Bench Script" ).withScripting();

    initScriptableField( m_intVal, "IntVal", 0, "Int" );
    initScriptableField( m_dblVal, "DblVal", 0.0, "Double" );
    initScriptableField( m_boolVal, "BoolVal", false, "Bool" );
    initScriptableField( m_strVal, "StrVal", QString(), "String" ).withToolTip( "A tool tip" );
    initScriptableField( m_fltVal, "FltVal", 0.0f, "Float" );
    initScriptableField( m_vecDbl, "VecDbl", "Vector of doubles" );
    initScriptableField( m_vecStr, "VecStr", "Vector of strings" );
    initScriptableField( m_vecInt, "VecInt", "Vector of ints" );
    initScriptableField( m_intVal2, "IntVal2", 0, "Int 2" );
    initScriptableField( m_dblVal2, "DblVal2", 0.0, "Double 2" );
}
