# Error Handling

## The Result Type

Every operation in the GroveDB C++ API returns [`grovedb::Result<T, Error>`](@ref grovedb::Result). A `grovedb::Result` either holds a success value (`T`) or an [`Error`](@ref grovedb::Error) — there's no ambiguous "empty" state. You always know whether an operation succeeded or failed.

```cpp
#include <grovedb/result.h>  // for Result

auto result = db.Get(path, key);
// result either holds the value or an Error
```

You can work with results in two styles: **imperative** (explicit `if`/`else` checking) or **chained** (`.and_then()` / `.map()`). Both are idiomatic — use whichever fits the situation.

## Imperative Style

Check results directly with `has_value()`:

```cpp
auto result = db.Get(path, key);
if (result.has_value()) {
  auto& val = result.value();
  // or: auto& val = *result;
} else {
  auto& err = result.error();
  // err.code() — ErrorCode enum
  // err.message() — human-readable string
}
```

This works well for simple operations. But GroveDB's return types are often nested — [`Get`](@ref grovedb::Db::Get) doesn't return a bare [`Element`](@ref grovedb::Element), it returns `grovedb::Result<Costed<Element>, Error>`. And [`GetOptional`](@ref grovedb::Db::GetOptional) returns `grovedb::Result<Costed<std::optional<Element>>, Error>`. Unwrapping these imperatively means multiple levels of checking:

```cpp
// Imperative approach to GetOptional — verbose but explicit
auto result = db.GetOptional(path, key);
if (result.has_value()) {
  grovedb::Costed<std::optional<grovedb::Element>>& costed = result.value();
  if (costed.value().has_value()) {
    grovedb::Element& elem = costed.value().value();
    // finally: use the element
  }
}
```

This is where chaining becomes valuable.

## Chained Style

[`grovedb::Result`](@ref grovedb::Result) supports `.and_then()` and `.map()` for composing operations without manually unwrapping at each step. The examples below show common patterns and why you'd reach for each one.

### Unwrapping a Costed value

[`Get`](@ref grovedb::Db::Get) returns `grovedb::Result<Costed<Element>, Error>` — you usually want the [`Element`](@ref grovedb::Element), not the cost wrapper. `.map()` strips the layer away:

```cpp
auto elem = db.Get(root, key).map([](grovedb::Costed<grovedb::Element> ce) {
  return ce.value();
});
// grovedb::Result<Element, Error> — the Costed wrapper is gone
```

If `Get` fails, `.map` is skipped and the error propagates. If it succeeds, the lambda extracts the inner value.

### Validated create-then-insert

`Element::Item` can fail (e.g. if the bytes are too large). Chaining guarantees the element is valid before it reaches `Put`:

```cpp
grovedb::Element::Item(grovedb::Bytes::FromString("hello"))
  .and_then([&](grovedb::Element e) {
    return db.Put(root, key, e);
  });
```

There's no way to accidentally insert an invalid element — `.and_then` won't call `Put` unless `Item` succeeded.

### Accumulating costs

Operations return [`OperationCost`](@ref grovedb::OperationCost), which supports `operator+=`. In a loop, chain the happy path and accumulate costs imperatively:

```cpp
grovedb::OperationCost total{};
for (int i = 0; i < 100; ++i) {
  auto key = grovedb::Bytes::FromString(std::format("key_{:03d}", i));
  auto result = grovedb::Element::Item(grovedb::Bytes::FromString("data"))
    .and_then([&](grovedb::Element e) {
      return db.Put(root, key, e);
    });
  if (!result.has_value()) {
    // handle error — total reflects cost of operations completed so far
    break;
  }
  total += *result;
}
fprintf(stdout, "total: %lu seeks, %lu bytes added\n", total.m_seek_count, total.m_storage_added_bytes);
```

The chain handles validation (element creation → insertion), while the loop handles accumulation and early exit — each style where it's strongest.

### Build a query and execute it

[`PathQuery::New`](@ref grovedb::PathQuery::New) returns a `grovedb::Result` too. Chain it directly into the query execution:

```cpp
auto result = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeInclusive(from, to)})
  .and_then([&](grovedb::PathQuery q) {
    return db.QueryValues(q);
  });
```

If the query construction fails (e.g. invalid predicates), the execution never runs.

### Prove and verify in one chain

Generate a proof and verify it without intermediate variables:

