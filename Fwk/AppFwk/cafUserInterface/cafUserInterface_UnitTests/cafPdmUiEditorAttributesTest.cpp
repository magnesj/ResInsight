#include "gtest/gtest.h"

#include "cafPdmField.h"
#include "cafPdmLogging.h"
#include "cafPdmObject.h"
#include "cafPdmUiCheckBoxEditor.h"
#include "cafPdmUiDoubleSliderEditor.h"
#include "cafPdmUiLineEditor.h"
#include "cafPdmUiSliderEditor.h"

#include <QCheckBox>
#include <QIntValidator>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

#include <memory>
#include <vector>

using namespace caf;

class EditorAttributesTestObj : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    EditorAttributesTestObj()
    {
        CAF_PDM_InitObject( "EditorAttributesTestObj" );
        CAF_PDM_InitField( &m_int, "Int", 5, "Int Field" );
        CAF_PDM_InitField( &m_bool, "Bool", true, "Bool Field" );
        CAF_PDM_InitField( &m_text, "Text", QString( "text" ), "Text Field" );
    }

    caf::PdmField<int>     m_int;
    caf::PdmField<bool>    m_bool;
    caf::PdmField<QString> m_text;
};
CAF_PDM_SOURCE_INIT( EditorAttributesTestObj, "EditorAttributesTestObj" );

namespace
{
class WarningCollector : public caf::PdmLogger
{
public:
    int  level() const override { return static_cast<int>( caf::PdmLogLevel::PDM_LL_DEBUG ); }
    void setLevel( int ) override {}
    void error( const QString& ) override {}
    void warning( const QString& message ) override { warnings.push_back( message ); }
    void info( const QString& ) override {}
    void debug( const QString& ) override {}

    std::vector<QString> warnings;
};
} // namespace

