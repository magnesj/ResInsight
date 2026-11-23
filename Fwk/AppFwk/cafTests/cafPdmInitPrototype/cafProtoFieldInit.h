#pragma once

// Prototype for issue #1202, steps 1.2 and 1.3: initialization of PDM fields and objects with functions instead of the
// CAF_PDM_Init* macros.
//
// The functions are members of PdmObjectProto, a stand-in for caf::PdmObject, so the prototype can be compared with the
// macros without changing the framework.
//
// Design
//   - The typed front end (function templates and PdmFieldInit<FieldType>) is thin. All work that does not depend on the
//     field type is done by PdmFieldInitBase, implemented in the cpp file.
//   - Keywords are verified at compile time by the consteval constructors of PdmKeyword and PdmScriptKeyword.
//   - UI text is given as PdmStaticText. No QString is created when an object is constructed, except the first time.
//   - The static PdmUiItemInfo is shared by all fields with the same text. See cafProtoFieldInit.cpp.
//   - This header does not include any scripting headers. A translation unit using initScriptableField() or
//     withScriptKeyword() must include cafProtoScripting.h.

#include "cafProtoFixedString.h"
#include "cafProtoKeyword.h"
#include "cafProtoStaticText.h"

#include "cafPdmObject.h"

#include <concepts>
#include <memory>
#include <string_view>
#include <type_traits>

namespace cafproto
{
//==================================================================================================
/// Storage of static UI information. INTERNED is the proposed design, the others are used for
/// comparison in benchmarks.
//==================================================================================================
enum class MetadataStorage
{
    INTERNED, // One PdmUiItemInfo for each distinct set of string literals
    CLASS_REGISTRY, // One PdmUiItemInfo for each class keyword and field keyword
    PER_INSTANCE // One PdmUiItemInfo for each field instance
};

void   setMetadataStorage( MetadataStorage storage );
size_t metadataObjectCount();
void   clearMetadata();

// Defined in cafProtoScripting.h. Keeps the scripting dependency out of this header.
template <typename FieldType>
struct PdmFieldScriptingHook;
struct PdmObjectScriptingHook;

using ToolTipDecorator     = QString ( * )( const QString& toolTip, const QString& scriptKeyword );
using ScriptClassRegistrar = void ( * )( const QString& classKeyword,
                                         const QString& scriptClassName,
                                         const QString& scriptComment );

namespace detail
{
    inline QString toQString( std::string_view text )
    {
        return QString::fromLatin1( text.data(), static_cast<qsizetype>( text.size() ) );
    }

    template <typename DataType>
    void applyDefault( caf::PdmField<DataType>& field, const std::type_identity_t<DataType>& defaultValue )
    {
        field.setDefaultValue( defaultValue );
        field = defaultValue;
    }

    // An enum value can be used as default value for an AppEnum field
    template <typename EnumType>
        requires std::is_enum_v<EnumType>
    void applyDefault( caf::PdmField<caf::AppEnum<EnumType>>& field, EnumType defaultValue )
    {
        applyDefault( field, caf::AppEnum<EnumType>( defaultValue ) );
    }
} // namespace detail

//==================================================================================================
/// Non-template part of the field builder. The destructor assigns the static UI information to the
/// field, so the order of the calls in a builder chain does not matter.
//==================================================================================================
class PdmFieldInitBase
{
public:
    PdmFieldInitBase( const PdmFieldInitBase& )            = delete;
    PdmFieldInitBase& operator=( const PdmFieldInitBase& ) = delete;

protected:
    PdmFieldInitBase( caf::PdmFieldHandle* field, std::string_view keyword, PdmStaticText uiName );
    ~PdmFieldInitBase();

    enum class TextRole
    {
        UI_NAME,
        ICON,
        TOOL_TIP,
        WHATS_THIS
    };

    void setText( TextRole role, PdmStaticText text );
    void setDynamicText( TextRole role, const QString& text );
    void setScriptKeyword( std::string_view scriptKeyword, ToolTipDecorator toolTipDecorator );
    void addKeywordAlias( std::string_view alias );

private:
    struct DynamicTexts;

    caf::PdmFieldHandle*          m_field;
    std::string_view              m_keyword;
    std::string_view              m_scriptKeyword;
    const char*                   m_texts[4];
    ToolTipDecorator              m_toolTipDecorator;
    std::unique_ptr<DynamicTexts> m_dynamicTexts;
};

//==================================================================================================
/// Builder returned by initField()
//==================================================================================================
template <typename FieldType>
class PdmFieldInit : public PdmFieldInitBase
{
public:
    PdmFieldInit( FieldType* field, std::string_view keyword, PdmStaticText uiName )
        : PdmFieldInitBase( field, keyword, uiName )
        , m_typedField( field )
    {
    }

