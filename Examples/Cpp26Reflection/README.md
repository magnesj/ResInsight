# C++26 Reflection Example

This directory contains a demonstration of the C++26 reflection features proposed in [P2996R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r7.html).

## Overview

The reflection example (`reflection_example.cpp`) demonstrates several key features of C++26 reflection:

1. **Member Introspection**: Examining struct members at compile-time
2. **Member Iteration**: Iterating over all non-static data members
3. **Type Information**: Getting member names and types
4. **Reflection-based Operations**: Generic functions that work with any struct

## Features Demonstrated

### 1. Print Members
Generic `printMembers()` function that prints all members of any struct with appropriate formatting based on type.

### 2. Member Count
Compile-time function to count the number of members in a struct.

### 3. Member Existence Check
Compile-time function to check if a struct has a specific member by name.

### 4. Reflection-based Serialization
JSON serialization function that automatically serializes any struct using reflection, without manual field enumeration.

### 5. Member Name Listing
Compile-time listing of all member names.

## Key C++26 Reflection Features Used

- `^` operator: Obtains reflection of a type or expression
- `std::meta::name_of()`: Gets the name of a reflected entity
- `std::meta::type_of()`: Gets the type of a reflected member
- `std::meta::nonstatic_data_members_of()`: Gets all non-static data members
- `[:reflection:]`: Splicer syntax to use reflection results in code

## Compiler Support

As of November 2024, C++26 reflection is still experimental and requires:

### Clang (Experimental)
- Clang 18+ with experimental reflection support
- Compile flags: `-std=c++2c -freflection`

### EDG-based Compilers
Some EDG-based compilers may have experimental support.

### GCC and MSVC
Full support not yet available (as of GCC 14 and MSVC 17.11).

## Building

**Note**: This example requires a compiler with C++26 reflection support, which is currently very limited.

### With CMake

If your compiler supports C++26 reflection:

```bash
mkdir build && cd build
cmake .. -DCMAKE_CXX_STANDARD=26
cmake --build . --target reflection_example
./reflection_example
```

### Manual Compilation

With a compatible compiler:

```bash
clang++ -std=c++2c -freflection reflection_example.cpp -o reflection_example
./reflection_example
```

## Expected Output

```
=== C++26 Reflection Example ===

Example 1: Print struct members
Members of Point:
  x: 10.50
  y: 20.30
  z: 30.70
  label: "Surface Point"

Example 2: Member count
Point has 4 members
WellData has 4 members

Example 3: Check for specific members
Point has 'x' member: true
Point has 'name' member: false
WellData has 'depth' member: true

Example 4: Reflection-based JSON serialization
WellData:
Members of WellData:
  name: "Well-A"
  depth: 3500.00
  pressure: 5200.00
  isActive: true

JSON representation:
{
  "name": "Well-A",
  "depth": 3500,
  "pressure": 5200,
  "isActive": true
}

Example 5: List all member names of Point
  - x
  - y
  - z
  - label
```

## Practical Applications

Reflection enables many powerful patterns without code generation:

1. **Automatic Serialization**: JSON, XML, binary formats without manual field listing
2. **Object Inspection**: Debug printing, logging, validation
3. **Generic Algorithms**: Operations that work on any struct automatically
4. **ORM/Database Mapping**: Automatic SQL generation and object mapping
5. **UI Generation**: Automatic form generation from data structures
6. **Testing**: Automatic test data generation and comparison

## References

- [P2996R7 - Reflection for C++26](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2996r7.html)
- [C++ Reflection TS](https://en.cppreference.com/w/cpp/experimental/reflect)
- [CppCon Talks on Reflection](https://www.youtube.com/results?search_query=cppcon+reflection)

## ResInsight Context

While ResInsight currently uses C++23, this example demonstrates future capabilities that could simplify:

- PDM (Project Data Model) object serialization
- GRPC interface generation
- Debug visualization
- Automated testing of data structures

As compiler support matures, reflection could reduce boilerplate code in ResInsight's data model infrastructure.
