
#include "VariableServer.h"
#include "../../cafUserInterface/cafPdmUiPushButtonEditor.h"
#include "MainWindow.h"
#include "cafPdmUiListEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( NamedVariable, "NamedVariable" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
NamedVariable::NamedVariable()
{
    CAF_PDM_InitObject( "NamedVariable",
                        ":/images/win/filenew.png",
                        "This object is a demo of the CAF framework",
                        "This object is a demo of the CAF framework" );

    CAF_PDM_InitField( &m_doubleValue, "DoubleValue", 0.0, "DoubleValue" );
    CAF_PDM_InitFieldNoDefault( &m_name, "Name", "Name" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NamedVariable::setDoubleValue( double value )
{
    if ( m_doubleValue != value )
    {
        m_doubleValue = value;
        notifyListeners();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NamedVariable::addListener( caf::PdmValueField* field )
{
    if ( std::find( m_listeners.begin(), m_listeners.end(), field ) == m_listeners.end() )
    {
        m_listeners.push_back( field );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NamedVariable::removeListener( caf::PdmValueField* field )
{
    m_listeners.erase( std::remove( m_listeners.begin(), m_listeners.end(), field ), m_listeners.end() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NamedVariable::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                      const QVariant&            oldValue,
                                      const QVariant&            newValue )
{
    if ( changedField == &m_doubleValue )
    {
        notifyListeners();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NamedVariable::notifyListeners()
{
    // Related code
    // RimCommandIssueFieldChanged::redo()

    for ( caf::PdmValueField* field : m_listeners )
    {
        if ( !field ) continue;

        QVariant oldValue = field->toQVariant();
        QVariant newValue( m_doubleValue );

        field->setFromQVariant( newValue );

        if ( auto uiFieldHandle = field->uiCapability() )
        {
            uiFieldHandle->notifyFieldChanged( oldValue, newValue );
        }
    }
}

CAF_PDM_SOURCE_INIT( VariableServer, "VariableServer" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
VariableServer::VariableServer()
{
    CAF_PDM_InitObject( "VariableServer",
                        ":/images/win/filenew.png",
                        "This object is a demo of the CAF framework",
                        "This object is a demo of the CAF framework" );

    CAF_PDM_InitFieldNoDefault( &m_root, "Root", "Root" );

    CAF_PDM_InitFieldNoDefault( &m_variables, "Variables", "Variables" );
    CAF_PDM_InitFieldNoDefault( &m_valueMultiplexers, "ValueMultiplexers", "Value Multiplexers" );

    setVariableValue( "MyVariable", 12.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableServer::setRoot( caf::PdmObject* root )
{
    m_root = root;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* VariableServer::root() const
{
    return m_root;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
NamedVariable* VariableServer::variable( const QString& name )
{
    for ( auto variable : m_variables.childrenByType() )
    {
        if ( variable->name() == name )
        {
            return variable;
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableServer::addMultiplexer( caf::PdmObject* source,
                                     const QString&  fieldName,
                                     caf::PdmObject* destination,
                                     const QString&  destinationFieldName )
{
    if ( !source || !destination )
    {
        return;
    }

    auto sourceValueField      = dynamic_cast<caf::PdmValueField*>( source->findField( fieldName ) );
    auto destinationValueField = dynamic_cast<caf::PdmValueField*>( destination->findField( destinationFieldName ) );

    if ( sourceValueField && destinationValueField )
    {
        for ( auto m : m_valueMultiplexers )
        {
            if ( m->source() == source && m->sourceFieldName() == fieldName && m->destination() == destination &&
                 m->destinationFieldName() == destinationFieldName )
            {
                return;
            }
        }

        auto multiplexer = new ValueMultiplexer();
        multiplexer->setSource( source, fieldName );
        multiplexer->setDestination( destination, destinationFieldName );

        m_valueMultiplexers.push_back( multiplexer );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableServer::removeMultiplexer( caf::PdmObject* source,
                                        const QString&  fieldName,
                                        caf::PdmObject* destination,
                                        const QString&  destinationFieldName )
{
    if ( !source || !destination )
    {
        return;
    }

    std::vector<ValueMultiplexer*> objectsToDelete;

    for ( auto m : m_valueMultiplexers )
    {
        // erase the multiplexer if it exists
        if ( m->source() == source && m->sourceFieldName() == fieldName && m->destination() == destination &&
             m->destinationFieldName() == destinationFieldName )
        {
            objectsToDelete.push_back( m );
        }
    }

    for ( auto m : objectsToDelete )
    {
        m_valueMultiplexers.removeChild( m );

        delete m;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableServer::notifyFieldChanged( caf::PdmObject* source, const QString& fieldName, QVariant newValue )
{
    for ( auto m : m_valueMultiplexers )
    {
        if ( m->source() == source && m->sourceFieldName() == fieldName )
        {
            if ( auto destinationField = m->destinationField() )
            {
                destinationField->setFromQVariant( newValue );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableServer::setVariableValue( const QString& name, double value )
{
    NamedVariable* variableToUpdate = nullptr;

    for ( auto variable : m_variables.childrenByType() )
    {
        if ( variable->name() == name )
        {
            variableToUpdate = variable;
            break;
        }
    }

    if ( !variableToUpdate )
    {
        variableToUpdate = new NamedVariable();
        variableToUpdate->setName( name );
        m_variables.push_back( variableToUpdate );
    }

    if ( variableToUpdate )
    {
        variableToUpdate->setDoubleValue( value );
    }
}

CAF_PDM_SOURCE_INIT( VariableClient, "VariableClient" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
VariableClient::VariableClient()
{
    CAF_PDM_InitObject( "VariableClient",
                        ":/images/win/filenew.png",
                        "This object is a demo of the CAF framework",
                        "This object is a demo of the CAF framework" );

    CAF_PDM_InitField( &m_doubleValue, "DoubleValue", 0.0, "DoubleValue" );

    CAF_PDM_InitField( &m_connect, "Connect", false, "Connect" );
    caf::PdmUiPushButtonEditor::configureEditorLabelHidden( &m_connect );

    CAF_PDM_InitField( &m_disconnect, "DisConnect", false, "Disconnect" );
    caf::PdmUiPushButtonEditor::configureEditorLabelHidden( &m_disconnect );

    connectVariable( "MyVariable" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableClient::connectVariable( const QString& name )
{
    auto mainWindows    = MainWindow::instance();
    auto variableServer = mainWindows->variableServer();
    if ( variableServer )
    {
        auto variable = variableServer->variable( name );
        if ( variable )
        {
            variable->addListener( &m_doubleValue );
            //m_doubleValue = variable->valuem_doubleValue;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableClient::disconnectVariable( const QString& name )
{
    auto mainWindows    = MainWindow::instance();
    auto variableServer = mainWindows->variableServer();
    if ( variableServer )
    {
        auto variable = variableServer->variable( name );
        if ( variable )
        {
            variable->removeListener( &m_doubleValue );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableClient::initAfterRead()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableClient::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                            QString                    uiConfigName,
                                            caf::PdmUiEditorAttribute* attribute )
{
    if ( auto attrib = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>( attribute ) )
    {
        if ( &m_connect == field )
        {
            attrib->m_buttonText = "Connect";
        }
        if ( &m_disconnect == field )
        {
            attrib->m_buttonText = "Disconnect";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void VariableClient::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                       const QVariant&            oldValue,
                                       const QVariant&            newValue )
{
    if ( &m_connect == changedField )
    {
        m_connect = false;

        connectVariable( "MyVariable" );
    }
    else if ( &m_disconnect == changedField )
    {
        m_disconnect = false;

        disconnectVariable( "MyVariable" );
    }
    else if ( &m_doubleValue == changedField )
    {
        auto mainWindows    = MainWindow::instance();
        auto variableServer = mainWindows->variableServer();
        if ( variableServer )
        {
            variableServer->notifyFieldChanged( this, m_doubleValue.keyword(), newValue );
        }
    }
}

CAF_PDM_SOURCE_INIT( ValueMultiplexer, "ValueMultiplexer" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ValueMultiplexer::ValueMultiplexer()
{
    CAF_PDM_InitObject( "ValueMultiplexer",
                        ":/images/win/filenew.png",
                        "This object is a demo of the CAF framework",
                        "This object is a demo of the CAF framework" );

    CAF_PDM_InitFieldNoDefault( &m_source, "Source", "Source" );
    CAF_PDM_InitFieldNoDefault( &m_sourceFieldName, "SourceFieldName", "Source Fieldname" );

    CAF_PDM_InitFieldNoDefault( &m_destination, "Destination", "Destination" );
    CAF_PDM_InitFieldNoDefault( &m_destinationFieldName, "DestinationFieldName", "Destination Fieldname" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* ValueMultiplexer::source() const
{
    return m_source();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString ValueMultiplexer::sourceFieldName() const
{
    return m_sourceFieldName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmValueField* ValueMultiplexer::sourceField() const
{
    if ( m_source )
    {
        if ( auto valueField = dynamic_cast<caf::PdmValueField*>( m_source->findField( m_sourceFieldName() ) ) )
        {
            return valueField;
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* ValueMultiplexer::destination() const
{
    return m_destination();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString ValueMultiplexer::destinationFieldName() const
{
    return m_destinationFieldName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmValueField* ValueMultiplexer::destinationField() const
{
    if ( m_destination )
    {
        if ( auto valueField = dynamic_cast<caf::PdmValueField*>( m_destination->findField( m_destinationFieldName() ) ) )
        {
            return valueField;
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void ValueMultiplexer::setSource( caf::PdmObject* source, const QString& fieldName )
{
    m_source          = source;
    m_sourceFieldName = fieldName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void ValueMultiplexer::setDestination( caf::PdmObject* destination, const QString& fieldName )
{
    m_destination          = destination;
    m_destinationFieldName = fieldName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> ValueMultiplexer::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_source || fieldNeedingOptions == &m_destination )
    {
        auto root = MainWindow::instance()->variableServer()->root();
        if ( root )
        {
            auto allObjects = root->descendantsOfType<caf::PdmObject>();

            for ( auto obj : allObjects )
            {
                options.push_back( caf::PdmOptionItemInfo( obj->uiName(), obj ) );
            }
        }
    }
    else if ( fieldNeedingOptions == &m_sourceFieldName )
    {
        if ( m_source() )
        {
            auto allFields = m_source()->fields();

            for ( auto field : allFields )
            {
                if ( auto valueField = dynamic_cast<caf::PdmValueField*>( field ) )
                {
                    options.push_back( caf::PdmOptionItemInfo( valueField->keyword(), valueField->keyword() ) );
                }
            }
        }
    }
    else if ( fieldNeedingOptions == &m_destinationFieldName )
    {
        if ( m_destination() )
        {
            auto allFields = m_destination()->fields();

            for ( auto field : allFields )
            {
                if ( auto valueField = dynamic_cast<caf::PdmValueField*>( field ) )
                {
                    options.push_back( caf::PdmOptionItemInfo( valueField->keyword(), valueField->keyword() ) );
                }
            }
        }
    }

    return options;
}
