# PDM initialization with functions: baseline, prototypes and selected design

Status: result of steps 1.0, 1.2 and 1.3 of issue #1202. The framework and the application are not changed. All code
is in `Fwk/AppFwk/cafTests/cafPdmInitPrototype`, and in new unit tests.

The goal of the issue is to replace the `CAF_PDM_Init*` macros with functions, without changing XML files, generated
Python code, UI behavior or build time.

## Summary

| Question | Answer |
|---|---|
| Compile-time keyword check without macros | Keyword types with `consteval` constructors. Works on MSVC, GCC and Clang. |
| Parameterless scripting with compile-time check | Separate function `initScriptableField()`. A builder call `.withScripting()` can only be checked at compile time if the keyword is a template argument. |
| Static `PdmUiItemInfo` without a static variable at the call site | One shared object for each distinct set of string literals, found by pointer identity. |
| Static storage per call site with a lambda as template argument | Rejected. Clang shares the storage between translation units. |
| Build time | 24 % lower for plain fields, 43 % lower for scriptable fields, same compiler memory or lower. No cost for including the header. |
| Construction time | 7-16 % lower for plain fields, 58 % lower for scriptable fields. |
| Memory | 6 % lower. |
| Functional parity | Field order, keywords, owner class, aliases, UI text, defaults, XML, scripting names and generated Python are identical in the parity tests. |

