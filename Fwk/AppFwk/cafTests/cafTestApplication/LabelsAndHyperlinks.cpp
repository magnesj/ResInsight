#include "LabelsAndHyperlinks.h"

#include "cafPdmUiLabelEditor.h"

CAF_PDM_SOURCE_INIT( LabelsAndHyperlinks, "LabelsAndHyperlinks" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
LabelsAndHyperlinks::LabelsAndHyperlinks()
{
    CAF_PDM_InitObject( "Labels And Hyperlinks", "", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_labelTextField, "LabelTextField", "Label Text", "", "", "" );
    m_labelTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_hyperlinkTextField, "HyperlinkTextField", "Hyperlink Text", "", "", "" );
    m_hyperlinkTextField.uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                            const QVariant&            oldValue,
                                            const QVariant&            newValue )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void LabelsAndHyperlinks::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                 QString                    uiConfigName,
                                                 caf::PdmUiEditorAttribute* attribute )
{
}