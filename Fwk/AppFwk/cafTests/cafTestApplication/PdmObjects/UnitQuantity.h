#pragma once

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmFieldTraits.h"
#include "cafPdmUiFieldHandle.h"

#include <QString>
#include <QTextStream>
#include <QVariant>

#include <functional>

//==================================================================================================
/// Prototype of unit-safe quantity fields: the value is always stored in SI, and only converted when
/// shown in (or entered from) the user interface.
///
/// In ResInsight the conversion functions would be backed by Opm::UnitSystem / Opm::unit::*. The
/// constants below are the exact SI definitions used by opm-common (feet = 12 * inch, inch = 2.54 cm).
//==================================================================================================
namespace qty
{
enum class UnitSystem
{
    Metric,
    Field
};

enum class Quantity
{
    Length, // Metric: meter, Field: feet
    Diameter, // Metric: meter, Field: inch
    Pressure, // Metric: bar, Field: psi
    BuildAngle // Metric: deg/m, Field: deg/ft (curvature, SI: rad/m)
};

// Concrete display units. Auto means "use the default unit of the unit system", and a specific unit (for
// instance mm or in) lets a field choose independently of the unit system.
enum class Unit
{
    Auto,
    Meter,
    Millimeter,
    Feet,
    Inch,
    Bar,
    Psi,
    DegPerMeter,
    DegPerFeet
};

namespace conversion
{
    constexpr double meter       = 1.0;
    constexpr double millimeter  = 0.001 * meter;
    constexpr double inch        = 2.54 * 0.01 * meter;
    constexpr double feet        = 12.0 * inch;
    constexpr double pascal      = 1.0;
    constexpr double bar         = 1.0e5 * pascal;
    constexpr double psi         = 6894.757293168361 * pascal; // opm-common unit::psia
    constexpr double pi          = 3.14159265358979323846;
    constexpr double degree      = pi / 180.0; // radian
    constexpr double degPerMeter = degree / meter;
    constexpr double degPerFeet  = degree / feet;

    constexpr Unit defaultUnit( Quantity quantity, UnitSystem unitSystem )
    {
        const bool isMetric = unitSystem == UnitSystem::Metric;
        switch ( quantity )
        {
            case Quantity::Length:
                return isMetric ? Unit::Meter : Unit::Feet;
            case Quantity::Diameter:
                return isMetric ? Unit::Meter : Unit::Inch;
            case Quantity::Pressure:
                return isMetric ? Unit::Bar : Unit::Psi;
            default:
                return isMetric ? Unit::DegPerMeter : Unit::DegPerFeet;
        }
    }

    constexpr Unit resolve( Quantity quantity, UnitSystem unitSystem, Unit unit )
    {
        return unit == Unit::Auto ? defaultUnit( quantity, unitSystem ) : unit;
    }

    // Number of SI units per display unit
    constexpr double siPerUnit( Unit unit )
    {
        switch ( unit )
        {
            case Unit::Millimeter:
                return millimeter;
            case Unit::Feet:
                return feet;
            case Unit::Inch:
                return inch;
            case Unit::Bar:
                return bar;
            case Unit::Psi:
                return psi;
            case Unit::DegPerMeter:
                return degPerMeter;
            case Unit::DegPerFeet:
                return degPerFeet;
            default:
                return meter;
        }
    }

    inline QString unitLabel( Unit unit )
    {
        switch ( unit )
        {
            case Unit::Millimeter:
                return "mm";
            case Unit::Feet:
                return "ft";
            case Unit::Inch:
                return "in";
            case Unit::Bar:
                return "bar";
            case Unit::Psi:
                return "psi";
            case Unit::DegPerMeter:
                return "deg/m";
            case Unit::DegPerFeet:
                return "deg/ft";
            default:
                return "m";
        }
    }

    constexpr double toSi( Unit unit, double displayValue )
    {
        return displayValue * siPerUnit( unit );
    }
    constexpr double fromSi( Unit unit, double siValue )
    {
        return siValue / siPerUnit( unit );
    }

