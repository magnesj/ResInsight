// DeckGui.hpp - CAF GUI wrappers for OPM Deck classes

#pragma once

#include "cafPdmChildArrayField.h"
#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiOrdering.h"

#include "RimNamedObject.h"
#include "cafPdmProxyValueField.h"
#include <QString>
#include <QStringList>

#include "opm/input/eclipse/Deck/Deck.hpp"

#include <memory>
#include <vector>

// Forward declarations
namespace Opm
{
class DeckItem;
class DeckRecord;
class DeckKeyword;
class Deck;
} // namespace Opm

//==================================================================================================
/// A CAF wrapper for DeckItem
//==================================================================================================
class PdmDeckItem : public RimNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    PdmDeckItem();
    explicit PdmDeckItem( Opm::DeckItem& deckItem );
    ~PdmDeckItem() override = default;

    // Initialize from a DeckItem
    void setFromDeckItem( Opm::DeckItem& deckItem );

    // Fields for UI display
    caf::PdmField<QString>              m_dataType;
    caf::PdmField<QString>              m_valueRepresentation;
    caf::PdmField<size_t>               m_dataSize;
    caf::PdmField<bool>                 m_defaultApplied;
    caf::PdmField<std::vector<QString>> m_stringValues;
    caf::PdmField<std::vector<double>>  m_doubleValues;
    caf::PdmField<std::vector<int>>     m_intValues;

    caf::PdmProxyValueField<QString> m_fractureCreationSummary;

protected:
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

private:
    QString fieldText() const;
    void    setFieldText( const QString& text );
};

//==================================================================================================
/// A CAF wrapper for DeckRecord
//==================================================================================================
class PdmDeckRecord : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    PdmDeckRecord();
    explicit PdmDeckRecord( Opm::DeckRecord& deckRecord );
    ~PdmDeckRecord() override = default;

    // Initialize from a DeckRecord
    void setFromDeckRecord( Opm::DeckRecord& deckRecord );

    // Fields for UI display
    caf::PdmField<size_t>                 m_itemCount;
    caf::PdmChildArrayField<PdmDeckItem*> m_items;

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
};

//==================================================================================================
/// A CAF wrapper for DeckKeyword
//==================================================================================================
class PdmDeckKeyword : public RimNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    PdmDeckKeyword();
    explicit PdmDeckKeyword( const Opm::DeckKeyword& keyword );
    ~PdmDeckKeyword() override = default;

    // Initialize from a DeckKeyword
    void setFromDeckKeyword( const Opm::DeckKeyword& keyword );

    // Fields for UI display
    caf::PdmField<QString>                  m_fileName;
    caf::PdmField<int>                      m_lineNumber;
    caf::PdmField<bool>                     m_isDataKeyword;
    caf::PdmField<size_t>                   m_recordCount;
    caf::PdmChildArrayField<PdmDeckRecord*> m_records;

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;
};

//==================================================================================================
/// A CAF wrapper for Deck
//==================================================================================================
class PdmDeck : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    PdmDeck();
    ~PdmDeck() override = default;

    // Initialize from a Deck
    void setFromDeck();

    // Fields for UI display
    caf::PdmField<QString>                   m_fileName;
    caf::PdmField<size_t>                    m_keywordCount;
    caf::PdmChildArrayField<PdmDeckKeyword*> m_keywords;
    caf::PdmField<std::vector<QString>>      m_activeKeywords;
    caf::PdmField<bool>                      m_loadData;

private:
    void loadDeck( const QString& fileName );

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

private:
    Opm::Deck m_deck;
};
