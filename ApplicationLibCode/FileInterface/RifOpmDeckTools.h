/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025 Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <initializer_list>
#include <optional>
#include <string>
#include <variant>

namespace Opm
{
class DeckItem;
class DeckRecord;
} // namespace Opm

namespace RifOpmDeckTools
{
Opm::DeckItem item( std::string name, std::string value );
Opm::DeckItem optionalItem( std::string name, std::optional<std::string> value );
Opm::DeckItem item( std::string name, int value );
Opm::DeckItem optionalItem( std::string name, std::optional<int> value );
Opm::DeckItem item( std::string name, size_t value );
Opm::DeckItem optionalItem( std::string name, std::optional<size_t> value );
Opm::DeckItem item( std::string name, double value );
Opm::DeckItem optionalItem( std::string name, std::optional<float> value );
Opm::DeckItem optionalItem( std::string name, std::optional<double> value );
Opm::DeckItem defaultItem( std::string name );

struct NamedValue
{
    std::string                            name;
    std::variant<std::string, int, double> value;

    NamedValue( std::string n, std::string v ) : name( std::move( n ) ), value( std::move( v ) ) {}
    NamedValue( std::string n, const char* v ) : name( std::move( n ) ), value( std::string( v ) ) {}
    NamedValue( std::string n, int v ) : name( std::move( n ) ), value( v ) {}
    NamedValue( std::string n, size_t v ) : name( std::move( n ) ), value( (int)v ) {}
    NamedValue( std::string n, double v ) : name( std::move( n ) ), value( v ) {}
    NamedValue( std::string n, float v ) : name( std::move( n ) ), value( (double)v ) {}
};

Opm::DeckRecord createRecord( std::initializer_list<NamedValue> items );

} // namespace RifOpmDeckTools
