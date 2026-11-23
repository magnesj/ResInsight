# PDM object identity: contract for UUID adoption

Status: proposal for review. This document is step 2.0 of issue #1202. It settles the rules that steps 2.1-2.3
(runtime identity, server migration, rips migration) must follow. Nothing in this document is implemented.

Decisions marked **Decision** are choices made here. Items in [Open questions](#open-questions) need an answer from a
maintainer before the step that depends on them is started.

## Current state

Verified in ResInsight `dev` at `f2e3a182ae`.

| Use | Identity today | Source |
|---|---|---|
| Python handle | `class_keyword` and `uint64 address`, the memory address of the object | `PdmObject.proto`, `RiaGrpcServiceInterface.cpp:76` |
| Lookup of a handle | All descendants of the project with the class keyword are collected, and the pointers are compared | `RiaGrpcHelper::findCafObjectFromScriptNameAndAddress` |
| Object-valued arguments | Text `ClassName:address` | `pdmobject.py:275`, `cafPdmFieldScriptingCapability.h:722` |
| Pointer fields in project files | Path of field keywords and child indices from the root, for example `$ROOT$ Items 2` | `PdmReferenceHelper` |
| Undo/redo of field changes | Same path text | `cafCmdFieldChangeExec.cpp` |

Problems:

- Every lookup traverses the whole project.
- A handle to a deleted object resolves to another object if the allocator reuses the address and the class is the
  same. No error is reported.
- `RimCommandRouter` is resolved by class keyword alone, the address is ignored.
- Paths depend on the order of children. A path is only valid until siblings are added, removed or reordered.
- gRPC requests are processed on the main thread (`RiaGrpcApplicationInterface::processRequests`), so there is no
  concurrent access to the object tree today.

## Terms

- **Persistent identity**: the UUID of an object. Follows the object for as long as it exists, and is restored when a
  saved object is loaded (after step 2.4).
- **Handle**: what a client holds to refer to an object.
- **Document**: one object tree with a `PdmDocument` as root. ResInsight has one document, `RimProject`.

## 1. Type and generation

**Decision**: the identity type is `QUuid`, stored in `caf::PdmObjectHandle`. Text form is
`QUuid::toString( QUuid::WithoutBraces )`, lower case, 36 characters.

**Decision**: the UUID is generated lazily, on the first call to `uuid()`.

Measured on the benchmark machine (see `docs/pdm-init-functions-design.md`):

| | Cost |
|---|---|
| `QUuid::createUuid()` | 36 ns, 16 bytes |
| `QUuid::toString()` | 68 ns |
| Construction of a PDM object with ten fields | about 9 000 ns, 12 500 bytes |

Eager generation would add 0.4 % to construction time and 0.1 % to memory, so it is affordable. Lazy generation is
selected for a different reason: registration. An object that is never exposed to scripting and never saved does not
have to enter the registry, so construction and destruction of temporary objects are not affected at all.

`uuid()` is logically const. The lazy write is not thread safe, see [Threading](#7-threading).

## 2. Registry

**Decision**: one registry per document, owned by the root object. It is a hash map from `QUuid` to
`PdmObjectHandle*`. It does not own objects.

An object is a member of the registry of a document when both are true:

1. The object has a UUID.
2. The object is attached to the document, directly or through its ancestors.

Updates:

| Event | Action |
|---|---|
| `uuid()` generates a UUID for an attached object | Insert |
| A subtree is attached to a parent field | Insert all objects in the subtree that have a UUID |
| A subtree is detached from a parent field | Remove all objects in the subtree that have a UUID |
| A subtree is moved to another document | Remove from the old registry, insert into the new |
| An object is destroyed | Remove. This is done in `PdmObjectHandle::~PdmObjectHandle` and must not depend on virtual functions |
| The document is destroyed | The registry is destroyed first, objects do not have to remove themselves one by one |
| `setUuid()` during load | Insert, after the duplicate check in [section 4](#4-integrity) |

Attach and detach are already single points in the framework: `PdmObjectHandle::setAsParentField` and
`removeAsParentField`. The subtree traversal is only needed for objects that have a UUID. To keep attach of large
subtrees without any UUID cheap, each object stores a flag telling that the object or a descendant has a UUID.

Objects outside the project tree:

- `RimCommandRouter` is owned by the application, not by the project. **Decision**: the resolver keeps the special
  case. The command router gets a fixed handle and is not a member of any document registry.
- Objects returned from an object method with `persistent = false` are deleted after the reply is serialized. They
  never get a UUID and never enter a registry. Their handle is empty, see [section 5](#5-protocol).

## 3. Lifetime of handles

There are two independent questions: does a handle survive delete and undo, and does it survive reload of the project.

**Decision**: a handle is the persistent identity qualified by a session generation.

```
handle = ( document generation, uuid )
```

- The document generation is a counter owned by the application. It is incremented every time a project is closed,
  opened or reloaded.
- A handle is valid only if its generation is equal to the current generation, and its UUID is found in the registry.
- A handle created before the project was reloaded is stale, also when the reloaded file contains an object with the
  same UUID.

Reasons:

- Today a handle never survives reload, because addresses change. A script that depends on handles surviving reload
  does not exist. Keeping this behavior is the compatible choice.
- Without the generation, persistence of UUIDs (step 2.4) would silently change the meaning of old handles: a handle to
  an object in project A could resolve to an unrelated copy of that object in project B, if B was made from A with
  "Save as".
- The generation makes it possible to ship runtime identity (2.1-2.3) before persistence (2.4) with the same handle
  behavior before and after.

Delete followed by undo restores the object with its UUID (step 2.6). **Decision**: a handle to an object that is
deleted and restored by undo in the same generation becomes valid again. No object incarnation counter is used. The
restored object is the same object as far as the user and the project file are concerned.

## 4. Integrity

- `setUuid()` is not public API. It is available to the XML reader and to undo restore only.
- A UUID is never changed after it is set. `setUuid()` on an object that already has a different UUID is an error.
- Duplicate UUID in a document:
  - While loading a file: the first object keeps the UUID. Later objects with the same UUID get a new UUID, and one
    warning is logged for the file. References to the duplicated UUID resolve to the first object. The load does not
    fail, because a file with duplicates can be produced by manual editing or by merging files.
  - While attaching a subtree at runtime: this is a programming error. `CAF_ASSERT`, and the attached object gets a new
    UUID in builds without asserts.
- Copies never keep the UUID, see [section 6](#6-copy-clone-and-undo).

## 5. Protocol

### Resolution

One resolver is used for all object lookups, including object-valued arguments, arrays of objects and both streaming
calls:

```cpp
std::expected<caf::PdmObject*, RiaGrpcObjectError> resolve( const rips::PdmObject& handle, const QString& expectedScriptClass );
```

Rules, applied in order:

| Request | Result |
|---|---|
| UUID is present and malformed | `INVALID_ARGUMENT` |
| UUID is present, generation is not the current generation | `NOT_FOUND`, message tells that the handle is from a previous project session |
| UUID is present and is not found in the registry | `NOT_FOUND` |
| UUID is found, the class of the object does not match `class_keyword` | `FAILED_PRECONDITION` |
| UUID is found | The object |
| UUID is absent, address is non-zero | Legacy lookup by class keyword and address, as today |
| UUID and address are absent | `INVALID_ARGUMENT` |

A UUID that is present and does not resolve never falls back to the address. Fallback is only for requests without a
UUID.

The class check uses the same rules as today: the script class name is mapped to a class keyword, and the object must
inherit that class (`inheritsClassWithKeyword`). Class keyword aliases keep working, because the check is done on the
object and not on the text in the handle.

All callers must handle the error. Today the two streaming state handlers store the result of the lookup and use it
without a null check (`RiaGrpcPdmObjectService.cpp:174` and `:226`). This is fixed as part of step 2.2, independent of
UUID.

### Messages

New fields are added with new field numbers. Existing fields keep number, type and meaning.

```proto
message PdmObject {
  string class_keyword = 1;
  uint64 address = 2;              // Kept. Filled by new servers, used by old clients.
  map<string, string> parameters = 3;
  bool visible = 4;
  bool persistent = 5;
  string uuid = 6;                 // New. Empty for objects that are not persistent.
  uint64 document_generation = 7;  // New.
}
```

Object-valued arguments and return values are text inside `parameters`. The text format gets a second form:

| Form | Text |
|---|---|
| Legacy | `ClassName:address` |
| New | `ClassName:uuid:generation` |

The two forms are distinguished by the number of parts and by the UUID syntax (36 characters with hyphens). A reader
accepts both. One codec is used by `PdmFieldScriptingCapabilityIOHandler<DataType*>`, by the pointer array handlers and
by method arguments. The codec lives in `cafPdmScripting` and gets the resolver as a callback, because the registry
belongs to the document and the generation belongs to the application.

### Mixed versions

The server selects the form it writes from what the client is able to read. **Decision**: capability is announced
explicitly. `rips` sends gRPC metadata `rips-object-identity: uuid` with every call when it supports UUID handles.

| Client | Server | Behavior |
|---|---|---|
| Old | New | No metadata. The server fills `address`, and also `uuid`, which the old client ignores and sends back unchanged or not at all. Object references in parameters are written in legacy form. Lookup is legacy when the UUID is absent. Stale handles have the same weakness as today. |
| New | Old | The server ignores the metadata and unknown fields. Replies have empty `uuid`. The client uses `address` when `uuid` is empty, and writes legacy references. |
| New | New | Handles and references use UUID. Stale handles are rejected. |

A new client decides per object: an object with an empty `uuid` is addressed in legacy form. This also covers
objects that are not persistent.

`rips` compares its major and minor version with the server when it connects (`Instance.__check_version`), so the
mixed combinations occur in practice between patch versions, and for clients that skip the check.

`PdmObject.address()` in Python keeps returning the address. A new method `uuid()` is added. Generated class names,
method signatures, default values and documentation in `generated_classes.py` do not change.

## 6. Copy, clone and undo

XML reading has one boolean, `isCopyOperation`. **Decision**: it is replaced by an operation policy, passed
recursively to all child objects:

| Policy | UUID in the XML | Result |
|---|---|---|
| `LOAD` | Present | Restored |
| `LOAD` | Absent (file written by an older version) | No UUID. Generated lazily when needed |
| `CLONE` (`copyObject`, copy and paste, duplicate) | Ignored | New UUID for every object in the cloned subtree, generated lazily |
| `UNDO_RESTORE` | Present | Restored, also for all children |

For `CLONE`, a map from old UUID to new object is collected while the subtree is read. It is needed from step 2.5, when
references use UUIDs:

- A reference to an object inside the cloned subtree is redirected to the clone of that object.
- A reference to an object outside the subtree is kept, if the clone is inserted into the same document. This is the
  behavior of path references today, when the copy is inserted next to the original.
- A reference to an object outside the subtree is cleared, if the clone is inserted into another document.

Fields with `isCopyable() == false` are skipped for `CLONE` and read for the other policies, as today.

## 7. Threading

- The registry is confined to the main thread. This is the thread that owns the project tree and processes gRPC
  requests.
- No lock is added. A debug assert verifies the thread in registry functions.
- Worker threads must not call `uuid()` on an object without a UUID, attach or detach objects of a document, or
  resolve handles. Reading the UUID of an object that already has one is safe.
- A resolved pointer is valid until control returns to the event loop. A request handler must not keep it across
  calls that process events. The streaming handlers keep a pointer for the duration of a stream: **Decision**: they
  store the handle, and resolve it again for every chunk.

## 8. File compatibility

Not decided here. Steps 2.4 and 2.5 need a separate approval, with a test where a released version of ResInsight reads
a file written with the new format.

Constraints already known:

- Old files without UUID must load without warnings.
- Path references must still be read, including field keyword aliases in the path. The alias behavior is covered by
  `KeywordAliasTest` in `cafPdmXml_UnitTests`.
- A candidate representation is an XML attribute on the object element. Whether released versions ignore such an
  attribute is not known, and has to be verified with an older executable. Field elements already carry attributes,
  which are passed to the field capabilities.
- When a reference has both a UUID and a path, and they disagree, the UUID wins if it resolves. This rule is only safe
  if a reader that understands UUIDs always writes both consistently.

## Tests required for steps 2.1-2.3

Runtime identity (2.1):

- Lazy generation, stable value, text form.
- Two documents with independent registries.
- Attach, detach and move of subtrees with and without UUIDs.
- Destruction of objects and of the document in both orders.
- Duplicate UUID at load and at attach.
- Copy gets new UUIDs for all objects in the subtree.
- Objects that are never attached: no registry entry, no leak.
- Benchmark: construction and destruction of objects without UUID is unchanged. Lookup time is independent of the
  number of objects.

Server (2.2):

- Deleted handle, for all request types including both streams.
- Address reuse: delete an object, create an object of the same class until the address is reused, call with the old
  handle. Must fail with a UUID handle.
- Malformed UUID, wrong class, UUID and address that disagree (UUID wins, no fallback).
- Handle from a previous generation after close, open and reload.
- Legacy request without UUID gives the same result as today.
- Command router handle.

rips (2.3):

- Matrix of old and new client and server for: field read and write, object-valued argument, array of objects,
  returned object, both streams, object that is not persistent.
- `generated_classes.py` is identical before and after.

## Open questions

1. **Handles after reload.** This document selects handles that expire when the project is closed or reloaded. The
   alternative is handles that survive reload once UUIDs are persisted. That would be a new feature for scripts, and
   can be added later by relaxing the generation check. Is expiry the wanted behavior?
2. **Handles after delete and undo.** This document lets a handle become valid again when undo restores the object. Is
   that acceptable for scripts that deleted the object on purpose?
3. **Capability announcement.** gRPC metadata is proposed. The alternative is a version check, using the version call
   that `rips` already makes when it connects. Metadata is selected because it does not couple the protocol to
   ResInsight version numbers.
4. **Duplicate UUID in a file.** This document repairs and warns. The alternative is to reject the file.
5. **Scope of persistence** (for step 2.4): UUID for all objects in the file, or only for objects that have been
   exposed. Lazy generation makes "only objects that have a UUID" the natural default, but then saved identity depends
   on whether a script has touched the object. This must be decided together with the file format.