    PdmFieldInit( FieldType*       field,
                  std::string_view keyword,
                  PdmStaticText    uiName,
                  std::string_view scriptKeyword,
                  ToolTipDecorator toolTipDecorator )
        : PdmFieldInitBase( field, keyword, uiName )
        , m_typedField( field )
    {
        setScriptKeyword( scriptKeyword, toolTipDecorator );
    }

    template <typename DefaultType>
    PdmFieldInit& withDefault( const DefaultType& defaultValue )
    {
        detail::applyDefault( *m_typedField, defaultValue );
        return *this;
    }

    PdmFieldInit& withIcon( PdmStaticText iconResourceName )
    {
        setText( TextRole::ICON, iconResourceName );
        return *this;
    }

    PdmFieldInit& withToolTip( PdmStaticText toolTip )
    {
        setText( TextRole::TOOL_TIP, toolTip );
        return *this;
    }

    PdmFieldInit& withWhatsThis( PdmStaticText whatsThis )
    {
        setText( TextRole::WHATS_THIS, whatsThis );
        return *this;
    }

    // Text computed at runtime. The constraint prevents ambiguity for string literals.
    template <std::same_as<QString> StringType>
    PdmFieldInit& withUiName( const StringType& uiName )
    {
        setDynamicText( TextRole::UI_NAME, uiName );
        return *this;
    }

    template <std::same_as<QString> StringType>
    PdmFieldInit& withToolTip( const StringType& toolTip )
    {
        setDynamicText( TextRole::TOOL_TIP, toolTip );
        return *this;
    }

    PdmFieldInit& withKeywordAlias( PdmKeyword alias )
    {
        addKeywordAlias( alias.view() );
        return *this;
    }

    // Expose the field to scripting with a keyword different from the XML keyword
    PdmFieldInit& withScriptKeyword( PdmScriptKeyword scriptKeyword )
    {
        PdmFieldScriptingHook<FieldType>::addCapability( m_typedField, detail::toQString( scriptKeyword.view() ) );
        setScriptKeyword( scriptKeyword.view(), &PdmFieldScriptingHook<FieldType>::toolTip );
        return *this;
    }

private:
    FieldType* m_typedField;
};

//==================================================================================================
/// Builder returned by initFieldN(), the fixed string alternative. The keyword is part of the type,
/// so withScripting() can verify it at compile time.
//==================================================================================================
template <typename FieldType, FixedString Keyword>
class PdmFieldInitN : public PdmFieldInitBase
{
public:
    PdmFieldInitN( FieldType* field, std::string_view keyword, PdmStaticText uiName )
        : PdmFieldInitBase( field, keyword, uiName )
        , m_typedField( field )
    {
    }

    template <typename DefaultType>
    PdmFieldInitN& withDefault( const DefaultType& defaultValue )
    {
        detail::applyDefault( *m_typedField, defaultValue );
        return *this;
    }

    PdmFieldInitN& withIcon( PdmStaticText iconResourceName )
    {
        setText( TextRole::ICON, iconResourceName );
        return *this;
    }

    PdmFieldInitN& withToolTip( PdmStaticText toolTip )
    {
        setText( TextRole::TOOL_TIP, toolTip );
        return *this;
    }

    PdmFieldInitN& withWhatsThis( PdmStaticText whatsThis )
    {
        setText( TextRole::WHATS_THIS, whatsThis );
        return *this;
    }

    PdmFieldInitN& withScripting()
    {
        static_assert( isValidScriptKeywordV<Keyword>,
                       "Keyword used for scripting must be in compatible formatted camel casing starting with an "
                       "uppercase letter" );
        static constexpr auto text = Keyword;
        PdmFieldScriptingHook<FieldType>::addCapability( m_typedField, detail::toQString( text.view() ) );
        setScriptKeyword( text.view(), &PdmFieldScriptingHook<FieldType>::toolTip );
        return *this;
    }

private:
    FieldType* m_typedField;
};

//==================================================================================================
/// Builder returned by initObject()
//==================================================================================================
class PdmObjectInit
{
public:
    PdmObjectInit( caf::PdmObject* object, const QString& classKeyword, PdmStaticText uiName );
    ~PdmObjectInit();

    PdmObjectInit( const PdmObjectInit& )            = delete;
    PdmObjectInit& operator=( const PdmObjectInit& ) = delete;

