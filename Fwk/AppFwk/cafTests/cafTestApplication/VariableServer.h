#pragma once

#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"
#include "cafPdmPtrField.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class ValueMultiplexer : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    ValueMultiplexer();

    caf::PdmObject*     source() const;
    QString             sourceFieldName() const;
    caf::PdmValueField* sourceField() const;

    caf::PdmObject*     destination() const;
    QString             destinationFieldName() const;
    caf::PdmValueField* destinationField() const;

    void setSource( caf::PdmObject* source, const QString& fieldName );
    void setDestination( caf::PdmObject* destination, const QString& fieldName );

private:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

private:
    caf::PdmPtrField<caf::PdmObject*> m_source;
    caf::PdmField<QString>            m_sourceFieldName;

    caf::PdmPtrField<caf::PdmObject*> m_destination;
    caf::PdmField<QString>            m_destinationFieldName;
};

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

    void            setRoot( caf::PdmObject* root );
    caf::PdmObject* root() const;

    void           setVariableValue( const QString& name, double value );
    NamedVariable* variable( const QString& name );

    void addMultiplexer( caf::PdmObject* source,
                         const QString&  fieldName,
                         caf::PdmObject* destination,
                         const QString&  destinationFieldName );

    void removeMultiplexer( caf::PdmObject* source,
                            const QString&  fieldName,
                            caf::PdmObject* destination,
                            const QString&  destinationFieldName );

    void notifyFieldChanged( caf::PdmObject* source, const QString& fieldName, QVariant newValue );

private:
    caf::PdmPtrField<caf::PdmObject*> m_root;

    caf::PdmChildArrayField<NamedVariable*> m_variables;

    caf::PdmChildArrayField<ValueMultiplexer*> m_valueMultiplexers;
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
