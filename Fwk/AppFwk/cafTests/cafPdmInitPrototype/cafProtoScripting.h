#pragma once

// Prototype for issue #1202: scripting support for the functions in cafProtoFieldInit.h.
//
// Include this header in translation units that use initScriptableField(), withScriptKeyword() or
// PdmObjectInit::withScripting(). In the framework, these definitions belong in cafPdmFieldScriptingCapability.h and
// cafPdmObjectScriptingCapability.h, where the scriptable macros are defined today.

#include "cafProtoFieldInit.h"

#include "cafPdmFieldScriptingCapability.h"
#include "cafPdmObjectScriptingCapability.h"

namespace cafproto
{
template <typename FieldType>
struct PdmFieldScriptingHook
{
    static void addCapability( FieldType* field, const QString& scriptKeyword )
    {
        caf::AddScriptingCapabilityToField( field, scriptKeyword );
    }

    static QString toolTip( const QString& toolTip, const QString& scriptKeyword )
    {
        return caf::PdmAbstractFieldScriptingCapability::helpString( toolTip, scriptKeyword );
    }
};

struct PdmObjectScriptingHook
{
    static void
        registerScriptClass( const QString& classKeyword, const QString& scriptClassName, const QString& scriptComment )
    {
        caf::PdmObjectScriptingCapabilityRegister::registerScriptClassNameAndComment( classKeyword,
                                                                                      scriptClassName,
                                                                                      scriptComment );
    }
};

} // namespace cafproto