Selected design: [Selected design](#selected-design). Things that are not verified: [Not verified](#not-verified).

## Environment

All numbers in this document are measured on one machine:

| | |
|---|---|
| CPU | Intel Core i9-13900F, 32 logical processors |
| Compiler | MSVC 19.52.37029, `/O2 /Ob1 /Z7 -std:c++latest -permissive-` (RelWithDebInfo) |
| Qt | 6.6.3 |
| Build | Ninja, no precompiled headers, no unity build |
| Source | `dev` at `f2e3a182ae` |

Compile time is measured by `measure_compile_time.py`. It reads the compile command from the Ninja build directory and
runs the compiler directly, one process at a time, so the build cache and other build activity do not affect the
result.

## Step 1.0: compatibility and performance baseline

### Tests for keyword aliases

Aliases had no test coverage. New tests describe the current behavior, and must pass unchanged after the migration.

`cafPdmXml_UnitTests`, `KeywordAliasTest`:

- The class keyword is the first keyword. `matchesClassKeyword()` accepts aliases. Aliases of a base class are not
  inherited.
- The factory creates the object from the keyword and from every alias, and lists all of them as class keywords.
- A field is found by alias, also when the field is declared in a base class.
- An object is read from XML written with class and field aliases, and is written with the canonical keywords.
- If the alias and the canonical keyword are both in the file, the last one in the file is used.
- A file with old class keywords, old field keywords and old path references in pointer fields gives the same objects,
  and is written back identical to the canonical file.
- `PdmReferenceHelper` resolves paths containing aliases, and writes canonical keywords.
- Collisions are not rejected. A canonical keyword is found before an alias. For equal aliases, the field registered
  first is found.

`cafPdmScripting_UnitTests`, `PdmPythonGenerator.KeywordAliasesAreNotVisibleInGeneratedCode`: aliases do not appear in
generated Python code.

Not covered: registration of two classes with the same keyword or alias. The factory stops on `CAF_ASSERT`, which
cannot be tested in the unit test framework used by AppFwk.

### Generated Python and project files

`capture_compatibility_baseline.py` captures:

1. `generated_classes.py` from `ResInsight --console --generate`.
2. Five tracked project files, opened and saved by ResInsight. Each saved file is opened and saved again, and the two
   files must be identical.

Usage, before and after a change:

```
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/capture_compatibility_baseline.py capture --resinsight build/ResInsight.exe --output baseline
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/capture_compatibility_baseline.py capture --resinsight build/ResInsight.exe --output candidate
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/capture_compatibility_baseline.py compare baseline candidate
```

Result for this branch, built from `dev` at `f2e3a182ae` plus the test and prototype code:

- Two captures from the same executable are identical for all six files, and no project file changes when it is saved
  a second time. The capture is usable as a gate.
- SHA-256 of `generated_classes.py`: `af0e753b26a0a86367a03ae0f428db7789bb8517c3f02478dd7164b67d80691a`

The file name of the project, and date fields that get the current time as default value, are removed from the saved
project files before the comparison. Other file paths are kept, so checksums of project files are only comparable
between captures made from the same checkout folder. The captured files are not committed.

### Build time baseline

Time to compile one file, median of several runs:

| File | Time (ms) | Peak compiler memory (MB) |
|---|---:|---:|
| `cafPdm_CompileBench/bench_mixed_fields.cpp` (20 classes, 10 fields each) | 4755 | 349 |
| `bench_scriptable_macro.cpp` (20 classes, 10 fields each, 9 scriptable) | 4408 | 345 |
| `bench_include_macro.cpp` (includes `cafPdmObject.h` and `cafPdmField.h`, no classes) | 1601 | 263 |
| `RimWellPath.cpp` | 5378 | 447 |
| `RimEclipseCase.cpp` | 5718 | 487 |
| `RimSummaryCurve.cpp` | 5158 | 504 |
| `RimEclipseView.cpp` | 9172 | 621 |

Incremental build after a change in a leaf model class: one of the last four rows, plus link time.

Incremental build after a change in a common PDM header: 1795 translation units have a recorded dependency on
`cafPdmObject.h`. Two random samples of 40 and 60 of these files have a mean compile time of 2.6 and 2.8 seconds. The
estimated compile time for all of them is 78 to 83 CPU minutes, or about 4 minutes of wall-clock time with 20 parallel
jobs, before linking.

```
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/measure_compile_time.py --build-dir build --runs 1 --depends-on cafPdmObject.h --sample 40
```

### Runtime baseline

Construction of 200 000 objects with ten fields, initialized with the macros:

| Object | Construction (ns per object) | Memory (bytes per object) |
|---|---:|---:|
| Ten fields, not scriptable | 9 300 - 10 400 | 13 368 |
| Ten scriptable fields | 26 000 - 26 500 | 14 088 |

Scriptable fields are slow because the macro builds the tool tip text ("Available through python/rips as the
attribute ...") for every field of every object, also when the static `PdmUiItemInfo` already exists.

### Macro call sites

4565 field macro call sites in the repository, not counting third party code:

| | Call sites |
|---|---:|
| No optional arguments (icon, tool tip, "What's this") | 3917 |
| One, two or three optional arguments | 646 |
| UI name is not a string literal | 10 |
| An optional argument is not a string literal | 12 |
| The field argument is not `&member` | 56 |

## Step 1.3: compile-time keyword validation

The macros use `static_assert` on the keyword literal. A function parameter of type `const QString&` cannot be
checked at compile time. Two candidates were prototyped.

**Candidate A: keyword types with `consteval` constructors** (`cafProtoKeyword.h`)

```cpp
class PdmKeyword
{
public:
    consteval PdmKeyword( const char* text );   // same rules as CAF_PDM_VERIFY_XML_KEYWORD
};

class PdmScriptKeyword        // same rules as CAF_PDM_CheckScriptableKeyword
class PdmLegacyScriptKeyword  // allows a lowercase first letter
```

```cpp
initField( m_name, "Name", "Name" );
initScriptableField( m_radius, "Radius", "Radius" );
initField( m_legacy, "legacy_keyword", "Legacy" ).withScriptKeyword( "LegacyKeyword" );
```

The constructor calls a function that is not `constexpr` when a rule is broken. The compiler reports the name of that
function, for example `keywordContainsInvalidCharacter`.

**Candidate B: keyword as a fixed string template argument** (`cafProtoFixedString.h`)

```cpp
initFieldN<"Name">( m_name, "Name" );
initFieldN<"Radius">( m_radius, "Radius" ).withScripting();
```

The keyword is part of the builder type, so `.withScripting()` can use `static_assert`.

### Result

`compile_fail/run_keyword_compile_tests.py` compiles a model without Qt and PDM dependencies. It has valid uses that
must compile and run, and 20 invalid uses that must be rejected.

| Compiler | Valid uses | Invalid uses rejected |
|---|---|---|
| MSVC 19.52 | OK | 20 of 20 |
| GCC 14.2 | OK | 20 of 20 |
| GCC 15.2 | OK | 20 of 20 |
| Clang 19.1.7 (Linux) | OK | 20 of 20 |

| | Candidate A | Candidate B |
|---|---|---|
| XML keyword check | Yes | Yes |
| Scripting keyword check | Yes, in `initScriptableField()` and `withScriptKeyword()` | Yes, in `withScripting()` |
| Parameterless `.withScripting()` after `initField()` | No. The keyword value is not part of any type. | Yes |
| Legacy lowercase scripting keywords | A class hides `initScriptableField()` with an overload taking `PdmLegacyScriptKeyword` | A class hides `initFieldN()` |
| Keyword that is not a constant | Compile error | Compile error |
| Error location | The line of the call | The `static_assert` in the framework header. The call is in a note. |
| Error text | "call to consteval function is not a constant expression", the reason is in a note | The text of the `static_assert` |
| Syntax in a class template | Unchanged | Requires `this->template initFieldN<"Name">( ... )` |
| Compile time, plain fields | 0.76 of macros | 0.80 of macros |
| Compile time, scriptable fields | 0.57 of macros | 0.59 of macros |
| Compiler memory | 1.00 and 0.83 of macros | 1.03 and 0.85 of macros |

**Selected: candidate A.** It is the simplest, it is slightly cheaper to compile, the error is reported on the line
with the mistake, and the syntax is ordinary function calls. The price is that scripting is enabled by the function
name and not by a builder call.

The replacement for the `allowLowercaseScriptKeywords` class member is name hiding. `RicfCommandObject` declares
`initScriptableField()` overloads taking `PdmLegacyScriptKeyword`. They hide the functions in `caf::PdmObject` for
`RicfCommandObject` and all classes derived from it, which is the same set of classes as today.

A camel case keyword starting with an uppercase letter has only letters and digits, so `PdmScriptKeyword` is always a
valid XML keyword. `PdmLegacyScriptKeyword` checks the XML rules too.

## Step 1.2: metadata storage

The macros declare `static caf::PdmUiItemInfo objDescr( ... )` at the call site. Name, icon, tool tip and "What's this"
are stored once for each field of each class. A function needs another way to share this object.

| Candidate | Result |
|---|---|
| Static variable in a function template, one instantiation per call site | **Rejected, not portable** |
| One `PdmUiItemInfo` for each field instance | Rejected: 21-29 % more memory, 11-33 % slower |
| Registry with class keyword and field keyword as key | Works. 4-6 % slower than interned. |
| **Interned**: one `PdmUiItemInfo` for each distinct set of string literals | **Selected** |

### Rejected: static storage per call site

A function template gets its own static variable for every instantiation. If every call uses a distinct template
argument, every call site gets its own storage:

```cpp
template <typename FieldType, typename Tag = decltype( [] {} )>
PdmFieldInit<FieldType> initField( FieldType& field, PdmKeyword keyword )
{
    static PdmUiItemInfo info;   // one for each call site?
```

`unique_slot/run_unique_slot_test.py` tests this with two translation units:

| Compiler | Result |
|---|---|
| MSVC 19.52 | 8 distinct objects for 8 call sites |
| GCC 14.2, GCC 15.2 | 8 distinct objects for 8 call sites |
| Clang 19.1.7 | **4 distinct objects for 8 call sites** |

With Clang, call number N in one translation unit shares storage with call number N in another translation unit.
Fields in unrelated classes would silently get the UI text of each other. The design is not used.

### Selected: interned information

The static information is immutable. `PdmUiItem` only reads it, and changes for one instance are stored in the item
itself (`m_configItemInfos`). Two fields with the same keyword, name, icon, tool tip and "What's this" can therefore
share one `PdmUiItemInfo`, also when they belong to different classes.

Text is passed as `PdmStaticText`, a type with a `consteval` constructor:

```cpp
class PdmStaticText
{
public:
    consteval PdmStaticText( const char* text );
```

Only a constant expression can be converted to `PdmStaticText`, so the pointer always refers to text with static
storage duration and constant content. Equal pointers mean equal text. The key of a `PdmUiItemInfo` is the six
pointers (UI name, icon, tool tip, "What's this", keyword, scripting keyword). No text is hashed or compared.

Consequences:

- No `QString` is created for UI text when an object is constructed, except the first time. With the macros, a
  `std::vector<QString>` and three `QString` objects are created for every field of every object.
- The tool tip for scriptable fields is built once.
- Identical text in different places can have different pointers. This gives two equal `PdmUiItemInfo` objects, which
  is harmless.
- Text computed at runtime cannot use this path. `withUiName( const QString& )` and `withToolTip( const QString& )`
  store the text in the builder, and the information is found by content. 22 of 4565 call sites need this.

Lookup is done in a small cache owned by the thread, without a lock. The shared map is protected by a
`std::shared_mutex`, and is only used the first time a thread sees a key.

### Parity tests

`cafPdmInitPrototype_UnitTests` has two versions of the same classes, initialized with macros and with functions. The
tests compare:

- Number of fields, order, keywords, keyword aliases, owner object and owner class, including a field declared in a
  base class.
- Capabilities: UI, XML and scripting, and that `configureCapabilities()` is called.
- UI name, tool tip, "What's this" and icon for the object and for all fields, including the text added to the tool
  tip of scriptable fields.
- Values and default values, including an enum value used as default for an `AppEnum` field.
- XML text, reading a file with class and field aliases, pointer field references, and copy by XML serialization.
- Script class name and comment, scripting keywords, and the generated Python class.
- The static information is shared between instances, and a change of UI name for one instance does not affect
  another instance.
- Text computed at runtime, and the fixed string alternative.

### Build time

Median of 7 runs. The function versions use keywords that are unique for each class, as in application code.

| File | Time (ms) | Ratio | Peak memory (MB) | Ratio |
|---|---:|---:|---:|---:|
| Plain fields, macros | 4755 | 1.00 | 349 | 1.00 |
| Plain fields, functions with `.withDefault()` | 3605 | 0.76 | 349 | 1.00 |
| Plain fields, functions with positional default value | 3633 | 0.76 | 349 | 1.00 |
| Plain fields, fixed string keyword | 3794 | 0.80 | 358 | 1.03 |
| Scriptable fields, macros | 4408 | 1.00 | 345 | 1.00 |
| Scriptable fields, functions | 2499 | 0.57 | 286 | 0.83 |
| Scriptable fields, fixed string keyword | 2622 | 0.59 | 293 | 0.85 |
| Include only, `cafPdmObject.h` | 1601 | 1.00 | 263 | 1.00 |
| Include only, with `cafProtoFieldInit.h` | 1606 | 1.00 | 263 | 1.00 |

The functions are cheaper to compile than the macros. Each macro expands to a block with a `std::vector<QString>`,
three `QString` variables, three `static_assert` with recursive `constexpr` functions, and a static variable with a
thread safe guard. The function call is a call to a small template, and the body is compiled once in
`cafProtoFieldInit.cpp`.

The header adds no measurable cost for translation units that include `cafPdmObject.h`, so an incremental build after
a change in a common PDM header is not affected.

```
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/measure_compile_time.py --build-dir build --runs 7 --baseline bench_mixed_fields.cpp bench_mixed
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/measure_compile_time.py --build-dir build --runs 7 --baseline bench_scriptable_macro bench_scriptable
python Fwk/AppFwk/cafTests/cafPdmInitPrototype/measure_compile_time.py --build-dir build --runs 7 --baseline bench_include_macro bench_include
```

### Construction time and memory

200 000 objects with ten fields, best of five repetitions. Memory is the number of bytes allocated from the heap.
`sizeof( caf::PdmUiItemInfo )` is 272 bytes.

| Variant | Construction (ns per object) | Ratio | Memory (bytes per object) | Ratio |
|---|---:|---:|---:|---:|
| Plain fields, macros | 9 310 | 1.00 | 13 368 | 1.00 |
| Plain fields, functions, interned | 8 698 | 0.93 | 12 518 | 0.94 |
| Plain fields, functions, registry per class | 9 021 | 0.97 | 12 518 | 0.94 |
| Plain fields, functions, information per field | 12 351 | 1.33 | 16 235 | 1.21 |
| Scriptable fields, macros | 26 026 | 1.00 | 14 088 | 1.00 |
| Scriptable fields, functions, interned | 10 843 | 0.42 | 13 216 | 0.94 |
| Scriptable fields, functions, registry per class | 11 461 | 0.44 | 13 216 | 0.94 |
| Scriptable fields, functions, information per field | 28 950 | 1.11 | 18 184 | 1.29 |

A second run gave 0.84 for plain fields, interned. The other ratios were within 0.02.

Two details in the implementation matter for these numbers. Without them, the function version was 13 % slower than
the macros for plain fields:

- The keyword is converted to `QString` once for each thread and literal. Later fields get a shared copy. This is also
  the reason for the lower memory use.
- `classKeyword()` creates a `std::vector<QString>` on every call, and the macros call it for every field. The
  function caches the keyword of the class being constructed, using `typeid( *this )` as key.

```
cafPdmInitPrototype_UnitTests --gtest_also_run_disabled_tests --gtest_filter=*ProtoBenchmark*
```

## Selected design

```cpp
initObject( "Well Path" ).withIcon( ":/Well.svg" ).withScripting( "WellPath", "A well path" );

initField( m_name, "Name", "Name" );                                   // no default value
initField( m_show, "Show", true, "Show" ).withToolTip( "..." );        // default value
initField( m_count, "Count", "Count" ).withDefault( 3 );               // same result as the line above
initScriptableField( m_radius, "Radius", 0.1, "Radius" );              // "Radius" is XML and scripting keyword
initField( m_legacy, "legacy_keyword", 0, "Legacy" ).withScriptKeyword( "LegacyKeyword" );
initField( m_old, "Value", "Value" ).withKeywordAlias( "OldValue" );
initField( m_dynamic, "Dynamic", "" ).withUiName( computedName );      // text computed at runtime
```

| Macro | Function |
|---|---|
| `CAF_PDM_InitObject( name, icon, toolTip, whatsThis )` | `initObject( name ).withIcon().withToolTip().withWhatsThis()` |
| `CAF_PDM_InitScriptableObject( ... )` | `initObject( ... ).withScripting()` |
| `CAF_PDM_InitScriptableObjectWithNameAndComment( ..., scriptName, comment )` | `initObject( ... ).withScripting( scriptName, comment )` |
| `CAF_PDM_InitField( &f, kw, default, name, icon, toolTip, whatsThis )` | `initField( f, kw, default, name ).withIcon().withToolTip().withWhatsThis()` |
| `CAF_PDM_InitFieldNoDefault( &f, kw, name )` | `initField( f, kw, name )` |
| `CAF_PDM_InitScriptableField( &f, kw, default, name )` | `initScriptableField( f, kw, default, name )` |
| `CAF_PDM_InitScriptableFieldNoDefault( &f, kw, name )` | `initScriptableField( f, kw, name )` |
| `CAF_PDM_InitScriptableFieldWithScriptKeyword( &f, kw, scriptKw, default, name )` | `initField( f, kw, default, name ).withScriptKeyword( scriptKw )` |
| `CAF_PDM_InitScriptableFieldWithScriptKeywordNoDefault( &f, kw, scriptKw, name )` | `initField( f, kw, name ).withScriptKeyword( scriptKw )` |
| `field.registerKeywordAlias( alias )` | Unchanged, or `.withKeywordAlias( alias )` |

Decisions:

1. **UI name is a positional argument.** All call sites have it. The call is shorter, and the argument order is the
   same as in the macros, which makes the migration mechanical.
2. **The default value can be positional or `.withDefault()`.** Compile time is equal. The positional form keeps the
   argument order of the macros. It accepts values that are convertible to the field type, the macros require the
   exact type.
3. **Six field macros become two functions and one builder call.** `initScriptableField()` is needed for the
   compile-time check, see step 1.3.
4. **The field is passed by reference**, not by pointer.
5. **Layers.** `initField()` and the builder are in the object header. They are templates on the field type, and
   delegate to code in the cpp file. Scripting is reached through `PdmFieldScriptingHook<FieldType>` and
   `PdmObjectScriptingHook`, which are declared in the object header and defined in the scripting headers. A
   translation unit that uses `initScriptableField()` without including the scripting header gets a compile error
   about an incomplete type. No scripting header is included by the object header.
6. **Finalization.** The builder is a temporary object. Its destructor assigns the static information at the end of
   the statement. The order of the calls in a chain does not matter. Capabilities, the keyword, the owner class and
   the default value are set immediately, in the same order as in the macros, because a field must be registered
   before it gets a value.
7. **Owner class.** The functions call the virtual `classKeyword()`. In a constructor, a virtual call is dispatched to
   the class being constructed, so the result is the declaring class, as with the macros.
8. **Presence check.** The macros call `Error_You_forgot_to_add_the_macro_...()`. The check is not needed: a class
   with `CAF_PDM_HEADER_INIT` and without `CAF_PDM_SOURCE_INIT` fails to link because `classKeyword()` is missing.
   According to the macro definition, a class without `CAF_PDM_HEADER_INIT` is not detected today either, because the
   name is found in the base class. This is not tested.
9. **Aliases** are unchanged. `withKeywordAlias()` calls `registerKeywordAlias()`. The alias is checked as XML
   keyword, not as scripting keyword. Collision behavior is unchanged, see the baseline tests.

Requirements for the implementation in the framework (step 1.1):

- The registry of static information must never be destroyed. Objects can be destroyed during static destruction, and
  they hold pointers into it.
- `withIcon()` and `withWhatsThis()` need overloads for text computed at runtime, as `withUiName()` and
  `withToolTip()` have in the prototype.
- The prototype registers the script class on every construction, as the macros do. This can be done once per class.

## Not verified

- **GCC 13**, the minimum version in `docs/agents/build.md`. GCC 14.2 and 15.2 are tested. `consteval` is supported
  from GCC 10, so no problem is expected, but it has not been compiled.
- **Clang on Windows and macOS.** Clang 19.1 from the local LLVM installation does not work with the standard library
  of the installed Visual Studio, which requires Clang 22. Clang is tested on Linux.
- **The PDM prototype on Linux.** Only the keyword model and the static storage test are compiled with GCC and Clang.
  The prototype library, parity tests and benchmarks are built with MSVC only.
- **Wall-clock time of a full rebuild** after a change in a common header. The number in this document is an estimate
  from samples. The existing build directory for performance measurements (`build-perf`) refers to a compiler version
  that is no longer installed, and was not reconfigured.
- **Thread sanitizer.** Lookup of static information from several threads is protected by design, but there is no test
  constructing objects in parallel.
- **The visual comparison in `cafTestApplication`.** The objects "Init With Macros" and "Init With Functions" are
  added and the application builds. It has not been started.

## Next steps

1. Review of the selected design, in particular decision 3 (`initScriptableField()`), and interning by pointer
   identity.
2. Step 1.1: move the functions to `caf::PdmObject`, the keyword types to `cafPdmCore`, and the hooks to the scripting
   headers. Move the parity tests to the AppFwk unit tests. Build on Linux in CI.
3. Step 1.4: implement the macros with the functions. Run `capture_compatibility_baseline.py` before and after, and
   the build time measurements above. A macro can forward string literals to the functions. The optional positional
   arguments, and the 22 call sites with text computed at runtime, need a design that is not covered by the prototype.
   The cost of the macros implemented this way must be measured.
4. Step 1.5: migration tool. The call site survey shows that 86 % of the call sites have no optional arguments, and
   that 22 call sites need text computed at runtime.
