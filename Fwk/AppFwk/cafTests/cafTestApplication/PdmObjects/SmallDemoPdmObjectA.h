#pragma once

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"
#include "cafPdmPtrField.h"

#include <QString>

class SmallDemoPdmObjectA : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    enum class TestEnumType
    {
        T1 = 10,
        T2,
        T3
    };

    SmallDemoPdmObjectA();

    caf::PdmField<double>                     m_doubleField;
    caf::PdmField<int>                        m_intField;
    caf::PdmField<QString>                    m_textField;
    caf::PdmField<caf::AppEnum<TestEnumType>> m_testEnumField;
    caf::PdmPtrField<SmallDemoPdmObjectA*>    m_ptrField;

    caf::PdmProxyValueField<caf::AppEnum<TestEnumType>> m_proxyEnumField;
    void                       setEnumMember( const caf::AppEnum<TestEnumType>& val ) { m_proxyEnumMember = val; }
    caf::AppEnum<TestEnumType> enumMember() const { return m_proxyEnumMember; }
    TestEnumType               m_proxyEnumMember;

    // vector of app enum
    caf::PdmField<std::vector<caf::AppEnum<TestEnumType>>> m_multipleAppEnum;
    caf::PdmField<caf::AppEnum<TestEnumType>>              m_highlightedEnum;

    caf::PdmField<bool> m_toggleField;
    caf::PdmField<bool> m_pushButtonField;

    caf::PdmFieldHandle* objectToggleField() override { return &m_toggleField; }

    void migrateFieldContent( QString& fieldContent, caf::PdmFieldHandle* fieldHandle ) override
    {
        if ( fieldHandle == &m_textField )
        {
            fieldContent = "Migrated Text Field Content";
        }
    }

    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override
    {
        if ( changedField == &m_toggleField )
        {
            std::cout << "Toggle Field changed" << std::endl;
        }
        else if ( changedField == &m_highlightedEnum )
        {
            std::cout << "Highlight value " << m_highlightedEnum().uiText().toStdString() << std::endl;
        }
        else if ( changedField == &m_pushButtonField )
        {
            std::cout << "Push Button pressed " << std::endl;
        }
    }

    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    caf::PdmFieldHandle* userDescriptionField() override { return &m_textField; }

    void enableAutoValueForTestEnum( TestEnumType value )
    {
        // Convert to integer value as this is used when communicating enum from UI to field enum value
        // See PdmUiFieldSpecialization<caf::AppEnum<T>>
        auto enumValue = static_cast<std::underlying_type_t<TestEnumType>>( value );

        m_testEnumField.uiCapability()->enableAndSetAutoValue( enumValue );
    }

    void enableAutoValueForDouble( double value ) { m_doubleField.uiCapability()->enableAndSetAutoValue( value ); }

    void enableAutoValueForInt( double value ) { m_intField.uiCapability()->enableAndSetAutoValue( value ); }

    void setAutoValueForTestEnum( TestEnumType value )
    {
        // Convert to integer value as this is used when communicating enum from UI to field enum value
        // See PdmUiFieldSpecialization<caf::AppEnum<T>>
        auto enumValue = static_cast<std::underlying_type_t<TestEnumType>>( value );

        m_testEnumField.uiCapability()->setAutoValue( enumValue );
    }

    void setAutoValueForDouble( double value )
    {
        m_doubleField.uiCapability()->setAutoValue( value );
        m_doubleField.uiCapability()->updateConnectedEditors();
    }

    void setAutoValueForInt( double value ) { m_intField.uiCapability()->setAutoValue( value ); }

protected:
    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void defineEditorAttribute( const caf::PdmFieldHandle* field,
                                QString                    uiConfigName,
                                caf::PdmUiEditorAttribute* attribute ) override;

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void defineObjectEditorAttribute( QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;
};
