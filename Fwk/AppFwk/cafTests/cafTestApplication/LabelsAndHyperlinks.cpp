#include "LabelsAndHyperlinks.h"

#include "cafPdmUiLabelEditor.h"

#include "MainWindow.h"
#include "OptionalFields.h"

#include <QDebug>
#include <QObject>

CAF_PDM_SOURCE_INIT( LabelsAndHyperlinks, "LabelsAndHyperlinks" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
LabelsAndHyperlinks::LabelsAndHyperlinks()
{
    CAF_PDM_InitObject( "Labels And Hyperlinks", "", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_labelTextField, "LabelTextField", "Label Text", "", "", "" );
    m_labelTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );
    m_labelTextField = "This is a regular label with <b>bold</b> and <i>italic</i> text.";

    CAF_PDM_InitFieldNoDefault( &m_hyperlinkTextField,
                                "HyperlinkTextField",
                                "Click <a href=\"link1\">this link</a> or <a href=\"link2\">another link</a> to test "
                                "hyperlinks.",
                                "",
                                "",
                                "" );
    m_hyperlinkTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );
    m_hyperlinkTextField =
        "Click <a href=\"link1\">this link</a> or <a href=\"link2\">another link</a> to test hyperlinks.";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::onEditorWidgetsCreated()
{
    for ( auto editor : m_hyperlinkTextField.uiCapability()->connectedEditors() )
    {
        caf::PdmUiLabelEditor* labelEditor = dynamic_cast<caf::PdmUiLabelEditor*>( editor );
        if ( labelEditor )
        {
            auto txt = "Click  <a href=\"link2\">another link</a> to test";
            m_hyperlinkTextField.uiCapability()->setUiName( txt );

            QObject::connect( labelEditor,
                              &caf::PdmUiLabelEditor::linkActivated,
                              [this]( const QString& link )
                              {
                                  auto mainWin = MainWindow::instance();
                                  auto root    = mainWin->root();
                                  auto obj     = root->descendantsIncludingThisOfType<OptionalFields>();
                                  if ( !obj.empty() )
                                  {
                                      mainWin->selectInTreeView( obj.front() );
                                  }
                              } );
        }
    }
}
