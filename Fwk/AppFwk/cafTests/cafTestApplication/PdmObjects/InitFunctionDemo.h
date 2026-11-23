#pragma once

#include "cafProtoFieldInit.h"

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <optional>

//==================================================================================================
/// Prototype for issue #1202. Two objects with identical content, for visual comparison in the
/// property editor: InitMacroDemo is initialized with the CAF_PDM_Init* macros, InitFunctionDemo is
/// initialized with functions.
//==================================================================================================
class InitMacroDemo : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    InitMacroDemo();

private:
    caf::PdmField<QString>               m_name;
    caf::PdmField<double>                m_radius;
    caf::PdmField<int>                   m_count;
    caf::PdmField<bool>                  m_isActive;
    caf::PdmField<std::optional<double>> m_optionalValue;
};

class InitFunctionDemo : public cafproto::PdmObjectProto
{
    CAF_PDM_HEADER_INIT;

public:
    InitFunctionDemo();

private:
    caf::PdmField<QString>               m_name;
    caf::PdmField<double>                m_radius;
    caf::PdmField<int>                   m_count;
    caf::PdmField<bool>                  m_isActive;
    caf::PdmField<std::optional<double>> m_optionalValue;
};
