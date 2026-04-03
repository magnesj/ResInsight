#pragma once

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmPointer.h"

#include <QVariant>

#include <type_traits>
#include <utility>
#include <vector>

namespace caf
{

//==================================================================================================
/// Type traits used for dispatch in pdmVariantEqual
//==================================================================================================

template <typename T>
struct is_std_vector : std::false_type
{
};
template <typename T, typename A>
struct is_std_vector<std::vector<T, A>> : std::true_type
{
};

template <typename T>
struct is_std_pair : std::false_type
{
};
template <typename T, typename U>
struct is_std_pair<std::pair<T, U>> : std::true_type
{
};

template <typename T>
struct is_pdm_pointer : std::false_type
{
};
template <typename T>
struct is_pdm_pointer<PdmPointer<T>> : std::true_type
{
};

template <typename T>
struct is_app_enum : std::false_type
{
};
template <typename T>
struct is_app_enum<AppEnum<T>> : std::true_type
{
};

//==================================================================================================
/// pdmToVariant / pdmFromVariant
///
/// ADL-based customization points for converting field values to/from QVariant.
/// The default implementations work for any Qt-metatype-registered type.
/// Add overloads in the caf namespace to support additional types.
//==================================================================================================

template <typename T>
QVariant pdmToVariant( const T& value )
{
    return QVariant::fromValue( value );
}

template <typename T>
void pdmFromVariant( const QVariant& v, T& out )
{
    out = v.value<T>();
}

//==================================================================================================
/// std::vector overloads — element-wise delegation
//==================================================================================================

template <typename T>
QVariant pdmToVariant( const std::vector<T>& value )
{
    QList<QVariant> list;
    list.reserve( static_cast<int>( value.size() ) );
    for ( const auto& element : value )
    {
        list.push_back( pdmToVariant( element ) );
    }
    return list;
}

template <typename T>
void pdmFromVariant( const QVariant& v, std::vector<T>& out )
{
    if ( v.canConvert<QList<QVariant>>() )
    {
        out.clear();
        const QList<QVariant> list = v.toList();
        for ( const auto& item : list )
        {
            T element;
            pdmFromVariant( item, element );
            out.push_back( element );
        }
    }
}

//==================================================================================================
/// std::pair overloads
//==================================================================================================

template <typename T, typename U>
QVariant pdmToVariant( const std::pair<T, U>& value )
{
    QList<QVariant> list;
    list.push_back( pdmToVariant( value.first ) );
    list.push_back( pdmToVariant( value.second ) );
    return list;
}

template <typename T, typename U>
void pdmFromVariant( const QVariant& v, std::pair<T, U>& out )
{
    if ( v.canConvert<QList<QVariant>>() )
    {
        const QList<QVariant> list = v.toList();
        if ( list.size() == 2 )
        {
            pdmFromVariant( list[0], out.first );
            pdmFromVariant( list[1], out.second );
        }
    }
}

//==================================================================================================
/// PdmPointer overloads
/// Stores as PdmPointer<PdmObjectHandle> to avoid Q_DECLARE_METATYPE for each pointed-to type.
//==================================================================================================

template <typename T>
QVariant pdmToVariant( const PdmPointer<T>& value )
{
    return QVariant::fromValue( PdmPointer<PdmObjectHandle>( value.rawPtr() ) );
}

template <typename T>
void pdmFromVariant( const QVariant& v, PdmPointer<T>& out )
{
    out.setRawPtr( v.value<PdmPointer<PdmObjectHandle>>().rawPtr() );
}

//==================================================================================================
/// AppEnum overloads
/// Stores as int to support enum class without Q_DECLARE_METATYPE.
//==================================================================================================

template <typename T>
QVariant pdmToVariant( const AppEnum<T>& value )
{
    return QVariant( static_cast<int>( static_cast<T>( value ) ) );
}

template <typename T>
void pdmFromVariant( const QVariant& v, AppEnum<T>& out )
{
    out = static_cast<T>( v.toInt() );
}

//==================================================================================================
/// FilePath overloads — stores the path string directly in the QVariant
//==================================================================================================

inline QVariant pdmToVariant( const FilePath& value )
{
    return QVariant( value.path() );
}

inline void pdmFromVariant( const QVariant& v, FilePath& out )
{
    out.setPath( v.toString() );
}

//==================================================================================================
/// pdmVariantEqual<T>
///
/// Compares two QVariants that carry a field value of type T.
/// Uses if constexpr to handle container and pointer types.
/// Full specializations below handle FilePath, float, and double.
/// Additional full specializations (e.g. for cvf types) can be provided in type-specific headers
/// after including this header.
//==================================================================================================

template <typename T>
bool pdmVariantEqual( const QVariant& a, const QVariant& b )
{
    if constexpr ( is_pdm_pointer<T>::value )
    {
        return a.value<PdmPointer<PdmObjectHandle>>() == b.value<PdmPointer<PdmObjectHandle>>();
    }
    else if constexpr ( is_std_vector<T>::value )
    {
        using ElementType = typename T::value_type;

        const QList<QVariant> la = a.toList();
        const QList<QVariant> lb = b.toList();
        if ( la.size() != lb.size() ) return false;

        for ( int i = 0; i < la.size(); ++i )
        {
            if ( !pdmVariantEqual<ElementType>( la[i], lb[i] ) ) return false;
        }
        return true;
    }
    else if constexpr ( is_std_pair<T>::value )
    {
        const QList<QVariant> la = a.toList();
        const QList<QVariant> lb = b.toList();
        if ( la.size() != 2 || lb.size() != 2 ) return false;

        return pdmVariantEqual<typename T::first_type>( la[0], lb[0] ) &&
               pdmVariantEqual<typename T::second_type>( la[1], lb[1] );
    }
    else if constexpr ( is_app_enum<T>::value )
    {
        return a.toInt() == b.toInt();
    }
    else
    {
        return a.value<T>() == b.value<T>();
    }
}

template <>
inline bool pdmVariantEqual<FilePath>( const QVariant& a, const QVariant& b )
{
    return a.toString() == b.toString();
}

template <>
inline bool pdmVariantEqual<float>( const QVariant& a, const QVariant& b )
{
    // See PdmFieldWriter::writeFieldData for the precision used when writing float values
    const float epsilon = 1e-6f;
    return qAbs( a.value<float>() - b.value<float>() ) < epsilon;
}

template <>
inline bool pdmVariantEqual<double>( const QVariant& a, const QVariant& b )
{
    // See PdmFieldWriter::writeFieldData for the precision used when writing double values
    const double epsilon = 1e-8;
    return qAbs( a.value<double>() - b.value<double>() ) < epsilon;
}

} // namespace caf