    constexpr double toSi( Quantity quantity, UnitSystem unitSystem, double displayValue )
    {
        return toSi( defaultUnit( quantity, unitSystem ), displayValue );
    }

    constexpr double fromSi( Quantity quantity, UnitSystem unitSystem, double siValue )
    {
        return fromSi( defaultUnit( quantity, unitSystem ), siValue );
    }

    inline QString unitLabel( Quantity quantity, UnitSystem unitSystem )
    {
        return unitLabel( defaultUnit( quantity, unitSystem ) );
    }
} // namespace conversion

//==================================================================================================
/// A value of a given quantity, stored in SI. There is deliberately no implicit conversion to double:
/// the caller must state which unit system is wanted.
//==================================================================================================
template <Quantity Q>
class QuantityValue
{
public:
    constexpr QuantityValue() = default;

    static constexpr QuantityValue fromSi( double siValue ) { return QuantityValue( siValue ); }
    static constexpr QuantityValue fromUnit( double value, UnitSystem unitSystem )
    {
        return QuantityValue( conversion::toSi( Q, unitSystem, value ) );
    }

    constexpr double si() const { return m_si; }
    constexpr double value( UnitSystem unitSystem ) const { return conversion::fromSi( Q, unitSystem, m_si ); }

    // Explicit unit, for instance Unit::Millimeter or Unit::Inch
    static constexpr QuantityValue fromUnit( double value, Unit unit )
    {
        return QuantityValue( conversion::toSi( conversion::resolve( Q, UnitSystem::Metric, unit ), value ) );
    }
    constexpr double value( Unit unit ) const
    {
        return conversion::fromSi( conversion::resolve( Q, UnitSystem::Metric, unit ), m_si );
    }

    constexpr bool operator==( const QuantityValue& other ) const { return m_si == other.m_si; }

private:
    explicit constexpr QuantityValue( double siValue )
        : m_si( siValue )
    {
    }

    double m_si = 0.0;
};

using Length     = QuantityValue<Quantity::Length>;
using Diameter   = QuantityValue<Quantity::Diameter>;
using Pressure   = QuantityValue<Quantity::Pressure>;
using BuildAngle = QuantityValue<Quantity::BuildAngle>;

//--------------------------------------------------------------------------------------------------
// caf field traits (found by ADL). The QVariant and the XML representation are the SI value.
//--------------------------------------------------------------------------------------------------
template <Quantity Q>
QVariant pdmToVariant( const QuantityValue<Q>& value )
{
    return QVariant( value.si() );
}

template <Quantity Q>
void pdmFromVariant( const QVariant& variant, QuantityValue<Q>& out )
{
    out = QuantityValue<Q>::fromSi( variant.toDouble() );
}

template <Quantity Q>
QTextStream& operator<<( QTextStream& str, const QuantityValue<Q>& value )
{
    str << value.si();
    return str;
}

template <Quantity Q>
QTextStream& operator>>( QTextStream& str, QuantityValue<Q>& value )
{
    double siValue = 0.0;
    str >> siValue;
    value = QuantityValue<Q>::fromSi( siValue );
    return str;
}

//--------------------------------------------------------------------------------------------------
// Field helpers
//--------------------------------------------------------------------------------------------------

// Makes the UI show and accept the value in the unit system returned by unitSystemProvider, while the
// field keeps storing SI. The provider is evaluated on every access, so the owner can change unit
// system without touching the stored values.
template <Quantity Q>
void setDisplayUnit( caf::PdmField<QuantityValue<Q>>* field, std::function<UnitSystem()> unitSystemProvider )
{
    auto toUi = [unitSystemProvider]( const QVariant& siValue )
    { return QVariant( conversion::fromSi( Q, unitSystemProvider(), siValue.toDouble() ) ); };

    auto fromUi = [unitSystemProvider]( const QVariant& displayValue )
    { return QVariant( conversion::toSi( Q, unitSystemProvider(), displayValue.toDouble() ) ); };

    field->uiCapability()->setUiValueConverters( toUi, fromUi );
}

// Variant where the owner states both a unit system and a per-field unit. The field unit wins unless it
// is Unit::Auto, in which case the default unit of the unit system is used.
template <Quantity Q>
void setDisplayUnit( caf::PdmField<QuantityValue<Q>>* field,
                     std::function<UnitSystem()>      unitSystemProvider,
                     std::function<Unit()>            unitProvider )
{
    auto resolved = [unitSystemProvider, unitProvider]()
    { return conversion::resolve( Q, unitSystemProvider(), unitProvider() ); };

    auto toUi = [resolved]( const QVariant& siValue )
    { return QVariant( conversion::fromSi( resolved(), siValue.toDouble() ) ); };

    auto fromUi = [resolved]( const QVariant& displayValue )
    { return QVariant( conversion::toSi( resolved(), displayValue.toDouble() ) ); };

    field->uiCapability()->setUiValueConverters( toUi, fromUi );
}

template <Quantity Q>
void setUnitLabel( caf::PdmField<QuantityValue<Q>>* field, const QString& name, UnitSystem unitSystem, Unit unit = Unit::Auto )
{
    const auto resolved = conversion::resolve( Q, unitSystem, unit );
    field->uiCapability()->setUiName( QString( "%1 [%2]" ).arg( name, conversion::unitLabel( resolved ) ) );
}
// Prescribes a display unit per unit system, so two fields of the same quantity can be shown in different
// units: for instance both as m in Metric, but one as ft and the other as in in Field.
struct UnitPerSystem
{
    Unit metric = Unit::Auto;
    Unit field  = Unit::Auto;