```cpp
auto verified = grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
  .and_then([&](grovedb::PathQuery q) { return db.Prove(q); })
  .and_then([&](grovedb::Costed<grovedb::Bytes> proof) {
    return grovedb::PathQuery::New(root, {grovedb::QueryItem::RangeFull()})
      .and_then([&](grovedb::PathQuery q) {
        return db.VerifyQuery(proof.value(), q);
      });
  });
```

Each step depends on the previous one succeeding. If any step fails, the entire chain short-circuits to that error.

## When Imperative Is Better

Chaining shines for linear pipelines. But some patterns are clearer with `if`/`else`:

### Loops with early exit

When processing a batch of keys and you need per-item error handling:

```cpp
std::vector<grovedb::Bytes> keys = /* ... */;
for (const auto& key : keys) {
  auto result = db.Get(root, key);
  if (!result.has_value()) {
    if (result.error().code() == grovedb::ErrorCode::NotFound) {
      continue;  // skip missing keys
    }
    fprintf(stderr, "unexpected error: %s\n", result.error().message().c_str());
    return false;
  }
  // process result.value()
}
```

A chain can't `continue` or `break` — control flow like this needs imperative style.

### Branching on error type

When different errors need different recovery:

```cpp
auto result = db.Get(root, key);
if (!result.has_value()) {
  switch (result.error().code()) {
  case grovedb::ErrorCode::NotFound:
    // insert a default value
    break;
  case grovedb::ErrorCode::Corruption:
    // trigger integrity check
    break;
  default:
    // propagate
    return false;
  }
}
```

### Logging intermediate steps

When you need visibility into each stage:

```cpp
auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("data"));
if (!elem.has_value()) {
  fprintf(stderr, "element creation failed: %s\n", elem.error().message().c_str());
  return false;
}

auto put = db.Put(root, key, *elem);
if (!put.has_value()) {
  fprintf(stderr, "put failed: %s\n", put.error().message().c_str());
  return false;
}
fprintf(stdout, "put cost: %lu seeks\n", put->m_seek_count);
```

In practice, most code mixes both: chain the happy path with `.and_then()`, then check the final result imperatively.

## Error Codes

Every [`Error`](@ref grovedb::Error) carries an [`ErrorCode`](@ref grovedb::ErrorCode) discriminant:

| Code | When it occurs |
|------|----------------|
| `NotFound` | Key or path doesn't exist |
| `Corruption` | Data integrity failure (storage corruption, invalid proof) |
| `InvalidArgument` | Invalid parameters (bad path, malformed element) |
| `IOError` | Filesystem or storage I/O failure |
| `NotSupported` | Operation not supported in this context |
| `Aborted` | Transaction conflict or operation aborted |

Create errors with factory methods:

```cpp
auto err = grovedb::Error::NotFound("key 'alice' not found");
// err.code() == ErrorCode::NotFound
// err.message() == "key 'alice' not found"
```

Errors convert to a human-readable string:

```cpp
fprintf(stderr, "%s\n", err.to_string().c_str());
// prints "NotFound: key 'alice' not found"
```

## Error Propagation

Errors propagate through `.and_then()` chains without explicit checking. Failed operations skip all subsequent steps and forward the error:

```cpp
auto fail_chain = db.Get(root, grovedb::Bytes::FromString("nonexistent"))
  .and_then([](grovedb::Costed<grovedb::Element> ce) -> grovedb::Result<std::string, grovedb::Error> {
    // This lambda never runs — Get already failed
    return std::format("value has {} bytes", ce.value().data().size());
  });

// fail_chain.has_value() == false
// fail_chain.error().code() == ErrorCode::NotFound
```

## Constructing Errors

Use [`grovedb::Err()`](@ref grovedb::Err) to construct error results:

```cpp
grovedb::Result<int, grovedb::Error> validate(int x) {
  if (x < 0) {
    return grovedb::Err(grovedb::Error::InvalidArgument("x must be non-negative"));
  }
  return x;
}
```

## Pattern: Handling Optional Keys

When a missing key is expected (not an error), use [`GetOptional`](@ref grovedb::Db::GetOptional) instead of [`Get`](@ref grovedb::Db::Get):

```cpp
// Get returns an error for missing keys
auto get = db.Get(path, key);  // fails with NotFound

// GetOptional returns nullopt for missing keys
auto opt = db.GetOptional(path, key);
if (opt.has_value() && opt->value().has_value()) {
  // key exists
} else if (opt.has_value()) {
  // key doesn't exist — not an error, just nullopt
}
```

For all error handling patterns, see [`contrib/examples/error_handling.cpp`](../libgrovedb/contrib/examples/error_handling.cpp).
