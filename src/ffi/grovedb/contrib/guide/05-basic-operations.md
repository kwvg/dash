# Basic Operations

> **Prerequisites:** [Getting Started](03-getting-started.md), [Elements and Trees](04-elements-and-trees.md)

## Put

Insert or replace an element at the given path and key:

```cpp
auto result = db.Put(path, key, element);
// grovedb::Result<OperationCost, Error>
```

Every mutation returns its resource cost — an `OperationCost` with seek counts, storage bytes added/replaced/removed, and hash operations. In a blockchain context, these feed directly into fee calculation.

```cpp
auto put = grovedb::Element::Item(grovedb::Bytes::FromString("Hello GroveDB!"))
         .and_then([&](grovedb::Element e) {
           return db.Put(root, grovedb::Bytes::FromString("greeting"), e);
         });
if (put.has_value()) {
  // put->m_seek_count, put->m_storage_added_bytes, etc.
}
```

All Put variants also accept a `Transaction&` parameter for transactional writes. See [Transactions](08-transactions.md).

## Conditional Puts

GroveDB provides three conditional insertion variants for common blockchain state patterns:

### PutIfAbsent

Insert only if the key doesn't exist. Returns `true` if inserted, `false` if the key was already present:

```cpp
auto result = db.PutIfAbsent(path, key, element);
// grovedb::Result<Costed<bool>, Error>
```

Use case: idempotent replays. When replaying blocks, you want to insert state entries only if they don't already exist. `PutIfAbsent` avoids overwriting valid state during catch-up.

### PutIfAbsentAndGet

Insert if absent; if the key already exists, return the existing element:

```cpp
auto result = db.PutIfAbsentAndGet(path, key, element);
// grovedb::Result<Costed<std::optional<Element>>, Error>
// nullopt if inserted (key was new), Element if key already existed
```

Use case: deduplication. When processing transactions that might create duplicate entries, this atomically checks and either inserts or retrieves the existing value.

### PutIfChanged

Insert only if the new value differs from the existing one. Returns whether the value changed and the previous element:

```cpp
auto result = db.PutIfChanged(path, key, element);
// grovedb::Result<Costed<ChangedValue>, Error>
// ChangedValue { bool m_changed; std::optional<Element> m_previous; }
```

Use case: state caching. Skip unnecessary tree rebalancing when the value hasn't actually changed. If your block processing recalculates derived state, `PutIfChanged` avoids the I/O and hash cost of writing an identical value.

For all three conditional variants demonstrated, see [`contrib/examples/conditional_insert.cpp`](../libgrovedb/contrib/examples/conditional_insert.cpp).

## Get

Retrieve an element by path and key:

```cpp
auto result = db.Get(path, key);
// grovedb::Result<Costed<Element>, Error>
```

The result bundles the element with its operation cost via `Costed<T>`. Access both:

```cpp
if (result.has_value()) {
  auto& element = result->value();    // the Element
  auto& cost = result->cost();        // OperationCost
}
```

### Get Variants

| Method | Returns | When to use |
|--------|---------|-------------|
| `Get` | `Costed<Element>` | Standard retrieval, follows references |
| `GetDirect` | `Costed<Element>` | Don't follow references — get the raw element |
| `GetOptional` | `Costed<std::optional<Element>>` | Returns `nullopt` instead of error for missing keys |
| `KeyExists` | `Costed<bool>` | Check presence without loading the value |
| `SubtreeExists` | `Costed<bool>` | Check if all parent subtrees in a path exist |
| `IsEmptyTree` | `Costed<bool>` | Check if a subtree has no children |

`GetOptional` is particularly useful in "check if exists" flows — it returns `nullopt` for missing keys instead of an error, making the calling code cleaner:

```cpp
auto result = db.GetOptional(path, key);
if (result.has_value() && result->value().has_value()) {
  // key exists, use *result->value()
} else if (result.has_value()) {
  // key doesn't exist (nullopt), not an error
}
```

## Delete

Remove an element at the given path and key:

```cpp
auto result = db.Delete(path, key);
// grovedb::Result<OperationCost, Error>
```

### Delete Variants

| Method | Returns | Behavior |
|--------|---------|----------|
| `Delete` | `OperationCost` | Remove the element |
| `DeleteIfEmpty` | `Costed<bool>` | Delete only if the element is an empty subtree (safety check) |
| `PruneEmptyAncestors` | `Costed<uint32_t>` | Delete and recursively remove empty parent subtrees |
| `Clear` | `bool` | Remove all elements within a subtree |

`PruneEmptyAncestors` is useful for cleanup — after deleting the last item in a subtree, it removes the now-empty subtree and any empty ancestors above it:

```cpp
auto result = db.PruneEmptyAncestors(path, key);
// result->value() = number of ancestor levels removed
```

## Operation Costs

Every operation reports its resource consumption through `OperationCost`:

| Field | Type | Description |
|-------|------|-------------|
| `m_seek_count` | `uint32_t` | Number of database seeks |
| `m_storage_added_bytes` | `uint32_t` | Bytes of new storage written |
| `m_storage_replaced_bytes` | `uint32_t` | Bytes of existing storage overwritten |
| `m_storage_removed_bytes` | `uint32_t` | Bytes of storage freed |
| `m_storage_loaded_bytes` | `uint64_t` | Bytes loaded from storage |
| `m_hash_node_calls` | `uint32_t` | Number of hash computations |

Costs are additive — you can accumulate them across multiple operations:

```cpp
grovedb::OperationCost total{};
for (/* each operation */) {
  auto result = db.Put(path, key, elem);
  if (result.has_value()) {
    total += *result;
  }
}
// total now contains cumulative costs
```

In a blockchain context, a block producer can meter every state access and price operations based on actual resource consumption rather than estimates. This is how GroveDB supports fee markets — the cost of a state transition is deterministic and measurable.

For detailed cost tracking, see [`contrib/examples/operation_costs.cpp`](../libgrovedb/contrib/examples/operation_costs.cpp).