    Unit forSystem( UnitSystem unitSystem ) const { return unitSystem == UnitSystem::Metric ? metric : field; }
};

template <Quantity Q>
void setDisplayUnit( caf::PdmField<QuantityValue<Q>>* field,
                     std::function<UnitSystem()>      unitSystemProvider,
                     UnitPerSystem                    unitPerSystem )
{
    setDisplayUnit<Q>( field,
                       unitSystemProvider,
                       std::function<Unit()>( [unitSystemProvider, unitPerSystem]()
                                              { return unitPerSystem.forSystem( unitSystemProvider() ); } ) );
}

template <Quantity Q>
void setUnitLabel( caf::PdmField<QuantityValue<Q>>* field, const QString& name, UnitSystem unitSystem, UnitPerSystem unitPerSystem )
{
    setUnitLabel( field, name, unitSystem, unitPerSystem.forSystem( unitSystem ) );
}
// Display-unit presets found in the unit-converted UI fields of ResInsight (the hand-written
// "[m]" / "[ft]" label switches in defineUiOrdering). Each is {Metric unit, Field unit}.
namespace presets
{
    // Depths, lengths, spacing, roughness: RimPerforationInterval, RimSegmentInterval, RimMultipleValveLocations,
    // RimMswCompletionParameters, RimFishbones lateral length, RimFractureTemplate perforation length
    inline constexpr UnitPerSystem length{ Unit::Meter, Unit::Feet };

    // Well diameter and fracture width/ellipse width: RimFractureTemplate, RimFracture, RimEllipseFractureTemplate
    inline constexpr UnitPerSystem wellDiameter{ Unit::Meter, Unit::Inch };

    // Small diameters: RimFishbones tubing/lateral/ICD orifice, RimFishbonesPipeProperties hole, RimValveTemplate orifice
    inline constexpr UnitPerSystem smallDiameter{ Unit::Millimeter, Unit::Inch };

    // RimPressureTableItem: always bar today, a candidate for {Bar, Psi}
    inline constexpr UnitPerSystem pressure{ Unit::Bar, Unit::Psi };

    // RimFishbones build angle
    inline constexpr UnitPerSystem buildAngle{ Unit::DegPerMeter, Unit::DegPerFeet };
} // namespace presets
} // namespace qty

namespace caf
{
template <qty::Quantity Q>
struct PdmVariantEqualImpl<qty::QuantityValue<Q>>
{
    static bool equal( const QVariant& a, const QVariant& b ) { return PdmVariantEqualImpl<double>::equal( a, b ); }
};
} // namespace caf
