
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

    CAF_PDM_InitFieldNoDefault( &m_variables, "Variables", "Variables" );

    setVariableValue( "MyVariable", 12.0 );
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
            m_doubleValue = variable->valuem_doubleValue;
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
}
