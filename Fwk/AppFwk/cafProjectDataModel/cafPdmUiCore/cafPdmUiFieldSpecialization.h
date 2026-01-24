#pragma once

#include <QList>
#include <QVariant>

#include <vector>

namespace caf
{
template <typename T>
class PdmDataValueField;
class PdmOptionItemInfo;
class PdmObjectHandle;
class PdmFieldHandle;

//==================================================================================================
/// Base class providing default implementations for PdmUiFieldSpecialization methods.
/// Specializations can inherit from this to avoid repeating empty implementations.
//==================================================================================================
struct PdmUiFieldSpecializationDefaults
{
    template <typename T>
    static QList<PdmOptionItemInfo> valueOptions( PdmFieldHandle*, const T& )
    {
        return QList<PdmOptionItemInfo>();
    }

    template <typename T>
    static void childObjects( const PdmDataValueField<T>&, std::vector<PdmObjectHandle*>* )
    {
    }
};

//==================================================================================================
/// A proxy class that implements the Gui interface of fields
///
/// This class collects methods that need specialization when introducing a new type in a PdmField.
/// Having those methods in a separate class makes it possible to "partially specialize" the methods
/// for container classes etc. since partial specialization of template functions is not C++ as of yet.
///
/// When introducing a new type in a PdmField, you might need to implement a (partial)specialization
/// of this class.
//==================================================================================================

template <typename T>
class PdmUiFieldSpecialization : public PdmUiFieldSpecializationDefaults
{
public:
    /// Convert the field value into a QVariant
    static QVariant convert( const T& value ) { return QVariant::fromValue( value ); }

    /// Set the field value from a QVariant
    static void setFromVariant( const QVariant& variantValue, T& value ) { value = variantValue.value<T>(); }

    /// Check equality between QVariants that carries a Field Value.
    /// The == operator will normally work, but does not support custom types in the QVariant
    /// See http://qt-project.org/doc/qt-4.8/qvariant.html#operator-eq-eq-64
    /// This is needed for the lookup regarding OptionValues
    static bool isDataElementEqual( const QVariant& variantValue, const QVariant& variantValue2 )
    {
        if ( variantValue.typeId() > QMetaType::User )
        {
            return ( variantValue.value<T>() == variantValue2.value<T>() );
        }
        else
        {
            return variantValue == variantValue2;
        }
    }

    // valueOptions and childObjects are inherited from PdmUiFieldSpecializationDefaults
};
} // End of namespace caf

#include "cafInternalPdmFieldTypeSpecializations.h"
