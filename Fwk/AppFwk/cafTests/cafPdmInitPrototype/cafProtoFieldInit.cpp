#include "cafProtoFieldInit.h"

#include "cafPdmUiFieldHandle.h"

#include <QHash>

#include <array>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace cafproto
{
namespace
{
    //==================================================================================================
    /// Identity of static UI information. All pointers refer to string literals, so equal pointers
    /// always mean equal text. Two fields with equal keys can share one PdmUiItemInfo, because the
    /// static information is never modified after it is created. Changes for a single instance are
    /// stored in PdmUiItem, not in the static information.
    //==================================================================================================
    struct InfoKey
    {
        std::array<const void*, 6> pointers;

        bool operator==( const InfoKey& other ) const = default;
    };

    struct InfoKeyHash
    {
        size_t operator()( const InfoKey& key ) const
        {
            size_t seed = 0;
            for ( const void* p : key.pointers )
            {
                seed ^= std::hash<const void*>()( p ) + 0x9e3779b97f4a7c15ULL + ( seed << 6 ) + ( seed >> 2 );
            }
            return seed;
        }
    };

    struct InfoTexts
    {
        QString uiName;
        QString icon;
        QString toolTip;
        QString whatsThis;
        QString keyword;
    };

    class InfoRegistry
    {
    public:
        static InfoRegistry& instance()
        {
            static InfoRegistry registry;
            return registry;
        }

        // Fast path for text given as string literals
        template <typename CreateTexts>
        caf::PdmUiItemInfo* findOrCreate( const InfoKey& key, CreateTexts createTexts )
        {
            // Lock free lookup in a small cache owned by the thread
            ThreadCache& cache = threadCache();
            CacheEntry&  entry = cache.entries[InfoKeyHash()( key ) % cache.entries.size()];
            if ( entry.info && entry.key == key ) return entry.info;

            entry.key  = key;
            entry.info = findOrCreateShared( key, createTexts );
            return entry.info;
        }

        template <typename CreateTexts>
        caf::PdmUiItemInfo* findOrCreateShared( const InfoKey& key, CreateTexts createTexts )
        {
            {
                std::shared_lock lock( m_mutex );
                if ( auto it = m_interned.find( key ); it != m_interned.end() ) return it->second.get();
            }

            auto info = create( createTexts() );

            std::unique_lock lock( m_mutex );
            auto [it, isInserted] = m_interned.try_emplace( key, std::move( info ) );
            return it->second.get();
        }

        // Text computed at runtime is identified by content
        caf::PdmUiItemInfo* findOrCreate( const InfoTexts& texts )
        {
            QString key = texts.keyword + QChar( 0 ) + texts.uiName + QChar( 0 ) + texts.icon + QChar( 0 ) +
                          texts.toolTip + QChar( 0 ) + texts.whatsThis;

            std::unique_lock lock( m_mutex );
            auto&            info = m_byContent[key];
            if ( !info ) info = create( texts );
            return info.get();
        }

        template <typename CreateTexts>
        caf::PdmUiItemInfo* findOrCreate( const QString& classKeyword, const void* keyword, CreateTexts createTexts )
        {
            std::pair<QString, const void*> key( classKeyword, keyword );
            {
                std::shared_lock lock( m_mutex );
                if ( auto it = m_byClass.find( key ); it != m_byClass.end() ) return it->get();
            }

            auto info = create( createTexts() );

            std::unique_lock lock( m_mutex );
            auto&            entry = m_byClass[key];
            if ( !entry ) entry = std::move( info );
            return entry.get();
        }

        caf::PdmUiItemInfo* createForInstance( const InfoTexts& texts )
        {
            std::unique_lock lock( m_mutex );
            m_perInstance.push_back( create( texts ) );
            return m_perInstance.back().get();
        }

        size_t size() const
        {
            std::shared_lock lock( m_mutex );
            return m_interned.size() + m_byContent.size() + static_cast<size_t>( m_byClass.size() ) + m_perInstance.size();
        }

        void clear()
        {
            std::unique_lock lock( m_mutex );
            m_interned.clear();
            m_byContent.clear();
            m_byClass.clear();
            m_perInstance.clear();
            m_generation++;
        }

    private:
        struct CacheEntry
        {
            InfoKey             key{};
            caf::PdmUiItemInfo* info = nullptr;
        };

        struct ThreadCache
        {
            std::array<CacheEntry, 256> entries;
            size_t                      generation = 0;
        };

        ThreadCache& threadCache()
        {
            thread_local ThreadCache cache;

            // clear() is only used by benchmarks, when no other thread creates objects
            size_t generation = m_generation.load( std::memory_order_relaxed );
            if ( cache.generation != generation )
            {
                cache            = ThreadCache();
                cache.generation = generation;
            }
            return cache;
        }

        static std::shared_ptr<caf::PdmUiItemInfo> create( const InfoTexts& texts )
        {
            return std::make_shared<caf::PdmUiItemInfo>( texts.uiName, texts.icon, texts.toolTip, texts.whatsThis, texts.keyword );
        }

        mutable std::shared_mutex                                                     m_mutex;
        std::unordered_map<InfoKey, std::shared_ptr<caf::PdmUiItemInfo>, InfoKeyHash> m_interned;
        std::unordered_map<QString, std::shared_ptr<caf::PdmUiItemInfo>>              m_byContent;
        QHash<std::pair<QString, const void*>, std::shared_ptr<caf::PdmUiItemInfo>>   m_byClass;
        std::vector<std::shared_ptr<caf::PdmUiItemInfo>>                              m_perInstance;
        std::atomic<size_t>                                                           m_generation = 0;
    };

    //--------------------------------------------------------------------------------------------------
    /// QString for a keyword given as string literal. The conversion is done once for each thread and
    /// literal, later calls return a shared copy.
    //--------------------------------------------------------------------------------------------------
    const QString& keywordAsQString( std::string_view keyword )
    {
        struct Entry
        {
            const char* literal = nullptr;
            QString     text;
        };
        thread_local std::array<Entry, 256> cache;

        Entry& entry = cache[std::hash<const void*>()( keyword.data() ) % cache.size()];
        if ( entry.literal != keyword.data() )
        {
            entry.literal = keyword.data();
            entry.text    = detail::toQString( keyword );
        }
        return entry.text;
    }

    std::atomic<MetadataStorage> s_metadataStorage = MetadataStorage::INTERNED;

    const char* const s_emptyText = "";

} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void setMetadataStorage( MetadataStorage storage )
{
    s_metadataStorage = storage;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t metadataObjectCount()
{
    return InfoRegistry::instance().size();
}

//--------------------------------------------------------------------------------------------------
/// Only valid when no objects initialized by the prototype functions exist
//--------------------------------------------------------------------------------------------------
void clearMetadata()
{
    InfoRegistry::instance().clear();
}

//==================================================================================================
///
//==================================================================================================
struct PdmFieldInitBase::DynamicTexts
{
    std::array<QString, 4> texts;
    std::array<bool, 4>    isSet = {};
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmFieldInitBase::PdmFieldInitBase( caf::PdmFieldHandle* field, std::string_view keyword, PdmStaticText uiName )
    : m_field( field )
    , m_keyword( keyword )
    , m_texts{ uiName.c_str(), s_emptyText, s_emptyText, s_emptyText }
    , m_toolTipDecorator( nullptr )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmFieldInitBase::~PdmFieldInitBase()
{
    caf::PdmUiFieldHandle* uiField = m_field->uiCapability();
    if ( !uiField ) return;

    auto createTexts = [this]() -> InfoTexts
    {
        InfoTexts texts;

        auto text = [this]( TextRole role )
        {
            auto index = static_cast<size_t>( role );
            if ( m_dynamicTexts && m_dynamicTexts->isSet[index] ) return m_dynamicTexts->texts[index];
            return QString::fromUtf8( m_texts[index] );
        };

        texts.uiName    = text( TextRole::UI_NAME );
        texts.icon      = text( TextRole::ICON );
        texts.toolTip   = text( TextRole::TOOL_TIP );
        texts.whatsThis = text( TextRole::WHATS_THIS );
        texts.keyword   = detail::toQString( m_keyword );

        if ( m_toolTipDecorator )
            texts.toolTip = m_toolTipDecorator( texts.toolTip, detail::toQString( m_scriptKeyword ) );

        return texts;
    };

    caf::PdmUiItemInfo* info = nullptr;

    if ( m_dynamicTexts )
    {
        info = InfoRegistry::instance().findOrCreate( createTexts() );
    }
    else
    {
        switch ( s_metadataStorage.load( std::memory_order_relaxed ) )
        {
            case MetadataStorage::INTERNED:
            {
                InfoKey key{ { m_texts[0], m_texts[1], m_texts[2], m_texts[3], m_keyword.data(), m_scriptKeyword.data() } };
                info = InfoRegistry::instance().findOrCreate( key, createTexts );
                break;
            }
            case MetadataStorage::CLASS_REGISTRY:
                info = InfoRegistry::instance().findOrCreate( m_field->ownerClass(), m_keyword.data(), createTexts );
                break;
            case MetadataStorage::PER_INSTANCE:
                info = InfoRegistry::instance().createForInstance( createTexts() );
                break;
        }
    }

    uiField->setUiItemInfo( info );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmFieldInitBase::setText( TextRole role, PdmStaticText text )
{
    m_texts[static_cast<size_t>( role )] = text.c_str();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmFieldInitBase::setDynamicText( TextRole role, const QString& text )
{
    if ( !m_dynamicTexts ) m_dynamicTexts = std::make_unique<DynamicTexts>();

    auto index                   = static_cast<size_t>( role );
    m_dynamicTexts->texts[index] = text;
    m_dynamicTexts->isSet[index] = true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmFieldInitBase::setScriptKeyword( std::string_view scriptKeyword, ToolTipDecorator toolTipDecorator )
{
    m_scriptKeyword    = scriptKeyword;
    m_toolTipDecorator = toolTipDecorator;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmFieldInitBase::addKeywordAlias( std::string_view alias )
{
    m_field->registerKeywordAlias( detail::toQString( alias ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmObjectInit::PdmObjectInit( caf::PdmObject* object, const QString& classKeyword, PdmStaticText uiName )
    : m_object( object )
    , m_classKeyword( classKeyword )
    , m_texts{ uiName.c_str(), s_emptyText, s_emptyText, s_emptyText }
    , m_scriptClassName( nullptr )
    , m_scriptComment( nullptr )
    , m_scriptClassRegistrar( nullptr )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmObjectInit::~PdmObjectInit()
{
    auto createTexts = [this]() -> InfoTexts
    {
        return { QString::fromUtf8( m_texts[0] ),
                 QString::fromUtf8( m_texts[1] ),
                 QString::fromUtf8( m_texts[2] ),
                 QString::fromUtf8( m_texts[3] ),
                 {} };
    };

    InfoKey key{ { m_texts[0], m_texts[1], m_texts[2], m_texts[3], nullptr, nullptr } };
    m_object->setUiItemInfo( InfoRegistry::instance().findOrCreate( key, createTexts ) );

    if ( m_scriptClassRegistrar )
    {
        QString scriptClassName = m_scriptClassName ? QString::fromUtf8( m_scriptClassName ) : m_classKeyword;
        QString scriptComment = m_scriptComment ? QString::fromUtf8( m_scriptComment ) : QString::fromUtf8( m_texts[3] );

        m_scriptClassRegistrar( m_classKeyword, scriptClassName, scriptComment );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmObjectInit& PdmObjectInit::withIcon( PdmStaticText iconResourceName )
{
    m_texts[1] = iconResourceName.c_str();
    return *this;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmObjectInit& PdmObjectInit::withToolTip( PdmStaticText toolTip )
{
    m_texts[2] = toolTip.c_str();
    return *this;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmObjectInit& PdmObjectInit::withWhatsThis( PdmStaticText whatsThis )
{
    m_texts[3] = whatsThis.c_str();
    return *this;
}

//--------------------------------------------------------------------------------------------------
/// The class keyword is the keyword of the class currently being constructed, because virtual calls
/// in a constructor are dispatched to that class. This is the same result as in the macros.
//--------------------------------------------------------------------------------------------------
PdmObjectInit PdmObjectProto::initObject( PdmStaticText uiName )
{
    QString keyword = classKeyword();
    registerClassKeyword( keyword );

    return PdmObjectInit( this, keyword, uiName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmObjectProto::registerField( caf::PdmFieldHandle* field, std::string_view keyword )
{
    // classKeyword() creates a vector of strings on every call. All fields of a class are registered in sequence by
    // the constructor, so the keyword of the class currently being constructed is cached.
    struct ClassKeywordCache
    {
        const std::type_info* type = nullptr;
        QString               keyword;
    };
    thread_local ClassKeywordCache cache;

    const std::type_info* type = &typeid( *this );
    if ( cache.type != type )
    {
        cache.type    = type;
        cache.keyword = classKeyword();
    }

    field->configureCapabilities();
    field->setOwnerClass( cache.keyword );
    addField( field, keywordAsQString( keyword ) );
}

} // namespace cafproto
