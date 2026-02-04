//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) Ceetron Solutions AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "gtest/gtest.h"

#include "cafPdmPythonGenerator.h"

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_SimpleCamel )
{
    EXPECT_STREQ( "my_variable", caf::PdmPythonGenerator::camelToSnakeCase( "MyVariable" ).toStdString().c_str() );
    EXPECT_STREQ( "some_field_name", caf::PdmPythonGenerator::camelToSnakeCase( "SomeFieldName" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_AlreadySnake )
{
    EXPECT_STREQ( "already_snake", caf::PdmPythonGenerator::camelToSnakeCase( "already_snake" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_SingleWord )
{
    EXPECT_STREQ( "hello", caf::PdmPythonGenerator::camelToSnakeCase( "hello" ).toStdString().c_str() );
    EXPECT_STREQ( "hello", caf::PdmPythonGenerator::camelToSnakeCase( "Hello" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_AcronymPrefix )
{
    // Consecutive uppercase letters followed by a capitalised word
    EXPECT_STREQ( "xml_parser", caf::PdmPythonGenerator::camelToSnakeCase( "XMLParser" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_NumberInName )
{
    // A digit followed by an uppercase letter should produce an underscore
    EXPECT_STREQ( "my_var2_name", caf::PdmPythonGenerator::camelToSnakeCase( "myVar2Name" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, CamelToSnakeCase_EmptyString )
{
    EXPECT_STREQ( "", caf::PdmPythonGenerator::camelToSnakeCase( "" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, PythonifyDataValue_BooleanLiterals )
{
    EXPECT_STREQ( "True", caf::PdmPythonGenerator::pythonifyDataValue( "true" ).toStdString().c_str() );
    EXPECT_STREQ( "False", caf::PdmPythonGenerator::pythonifyDataValue( "false" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, PythonifyDataValue_NoChange )
{
    EXPECT_STREQ( "hello", caf::PdmPythonGenerator::pythonifyDataValue( "hello" ).toStdString().c_str() );
    EXPECT_STREQ( "42", caf::PdmPythonGenerator::pythonifyDataValue( "42" ).toStdString().c_str() );
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
TEST( PdmPythonGenerator, PythonifyDataValue_MixedBooleans )
{
    EXPECT_STREQ( "True and False",
                  caf::PdmPythonGenerator::pythonifyDataValue( "true and false" ).toStdString().c_str() );
}