    PdmObjectInit& withIcon( PdmStaticText iconResourceName );
    PdmObjectInit& withToolTip( PdmStaticText toolTip );
    PdmObjectInit& withWhatsThis( PdmStaticText whatsThis );

    // Script class name is the class keyword, and the comment is the "What's this" text
    template <typename Hook = PdmObjectScriptingHook>
    PdmObjectInit& withScripting()
    {
        m_scriptClassRegistrar = &Hook::registerScriptClass;
        return *this;
    }

    template <typename Hook = PdmObjectScriptingHook>
    PdmObjectInit& withScripting( PdmStaticText scriptClassName, PdmStaticText scriptComment )
    {
        m_scriptClassRegistrar = &Hook::registerScriptClass;
        m_scriptClassName      = scriptClassName.c_str();
        m_scriptComment        = scriptComment.c_str();
        return *this;
    }

private:
    caf::PdmObject*      m_object;
    QString              m_classKeyword;
    const char*          m_texts[4];
    const char*          m_scriptClassName;
    const char*          m_scriptComment;
    ScriptClassRegistrar m_scriptClassRegistrar;
};

//==================================================================================================
/// Stand-in for caf::PdmObject with the proposed functions
//==================================================================================================
class PdmObjectProto : public caf::PdmObject
{
protected:
    PdmObjectInit initObject( PdmStaticText uiName );

    template <typename FieldType>
    PdmFieldInit<FieldType> initField( FieldType& field, PdmKeyword keyword, PdmStaticText uiName )
    {
        caf::addXmlCapabilityToField( &field );
        caf::addUiCapabilityToField( &field );
        registerField( &field, keyword.view() );

        return PdmFieldInit<FieldType>( &field, keyword.view(), uiName );
    }

    // Same argument order as CAF_PDM_InitField
    template <typename FieldType, typename DefaultType>
    PdmFieldInit<FieldType>
        initField( FieldType& field, PdmKeyword keyword, const DefaultType& defaultValue, PdmStaticText uiName )
    {
        caf::addXmlCapabilityToField( &field );
        caf::addUiCapabilityToField( &field );
        registerField( &field, keyword.view() );
        detail::applyDefault( field, defaultValue );

        return PdmFieldInit<FieldType>( &field, keyword.view(), uiName );
    }

    // The field keyword is also the scripting keyword. PdmScriptKeyword verifies the keyword at compile time.
    template <typename FieldType>
    PdmFieldInit<FieldType> initScriptableField( FieldType& field, PdmScriptKeyword keyword, PdmStaticText uiName )
    {
        caf::addXmlCapabilityToField( &field );
        caf::addUiCapabilityToField( &field );
        registerField( &field, keyword.view() );
        PdmFieldScriptingHook<FieldType>::addCapability( &field, detail::toQString( keyword.view() ) );

        return PdmFieldInit<FieldType>( &field,
                                        keyword.view(),
                                        uiName,
                                        keyword.view(),
                                        &PdmFieldScriptingHook<FieldType>::toolTip );
    }

    template <typename FieldType, typename DefaultType>
    PdmFieldInit<FieldType>
        initScriptableField( FieldType& field, PdmScriptKeyword keyword, const DefaultType& defaultValue, PdmStaticText uiName )
    {
        caf::addXmlCapabilityToField( &field );
        caf::addUiCapabilityToField( &field );
        registerField( &field, keyword.view() );
        detail::applyDefault( field, defaultValue );
        PdmFieldScriptingHook<FieldType>::addCapability( &field, detail::toQString( keyword.view() ) );

        return PdmFieldInit<FieldType>( &field,
                                        keyword.view(),
                                        uiName,
                                        keyword.view(),
                                        &PdmFieldScriptingHook<FieldType>::toolTip );
    }

    // Fixed string alternative: initFieldN<"Keyword">( m_field, "Ui Name" )
    template <FixedString Keyword, typename FieldType>
    PdmFieldInitN<FieldType, Keyword> initFieldN( FieldType& field, PdmStaticText uiName )
    {
        static_assert( isValidXmlKeywordV<Keyword>, "Invalid XML keyword" );
        static constexpr auto text = Keyword;

        caf::addXmlCapabilityToField( &field );
        caf::addUiCapabilityToField( &field );
        registerField( &field, text.view() );

        return PdmFieldInitN<FieldType, Keyword>( &field, text.view(), uiName );
    }

private:
    // Calls configureCapabilities(), sets the owner class and adds the field to the object
    void registerField( caf::PdmFieldHandle* field, std::string_view keyword );
};

} // namespace cafproto
