# Advanced Topics

> **Prerequisites:** [Basic Operations](05-basic-operations.md), [Transactions](08-transactions.md)

## Checkpoints

Checkpoints create immutable, read-only snapshots of the database at a point in time. They use RocksDB's hard-link-based checkpoint mechanism — creation is near-instant regardless of database size.

### Creating a Checkpoint

```cpp
auto result = db.CreateCheckpoint("/path/to/checkpoint");
```

The checkpoint directory must not already exist. After creation, the checkpoint is an independent copy that won't be affected by further writes to the original database.

### Opening a Checkpoint

```cpp
auto ckpt = grovedb::Db::OpenCheckpoint("/path/to/checkpoint");
if (ckpt.has_value()) {
  auto hash = ckpt->GetRootHash();
  auto data = ckpt->Get(path, key);
  // read-only operations work normally
}
```

### Deleting a Checkpoint

```cpp
auto result = grovedb::Db::DeleteCheckpoint("/path/to/checkpoint");
```

Verifies the path is a valid checkpoint before deletion.

### Use Case: State Sync

Checkpoints enable state syncing — like Bitcoin's `assumeutxo`. A node can:

1. Create a checkpoint at a known block height
2. Serve the checkpoint to syncing nodes
3. The syncing node verifies the checkpoint's root hash against the block header
4. Bootstrap from the checkpoint instead of replaying every block from genesis

The original database continues accepting writes while the checkpoint is being served.

For the complete checkpoint example, see [`contrib/examples/checkpoint_snapshot.cpp`](../libgrovedb/contrib/examples/checkpoint_snapshot.cpp).

## Auxiliary Storage

Auxiliary storage provides out-of-tree key-value pairs that do **not** participate in the Merkle commitment. Changes to auxiliary data don't affect the root hash.

```cpp
// Store auxiliary data
db.PutAux(grovedb::Bytes::FromString("sync_height"),
      grovedb::Bytes::FromString("750000"));

// Retrieve
auto result = db.GetAux(grovedb::Bytes::FromString("sync_height"));
// grovedb::Result<Costed<std::optional<Bytes>>, Error>

// Delete
db.DeleteAux(grovedb::Bytes::FromString("sync_height"));
```

All three operations accept an optional [`Transaction`](@ref grovedb::Transaction)`&` parameter.

### Use Case

Store node-local metadata alongside the authenticated state: peer scores, sync progress, local configuration. This data is useful for the node operator but shouldn't be part of the consensus-committed state.

For the complete auxiliary storage example, see [`contrib/examples/auxiliary_storage.cpp`](../libgrovedb/contrib/examples/auxiliary_storage.cpp).

## Cost Tracking

Every operation returns an [`OperationCost`](@ref grovedb::OperationCost) with six fields measuring resource consumption:

| Field | Description |
|-------|-------------|
| `m_seek_count` | Database seeks (random I/O operations) |
| `m_storage_added_bytes` | New bytes written to storage |
| `m_storage_replaced_bytes` | Bytes of existing data overwritten |
| `m_storage_removed_bytes` | Bytes freed from storage |
| `m_storage_loaded_bytes` | Bytes read from storage |
| `m_hash_node_calls` | Number of hash computations performed |

### Fee Market Integration

In a blockchain, these costs feed directly into fee calculation. A block producer can meter every state access and price operations based on actual resource consumption:

```cpp
grovedb::OperationCost block_cost{};
for (const auto& tx : block.transactions()) {
  auto result = process_transaction(db, tx);
  block_cost += result.cost();
}
// block_cost contains total resources consumed by the block
// use for fee validation and gas metering
```

Costs are comparable across operations — a `Put` that adds 200 bytes of storage costs more than one that adds 50 bytes. This determinism is essential for consensus: all nodes must agree on the cost of every state transition.

For detailed cost tracking patterns, see [`contrib/examples/operation_costs.cpp`](../libgrovedb/contrib/examples/operation_costs.cpp).

## Integrity Verification

[`VerifyIntegrity`](@ref grovedb::Db::VerifyIntegrity) walks the entire tree hierarchy and checks all hashes:

```cpp
auto result = db.VerifyIntegrity();
// grovedb::Result<bool, Error> — true if all hashes are valid
```

Use after unclean shutdown, when importing state from an untrusted source, or as a periodic sanity check. This is computationally expensive — it reads and hashes the entire database.

## Tree Introspection

```cpp
// Discover all subtree paths under a root
auto paths = db.FindSubtrees(root);

// Get the current state commitment
auto hash = db.GetRootHash();

// Check structure
auto exists = db.SubtreeExists(path);
auto empty = db.IsEmptyTree(path);
```

[`FindSubtrees`](@ref grovedb::Db::FindSubtrees) returns every path in the hierarchy under the given root — useful for debugging, migration, and tree visualization.

For tree inspection patterns, see [`contrib/examples/tree_state.cpp`](../libgrovedb/contrib/examples/tree_state.cpp).

## Sum Trees

Sum trees automatically maintain a running sum of all their `SumItem` children. The sum is tracked in the tree structure itself — no iteration required to get the total.

### Creating a Sum Tree

```cpp
// Create the SumTree container
grovedb::Element::EmptySumTree().and_then([&](grovedb::Element e) {
  return db.Put(root, grovedb::Bytes::FromString("balances"), e);
});

// Insert SumItems (each contributes to the sum)
grovedb::Path balances{grovedb::Bytes::FromString("balances")};

grovedb::Element::SumItem(1000).and_then([&](grovedb::Element e) {
  return db.Put(balances, grovedb::Bytes::FromString("alice"), e);
});
grovedb::Element::SumItem(500).and_then([&](grovedb::Element e) {
  return db.Put(balances, grovedb::Bytes::FromString("bob"), e);
});
grovedb::Element::SumItem(-200).and_then([&](grovedb::Element e) {
  return db.Put(balances, grovedb::Bytes::FromString("charlie"), e);
});
```

### Querying Sums

[`QuerySums`](@ref grovedb::Db::QuerySums) returns aggregate sum values:

```cpp
auto sums = grovedb::PathQuery::New(balances, {grovedb::QueryItem::RangeFull()})
        .and_then([&](grovedb::PathQuery q) {
          return db.QuerySums(q);
        });
// sums->value().m_data contains the individual sum values
```

[`QueryItemsOrSums`](@ref grovedb::Db::QueryItemsOrSums) returns a tagged union for each entry, distinguishing between item data and sum values:

```cpp
auto result = db.QueryItemsOrSums(query);
for (const auto& entry : result->value().m_data) {
  switch (entry.m_kind) {
  case grovedb::QueryItemOrSumKind::SumValue:
    // entry.m_sum_value contains the int64_t sum
    break;
  case grovedb::QueryItemOrSumKind::ItemData:
    // entry.m_item_data contains raw bytes
    break;
  // ... other variants
  }
}
```

### Use Cases

- **Total stake tracking** — sum of all validator stakes, provable without iteration
- **Vote counting** — aggregate votes for governance proposals
- **Balance aggregation** — total coins held by a set of accounts

SumItem values can be negative (`int64_t`), so debits and credits can be tracked naturally.

For the complete sum tree example, see [`contrib/examples/sum_trees.cpp`](../libgrovedb/contrib/examples/sum_trees.cpp).
