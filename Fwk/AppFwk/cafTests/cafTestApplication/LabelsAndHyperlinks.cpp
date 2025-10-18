#include "LabelsAndHyperlinks.h"

#include "MainWindow.h"
#include "OptionalFields.h"

#include "cafPdmUiButton.h"
#include "cafPdmUiLabelEditor.h"

#include <QMessageBox>

CAF_PDM_SOURCE_INIT( LabelsAndHyperlinks, "LabelsAndHyperlinks" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
LabelsAndHyperlinks::LabelsAndHyperlinks()
{
    CAF_PDM_InitObject( "Labels And Hyperlinks", "", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_labelTextField, "LabelTextField", "Label Text this text can be very long", "", "", "" );
    m_labelTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_hyperlinkTextField, "HyperlinkTextField", "" );
    m_hyperlinkTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.addNewLabel( "This is a standalone label without PDM field connection" );
    uiOrdering.addNewLabel( "Labels can display informational text in the GUI" );

    // Button with just text of some sort
    uiOrdering.addNewButton( "Simple Button" );

    // Button with text and lambda callback
    uiOrdering.addNewButton( "Click Me!",
                             []()
                             {
                                 // This lambda will be executed when the button is clicked
                                 auto msgBox = new QMessageBox();
                                 msgBox->setWindowTitle( "Button Clicked" );
                                 msgBox->setText( "Hello from the button callback!" );
                                 msgBox->setAttribute( Qt::WA_DeleteOnClose );
                                 msgBox->show();
                             } );

    // Button with icon and callback
    auto* iconButton = uiOrdering.addNewButton( "Icon Button",
                                                []()
                                                {
                                                    auto msgBox = new QMessageBox();
                                                    msgBox->setWindowTitle( "Icon Button" );
                                                    msgBox->setText( "This button has an icon!" );
                                                    msgBox->setAttribute( Qt::WA_DeleteOnClose );
                                                    msgBox->show();
                                                } );
    iconButton->setIconFromResourceString( ":/cafCommandFeatures/Delete.svg" );

    // Button with left alignment (positioned left in its container)
    auto* leftAlignButton = uiOrdering.addNewButton( "Left Aligned Button" );
    leftAlignButton->setAlignment( Qt::AlignLeft | Qt::AlignTop );

    // Button with center alignment (positioned center in its container)
    auto* centerAlignButton = uiOrdering.addNewButton( "Center Aligned Button" );
    centerAlignButton->setAlignment( Qt::AlignHCenter | Qt::AlignTop );

    // Button with right alignment (positioned right in its container)
    auto* rightAlignButton = uiOrdering.addNewButton( "Right Aligned Button" );
    rightAlignButton->setAlignment( Qt::AlignRight | Qt::AlignTop );

    uiOrdering.add( &m_labelTextField );
    uiOrdering.add( &m_hyperlinkTextField );

    uiOrdering.addNewLabel( "Another label at the bottom demonstrating multiple labels" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                 QString                    uiConfigName,
                                                 caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_hyperlinkTextField )
    {
        if ( auto labelEditorAttribute = dynamic_cast<caf::PdmUiLabelEditorAttribute*>( attribute ) )
        {
            labelEditorAttribute->m_linkText =
                "Click <a href=\"dummy\">link</a> to select the <b>Optional Field</b> object.";

            labelEditorAttribute->m_linkActivatedCallback = [this]( const QString& link )
            {
                auto mainWin = MainWindow::instance();
                if ( auto root = mainWin->root() )
                {
                    auto obj = root->descendantsIncludingThisOfType<OptionalFields>();
                    if ( !obj.empty() )
                    {
                        mainWin->setTreeViewSelection( obj.front() );
                    }
                }
            };
        }
    }
}
