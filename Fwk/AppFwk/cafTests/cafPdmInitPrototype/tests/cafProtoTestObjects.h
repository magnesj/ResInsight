#pragma once

// Test objects for the prototype in issue #1202. Every object exists in two versions with identical content, one
// initialized with the CAF_PDM_Init* macros and one initialized with the prototype functions.

#include "cafProtoFieldInit.h"

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

#include <optional>
#include <vector>

enum class ParityEnum
{
    A,
    B,
    C
};

// Field with custom capability configuration
class ParityCustomField : public caf::PdmField<double>
{
public:
    void configureCapabilities() override { m_isConfigured = true; }

    bool m_isConfigured = false;
};

//==================================================================================================
// Initialized with macros
//==================================================================================================
class ParityMacroBase : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    ParityMacroBase();

    caf::PdmField<QString> m_name;
};

class ParityMacroObj : public ParityMacroBase
{
    CAF_PDM_HEADER_INIT;

public:
    ParityMacroObj();

    caf::PdmField<int>                        m_count;
    caf::PdmField<double>                     m_radius;
    caf::PdmField<bool>                       m_flag;
    caf::PdmField<QString>                    m_text;
    caf::PdmField<caf::AppEnum<ParityEnum>>   m_enum;
    caf::PdmField<std::vector<double>>        m_values;
    caf::PdmField<int>                        m_legacy;
    caf::PdmChildArrayField<ParityMacroBase*> m_children;
    caf::PdmPtrField<ParityMacroBase*>        m_selected;
    ParityCustomField                         m_custom;
};

//==================================================================================================
// Initialized with functions
//==================================================================================================
class ParityFnBase : public cafproto::PdmObjectProto
{
    CAF_PDM_HEADER_INIT;

public:
    ParityFnBase();

    caf::PdmField<QString> m_name;
};

class ParityFnObj : public ParityFnBase
{
    CAF_PDM_HEADER_INIT;

public:
    ParityFnObj();

    caf::PdmField<int>                      m_count;
    caf::PdmField<double>                   m_radius;
    caf::PdmField<bool>                     m_flag;
    caf::PdmField<QString>                  m_text;
    caf::PdmField<caf::AppEnum<ParityEnum>> m_enum;
    caf::PdmField<std::vector<double>>      m_values;
    caf::PdmField<int>                      m_legacy;
    caf::PdmChildArrayField<ParityFnBase*>  m_children;
    caf::PdmPtrField<ParityFnBase*>         m_selected;
    ParityCustomField                       m_custom;
};

// Uses the fixed string alternative and text computed at runtime
class ParityFnExtraObj : public cafproto::PdmObjectProto
{
    CAF_PDM_HEADER_INIT;

public:
    explicit ParityFnExtraObj( const QString& toolTip = "Dynamic tool tip" );

    caf::PdmField<double> m_radius;
    caf::PdmField<int>    m_dynamic;
};

//==================================================================================================
// Objects for the runtime benchmark: ten fields that are not scriptable
//==================================================================================================
class BenchPlainMacroObj : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    BenchPlainMacroObj();

    caf::PdmField<int>                      m_intVal;
    caf::PdmField<double>                   m_dblVal;
    caf::PdmField<bool>                     m_boolVal;
    caf::PdmField<QString>                  m_strVal;
    caf::PdmField<caf::AppEnum<ParityEnum>> m_enumVal;
    caf::PdmField<std::vector<double>>      m_vecDbl;
    caf::PdmField<std::vector<QString>>     m_vecStr;
    caf::PdmField<std::optional<double>>    m_optDbl;
    caf::PdmField<caf::FilePath>            m_fileA;
    caf::PdmField<std::pair<bool, double>>  m_pairVal;
};

class BenchPlainFnObj : public cafproto::PdmObjectProto
{
    CAF_PDM_HEADER_INIT;

public:
    BenchPlainFnObj();

    caf::PdmField<int>                      m_intVal;
    caf::PdmField<double>                   m_dblVal;
    caf::PdmField<bool>                     m_boolVal;
    caf::PdmField<QString>                  m_strVal;
    caf::PdmField<caf::AppEnum<ParityEnum>> m_enumVal;
    caf::PdmField<std::vector<double>>      m_vecDbl;
    caf::PdmField<std::vector<QString>>     m_vecStr;
    caf::PdmField<std::optional<double>>    m_optDbl;
    caf::PdmField<caf::FilePath>            m_fileA;
    caf::PdmField<std::pair<bool, double>>  m_pairVal;
};

//==================================================================================================
// Objects for the runtime benchmark: ten scriptable fields
//==================================================================================================
class BenchScriptMacroObj : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    BenchScriptMacroObj();

    caf::PdmField<int>                  m_intVal;
    caf::PdmField<double>               m_dblVal;
    caf::PdmField<bool>                 m_boolVal;
    caf::PdmField<QString>              m_strVal;
    caf::PdmField<float>                m_fltVal;
    caf::PdmField<std::vector<double>>  m_vecDbl;
    caf::PdmField<std::vector<QString>> m_vecStr;
    caf::PdmField<std::vector<int>>     m_vecInt;
    caf::PdmField<int>                  m_intVal2;
    caf::PdmField<double>               m_dblVal2;
};

class BenchScriptFnObj : public cafproto::PdmObjectProto
{
    CAF_PDM_HEADER_INIT;

public:
    BenchScriptFnObj();

    caf::PdmField<int>                  m_intVal;
    caf::PdmField<double>               m_dblVal;
    caf::PdmField<bool>                 m_boolVal;
    caf::PdmField<QString>              m_strVal;
    caf::PdmField<float>                m_fltVal;
    caf::PdmField<std::vector<double>>  m_vecDbl;
    caf::PdmField<std::vector<QString>> m_vecStr;
    caf::PdmField<std::vector<int>>     m_vecInt;
    caf::PdmField<int>                  m_intVal2;
    caf::PdmField<double>               m_dblVal2;
};
