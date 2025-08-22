
#pragma once

#include "gtest/gtest.h"

#include "cafAppEnum.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmChildField.h"
#include "cafPdmCodeGenerator.h"
#include "cafPdmDocument.h"
#include "cafPdmField.h"
#include "cafPdmFieldScriptingCapability.h"
#include "cafPdmObject.h"
#include "cafPdmObjectGroup.h"
#include "cafPdmObjectMethod.h"
#include "cafPdmObjectScriptingCapability.h"
#include "cafPdmPointer.h"
#include "cafPdmProxyValueField.h"
#include "cafPdmPythonGenerator.h"
#include "cafPdmReferenceHelper.h"

#include <QFile>
#include <memory>

/// Demo objects to show the usage of the Pdm system
enum class MyEnum
{
    T1,
    T2,
    T3,
    T4,
    T5,
    T6
};

class MyAppEnumField : public caf::PdmField<caf::AppEnum<MyEnum>>
{
public:
    MyAppEnumField() {

    };
};

namespace caf
{
template <>
class PdmFieldScriptingCapability<MyAppEnumField> : public PdmAbstractFieldScriptingCapability
{
public:
    PdmFieldScriptingCapability( MyAppEnumField* field, const QString& fieldName, bool giveOwnership )
        : PdmAbstractFieldScriptingCapability( field, fieldName, giveOwnership )
    {
        m_field = field;
    }

    void writeToField( QTextStream&          inputStream,
                       PdmObjectFactory*     objectFactory,
                       PdmScriptIOMessages*  errorMessageContainer,
                       bool                  stringsAreQuoted    = true,
                       caf::PdmObjectHandle* existingObjectsRoot = nullptr ) override
    {
        if ( this->isIOWriteable() )
        {
            caf::AppEnum<MyEnum> value;

            PdmFieldScriptingCapabilityIOHandler<caf::AppEnum<MyEnum>>::writeToField( value,
                                                                                      inputStream,
                                                                                      errorMessageContainer,
                                                                                      stringsAreQuoted );
            m_field->setValue( value );
        }
    }

    void readFromField( QTextStream& outputStream, bool quoteStrings = true, bool quoteNonBuiltins = false ) const override
    {
        PdmFieldScriptingCapabilityIOHandler<caf::AppEnum<MyEnum>>::readFromField( m_field->value(),
                                                                                   outputStream,
                                                                                   quoteStrings,
                                                                                   quoteNonBuiltins );
    }

    QStringList enumScriptTexts() const override
    {
        QStringList enumTexts;

        for ( size_t i = 0; i < AppEnum<MyEnum>::size(); i++ )
        {
            auto enumText = AppEnum<MyEnum>::text( AppEnum<MyEnum>::fromIndex( i ) );
            enumTexts.push_back( enumText );
        }

        return enumTexts;
    }

    QString dataType() const override { return "str"; }

private:
    PdmField<AppEnum<MyEnum>>* m_field;
};
}; // namespace caf

class SimpleObj : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    SimpleObj();

    /// Assignment and copying of PDM objects is not focus for the features. This is only a
    /// "would it work" test
    SimpleObj( const SimpleObj& other );

    ~SimpleObj() {}

    caf::PdmField<double>              m_position;
    caf::PdmField<double>              m_dir;
    caf::PdmField<double>              m_up;
    caf::PdmField<std::vector<double>> m_numbers;
    caf::PdmProxyValueField<double>    m_proxyDouble;

    void   setDoubleMember( const double& d );
    double doubleMember() const;

    double m_doubleMember;
};

class DemoPdmObject : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    DemoPdmObject();

    ~DemoPdmObject();

    // Fields
    caf::PdmField<double>  m_doubleField;
    caf::PdmField<int>     m_intField;
    caf::PdmField<QString> m_textField;
    MyAppEnumField         m_myAppEnum;

    caf::PdmChildField<SimpleObj*> m_simpleObjPtrField;
    caf::PdmChildField<SimpleObj*> m_simpleObjPtrField2;
};

class InheritedDemoObj : public DemoPdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    enum TestEnumType
    {
        T1,
        T2,
        T3
    };

    InheritedDemoObj();

    caf::PdmField<std::vector<QString>>  m_texts;
    caf::PdmField<std::vector<double>>   m_numbers;
    caf::PdmField<std::optional<double>> m_optionalNumber;

    caf::PdmField<caf::AppEnum<TestEnumType>> m_testEnumField;
    caf::PdmChildArrayField<SimpleObj*>       m_simpleObjectsField;
};

class MyPdmDocument : public caf::PdmDocument
{
    CAF_PDM_HEADER_INIT;

public:
    MyPdmDocument();

    caf::PdmChildArrayField<PdmObjectHandle*> objects;
};