//--------------------------------------------------------------------------------------------------
/// Collects PDM warnings so that tests can verify that supported attributes never trigger the
/// "Unsupported attribute" validation warning
//--------------------------------------------------------------------------------------------------
class PdmUiEditorAttributesTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_collector = std::make_shared<WarningCollector>();
        caf::PdmLogging::registerLogger( m_collector );
    }

    void TearDown() override { caf::PdmLogging::unregisterLogger( m_collector ); }

    std::vector<QString>& warnings() { return m_collector->warnings; }

    std::shared_ptr<WarningCollector> m_collector;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, SliderRangeAndSpinBoxVisibility )
{
    EditorAttributesTestObj obj;
    auto*                   ui = obj.m_int.uiCapability();
    ui->setAttribute( PdmUiSliderEditor::Keys::MINIMUM, 2 );
    ui->setAttribute( PdmUiSliderEditor::Keys::MAXIMUM, 40 );
    ui->setAttribute( PdmUiSliderEditor::Keys::STEP, 3 );
    ui->setAttribute( PdmUiSliderEditor::Keys::SHOW_SPIN_BOX, false );

    QWidget           parentWidget;
    PdmUiSliderEditor editor;
    editor.setUiField( ui );
    editor.createWidgets( &parentWidget );
    editor.updateUi();

    auto* slider  = editor.editorWidget()->findChild<QSlider*>();
    auto* spinBox = editor.editorWidget()->findChild<QSpinBox*>();
    ASSERT_NE( slider, nullptr );
    ASSERT_NE( spinBox, nullptr );

    EXPECT_EQ( 2, slider->minimum() );
    EXPECT_EQ( 40, slider->maximum() );
    EXPECT_EQ( 3, slider->singleStep() );
    EXPECT_EQ( 2, spinBox->minimum() );
    EXPECT_EQ( 40, spinBox->maximum() );
    EXPECT_FALSE( spinBox->isVisibleTo( editor.editorWidget() ) );
    EXPECT_TRUE( warnings().empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, SliderDefaultsWithoutAttributes )
{
    EditorAttributesTestObj obj;

    QWidget           parentWidget;
    PdmUiSliderEditor editor;
    editor.setUiField( obj.m_int.uiCapability() );
    editor.createWidgets( &parentWidget );
    editor.updateUi();

    auto* slider = editor.editorWidget()->findChild<QSlider*>();
    ASSERT_NE( slider, nullptr );
    EXPECT_EQ( 0, slider->minimum() );
    EXPECT_EQ( 10, slider->maximum() );
    EXPECT_EQ( 5, slider->value() );
}

//--------------------------------------------------------------------------------------------------
/// Attributes set for a specific ui config must only affect that config
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, SliderUsesConfigSpecificAttributes )
{
    EditorAttributesTestObj obj;
    auto*                   ui = obj.m_int.uiCapability();
    ui->setAttribute( PdmUiSliderEditor::Keys::MAXIMUM, 20 );
    ui->setAttribute( PdmUiSliderEditor::Keys::MAXIMUM, 99, "Special" );

    QWidget           parentWidget;
    PdmUiSliderEditor editor;
    editor.setUiField( ui );
    editor.createWidgets( &parentWidget );

    editor.updateUi( "" );
    auto* slider = editor.editorWidget()->findChild<QSlider*>();
    ASSERT_NE( slider, nullptr );
    EXPECT_EQ( 20, slider->maximum() );

    editor.updateUi( "Special" );
    EXPECT_EQ( 99, slider->maximum() );
    EXPECT_TRUE( warnings().empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, LineEditAttributes )
{
    EditorAttributesTestObj obj;
    auto*                   ui = obj.m_text.uiCapability();
    ui->setAttribute( PdmUiLineEditor::Keys::MAXIMUM_WIDTH, 123 );
    ui->setAttribute( PdmUiLineEditor::Keys::PLACEHOLDER_TEXT, "type here" );

    QIntValidator validator( 0, 100 );
    ui->setAttribute<QValidator*>( PdmUiLineEditor::Keys::VALIDATOR, &validator );

    obj.m_text = "";

    QWidget         parentWidget;
    PdmUiLineEditor editor;
    editor.setUiField( ui );
    editor.createWidgets( &parentWidget );
    editor.updateUi();

    auto* lineEdit = editor.editorWidget()->findChild<QLineEdit*>();
    if ( !lineEdit ) lineEdit = qobject_cast<QLineEdit*>( editor.editorWidget() );
    ASSERT_NE( lineEdit, nullptr );

    EXPECT_EQ( 123, lineEdit->maximumWidth() );
    EXPECT_EQ( "type here", lineEdit->placeholderText().toStdString() );
    EXPECT_EQ( &validator, lineEdit->validator() );
    EXPECT_TRUE( warnings().empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, CheckBoxNativeAttributeUsesFieldNameAsText )
{
    EditorAttributesTestObj obj;
    auto*                   ui = obj.m_bool.uiCapability();
    ui->setAttribute( PdmUiCheckBoxEditor::Keys::USE_NATIVE_CHECKBOX, true );

    QWidget             parentWidget;
    PdmUiCheckBoxEditor editor;
    editor.setUiField( ui );
    editor.createWidgets( &parentWidget );
    editor.updateUi();

    auto* checkBox = qobject_cast<QCheckBox*>( editor.editorWidget() );
    ASSERT_NE( checkBox, nullptr );
    EXPECT_TRUE( checkBox->isChecked() );
    EXPECT_EQ( "Bool Field", checkBox->text().toStdString() );
    EXPECT_TRUE( warnings().empty() );
}

//--------------------------------------------------------------------------------------------------
/// Editors must report attributes that they do not support, and only those
//--------------------------------------------------------------------------------------------------
TEST_F( PdmUiEditorAttributesTest, UnsupportedAttributeIsReportedByEditor )
{
    EditorAttributesTestObj obj;
    auto*                   ui = obj.m_bool.uiCapability();
    ui->setAttribute( PdmUiSliderEditor::Keys::MINIMUM, 1 ); // Slider key, not supported by a check box

    QWidget             parentWidget;
    PdmUiCheckBoxEditor editor;
    editor.setUiField( ui );
    editor.createWidgets( &parentWidget );
    editor.updateUi();

    ASSERT_EQ( 1u, warnings().size() );
    EXPECT_TRUE( warnings().front().contains( PdmUiSliderEditor::Keys::MINIMUM ) );
}

//--------------------------------------------------------------------------------------------------
/// Every key an editor consumes must be part of the editor's supported set, otherwise valid use
/// triggers a runtime warning
//--------------------------------------------------------------------------------------------------
TEST( PdmUiEditorSupportedAttributesTest, KeysAreRegisteredAsSupported )
{
    EXPECT_TRUE( PdmUiSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiSliderEditor::Keys::MINIMUM ) );
    EXPECT_TRUE( PdmUiSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiSliderEditor::Keys::MAXIMUM ) );
    EXPECT_TRUE( PdmUiSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiSliderEditor::Keys::SHOW_SPIN_BOX ) );
    EXPECT_TRUE( PdmUiSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiSliderEditor::Keys::STEP ) );

    EXPECT_TRUE( PdmUiDoubleSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiDoubleSliderEditor::Keys::MINIMUM ) );
    EXPECT_TRUE( PdmUiDoubleSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiDoubleSliderEditor::Keys::MAXIMUM ) );
    EXPECT_TRUE( PdmUiDoubleSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiDoubleSliderEditor::Keys::DECIMALS ) );
    EXPECT_TRUE( PdmUiDoubleSliderEditor::SUPPORTED_ATTRIBUTES.count( PdmUiDoubleSliderEditor::Keys::SLIDER_TICK_COUNT ) );

    EXPECT_TRUE( PdmUiLineEditor::SUPPORTED_ATTRIBUTES.count( PdmUiLineEditor::Keys::VALIDATOR ) );
    EXPECT_TRUE( PdmUiLineEditor::SUPPORTED_ATTRIBUTES.count( PdmUiLineEditor::Keys::PLACEHOLDER_TEXT ) );
    EXPECT_TRUE( PdmUiLineEditor::SUPPORTED_ATTRIBUTES.count( PdmUiLineEditor::Keys::MAXIMUM_WIDTH ) );

    EXPECT_TRUE( PdmUiCheckBoxEditor::SUPPORTED_ATTRIBUTES.count( PdmUiCheckBoxEditor::Keys::USE_NATIVE_CHECKBOX ) );
    EXPECT_TRUE( PdmUiCheckBoxEditor::SUPPORTED_ATTRIBUTES.count( PdmUiCheckBoxEditor::Keys::WORD_WRAP ) );
}
