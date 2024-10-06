#pragma once

#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class NamedVariable : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    NamedVariable();

    void    setName( const QString& name ) { m_name = name; }
    QString name() const { return m_name; }

    void setDoubleValue( double value );

    void addListener( caf::PdmValueField* field );
    void removeListener( caf::PdmValueField* field );

private:
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void notifyListeners();

private:
    caf::PdmField<QString> m_name;
    caf::PdmField<double>  m_doubleValue;

    std::vector<caf::PdmValueField*> m_listeners;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class VariableServer : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    VariableServer();

    void           setVariableValue( const QString& name, double value );
    NamedVariable* variable( const QString& name );

private:
    caf::PdmChildArrayField<NamedVariable*> m_variables;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class VariableClient : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    VariableClient();

    void connectVariable( const QString& name );
    void disconnectVariable( const QString& name );

private:
    caf::PdmField<double> m_doubleValue;

    caf::PdmField<bool> m_connect;
    caf::PdmField<bool> m_disconnect;

protected:
    void initAfterRead() override;

    void defineEditorAttribute( const caf::PdmFieldHandle* field,
                                QString                    uiConfigName,
                                caf::PdmUiEditorAttribute* attribute ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
};
