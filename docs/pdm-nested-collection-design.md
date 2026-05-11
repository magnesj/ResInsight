# `caf::PdmNestedCollection` — design notes

## Context

`caf::PdmNestedCollection` is the base class for tree-shaped PDM containers — a
folder of items that can also contain folders of the same kind. The current
design is a CRTP template:

```cpp
template <typename SelfT, typename ItemT>
class PdmNestedCollection : public PdmObjectCollection<ItemT>,
                            public PdmNestedCollectionInterface;
```

`SelfT` provides the type of subcollections (`PdmChildArrayField<SelfT*>
m_subCollections`); `ItemT` provides the type of leaf items (inherited from
`PdmObjectCollection<ItemT>` via `m_items`).

A separate non-templated `PdmNestedCollectionInterface` lets generic command
features (e.g. `RicNewNestedCollectionFeature`) operate on any nested
collection without knowing the template parameters.

This note records why the templated design was kept after a comparison with a
non-template alternative.

## Alternative considered: non-templated base

```cpp
class PdmNestedCollection : public PdmObject
{
public:
    QString    collectionName() const;
    void       setCollectionName( const QString& name );
    PdmObject* addNewSubCollection();
    void       setAsTopmostFolder();

protected:
    void registerSubCollectionsField( PdmChildArrayFieldHandle* field );
    void registerItemsField( PdmChildArrayFieldHandle* field );
    virtual PdmObject* createSubCollection() const = 0;

    PdmField<QString> m_collectionName;
    bool              m_isTopLevelFolder;
    // m_subCollectionsField / m_itemsField stored as type-erased handles
};
```

Each derived class would own its typed `PdmChildArrayField<DerivedType*>
m_subCollections` and `PdmChildArrayField<ItemType*> m_items`, register them
with the base in the constructor, and override `createSubCollection()`.

`PdmNestedCollectionInterface` collapses into the base — one type instead of
three.

## Per-derived-class boilerplate (non-template version)

About 25 lines per derived class beyond what the templated version requires:

- Two `PdmChildArrayField<...>` field declarations (currently inherited).
- Override of `createSubCollection()` returning `new MyType()`.
- Two `registerXxxField(&m_xxx)` calls in the constructor.
- Re-declared typed accessors (`subCollections()`, `items()`,
  `addSubCollection(MyType*)`) — the base can only return `PdmObject*`.

## Line-count comparison

| Component                          | Templated                          | Non-template                       |
| ---------------------------------- | ---------------------------------- | ---------------------------------- |
| Interface header                   | ~50 lines                          | (folded into base)                 |
| Base header                        | ~80 lines                          | ~45 lines                          |
| Base impl                          | ~150 lines (`.inl`, in headers)    | ~55 lines (`.cpp`)                 |
| **Base subtotal**                  | **~280 lines**                     | **~100 lines**                     |
| Per-derived overhead               | 0                                  | ~25 lines                          |
| **Total — 1 derived class**        | **~280**                           | **~125**                           |
| **Total — 7 derived classes**      | **~280**                           | **~275** (break-even)              |
| **Total — 10 derived classes**     | **~280**                           | **~350**                           |

The break-even point sits around 7 derived classes.

## Decision

ResInsight has roughly 8–10 collection classes that match the nested-folder
pattern, so the templated design wins on:

- **Total lines of code** at the expected scale.
- **Compile-time type safety** — `addSubCollection(SelfT*)` and
  `subCollections()` are typed at the base, so every derived class gets it for
  free without re-declaring accessors.
- **Less boilerplate per derived class**, which matters more as the count of
  derived classes grows.

The non-template version's main advantages — simpler type hierarchy, faster
header parsing, no `.inl` — do not outweigh the per-derived savings the
template gives at this scale.

**Conclusion: keep the templated `PdmNestedCollection`.** The
`PdmNestedCollectionInterface` overhead is the price for letting generic
features operate on any concrete instance without template parameters, and is
worth paying.

## When to revisit

- If the count of nested-collection types drops to 1–2.
- If the template instantiation cost shows up in build profiling for the PDM
  layer.
- If the typed accessors are rarely used in practice and most call sites
  immediately cast to `PdmObject*` anyway.
