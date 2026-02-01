// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <grovedb/db.h>

#include <test/util/tempdir.h>

#include <vector>

BOOST_AUTO_TEST_SUITE(batch_tests)

// ---------------------------------------------------------------------------
// Single insert
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_single_insert)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_single"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'k'}, item));

    auto status = db.ApplyBatch(ops, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());

    // Verify the element exists.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(root, {'k'}, fetched, cost).ok());
    BOOST_CHECK(!fetched.empty());
    BOOST_CHECK(fetched.data() == item.data());
}

// ---------------------------------------------------------------------------
// Multiple inserts
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_multiple_inserts)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_multi"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item_a, item_b, item_c;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item_a).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item_b).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'3'}, item_c).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'a'}, item_a));
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'b'}, item_b));
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'c'}, item_c));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    // Verify all three exist.
    for (uint8_t k = 'a'; k <= 'c'; ++k) {
        bool exists{false};
        BOOST_REQUIRE(db.KeyExists(root, {k}, exists, cost).ok());
        BOOST_CHECK(exists);
    }
}

// ---------------------------------------------------------------------------
// InsertOrReplace (upsert)
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_insert_or_replace)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_upsert"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert initial value.
    grovedb::Element item_old;
    BOOST_REQUIRE(grovedb::Element::Item({'o', 'l', 'd'}, item_old).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item_old, cost).ok());

    // Upsert with new value via batch.
    grovedb::Element item_new;
    BOOST_REQUIRE(grovedb::Element::Item({'n', 'e', 'w'}, item_new).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOrReplace(root, {'k'}, item_new));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    // Verify updated value.
    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(root, {'k'}, fetched, cost).ok());
    BOOST_CHECK(fetched.data() == item_new.data());
}

// ---------------------------------------------------------------------------
// Replace (must exist)
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_replace)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_replace"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert initial value.
    grovedb::Element item_old;
    BOOST_REQUIRE(grovedb::Element::Item({'o', 'l', 'd'}, item_old).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item_old, cost).ok());

    // Replace via batch.
    grovedb::Element item_new;
    BOOST_REQUIRE(grovedb::Element::Item({'n', 'e', 'w'}, item_new).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::Replace(root, {'k'}, item_new));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    grovedb::Element fetched;
    BOOST_REQUIRE(db.GetDirect(root, {'k'}, fetched, cost).ok());
    BOOST_CHECK(fetched.data() == item_new.data());
}

// ---------------------------------------------------------------------------
// Delete
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_delete)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_delete"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Delete via batch.
    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::Delete(root, {'k'}));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    // Verify element is gone.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(root, {'k'}, exists, cost).ok());
    BOOST_CHECK(!exists);
}

// ---------------------------------------------------------------------------
// DeleteTree
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_delete_tree)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_del_tree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Create an empty subtree.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());
    BOOST_REQUIRE(db.Put(root, {'t'}, tree, cost).ok());

    // Verify it exists.
    bool exists{false};
    BOOST_REQUIRE(db.SubtreeExists({{'t'}}, exists, cost).ok());
    BOOST_REQUIRE(exists);

    // Delete the tree via batch.
    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::DeleteTree(root, {'t'}));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    // Verify it no longer exists.
    exists = true;
    BOOST_REQUIRE(db.KeyExists(root, {'t'}, exists, cost).ok());
    BOOST_CHECK(!exists);
}

// ---------------------------------------------------------------------------
// Mixed operations
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_mixed_ops)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_mixed"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Pre-populate: insert 'a' (to be deleted) and 'b' (to be replaced).
    grovedb::Element item_a, item_b;
    BOOST_REQUIRE(grovedb::Element::Item({'1'}, item_a).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'2'}, item_b).ok());
    BOOST_REQUIRE(db.Put(root, {'a'}, item_a, cost).ok());
    BOOST_REQUIRE(db.Put(root, {'b'}, item_b, cost).ok());

    // Batch: delete 'a', replace 'b', insert 'c'.
    grovedb::Element item_b_new, item_c;
    BOOST_REQUIRE(grovedb::Element::Item({'B'}, item_b_new).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'3'}, item_c).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::Delete(root, {'a'}));
    ops.push_back(grovedb::BatchOperation::InsertOrReplace(root, {'b'}, item_b_new));
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'c'}, item_c));

    BOOST_REQUIRE(db.ApplyBatch(ops, cost).ok());

    // 'a' should be gone.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(root, {'a'}, exists, cost).ok());
    BOOST_CHECK(!exists);

    // 'b' should have the new value.
    grovedb::Element fetched_b;
    BOOST_REQUIRE(db.GetDirect(root, {'b'}, fetched_b, cost).ok());
    BOOST_CHECK(fetched_b.data() == item_b_new.data());

    // 'c' should exist.
    exists = false;
    BOOST_REQUIRE(db.KeyExists(root, {'c'}, exists, cost).ok());
    BOOST_CHECK(exists);
}

// ---------------------------------------------------------------------------
// Batch with transaction (commit)
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_with_transaction)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_tx"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());

    // Begin transaction, apply batch, commit.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'k'}, item));

    BOOST_REQUIRE(db.ApplyBatch(ops, txn, cost).ok());
    BOOST_REQUIRE(db.Commit(txn, cost).ok());

    // Verify element exists after commit.
    bool exists{false};
    BOOST_REQUIRE(db.KeyExists(root, {'k'}, exists, cost).ok());
    BOOST_CHECK(exists);
}

// ---------------------------------------------------------------------------
// Batch with transaction (rollback)
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_with_transaction_rollback)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_tx_rb"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());

    // Begin transaction, apply batch, rollback.
    grovedb::Transaction txn;
    BOOST_REQUIRE(db.BeginTransaction(txn).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'k'}, item));

    BOOST_REQUIRE(db.ApplyBatch(ops, txn, cost).ok());
    BOOST_REQUIRE(db.Rollback(txn).ok());

    // Element should NOT exist after rollback.
    bool exists{true};
    BOOST_REQUIRE(db.KeyExists(root, {'k'}, exists, cost).ok());
    BOOST_CHECK(!exists);
}

// ---------------------------------------------------------------------------
// Batch with options (validate insertion override)
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_with_options)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_opts"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Insert an item first.
    grovedb::Element item;
    BOOST_REQUIRE(grovedb::Element::Item({'v'}, item).ok());
    BOOST_REQUIRE(db.Put(root, {'k'}, item, cost).ok());

    // Try to InsertOnly over the existing key with override validation on.
    grovedb::Element item2;
    BOOST_REQUIRE(grovedb::Element::Item({'w'}, item2).ok());

    std::vector<grovedb::BatchOperation> ops;
    ops.push_back(grovedb::BatchOperation::InsertOnly(root, {'k'}, item2));

    grovedb::BatchApplyOptions options{
        .m_validate_insertion_does_not_override = true,
    };
    auto status = db.ApplyBatch(ops, options, cost);
    // Should fail because the key already exists.
    BOOST_CHECK(!status.ok());
}

// ---------------------------------------------------------------------------
// Empty batch
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_empty)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_empty"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    std::vector<grovedb::BatchOperation> ops;

    auto status = db.ApplyBatch(ops, cost);
    BOOST_CHECK_MESSAGE(status.ok(), status.message());
}

// ---------------------------------------------------------------------------
// Subtree operations across batches
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_apply_batch_subtree_operations)
{
    grovedb::test::TempDir tmp{"grovedb_test_batch_subtree"};
    grovedb::Db db;
    BOOST_REQUIRE(grovedb::Db::Open(tmp.PathToString(), db).ok());

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // First batch: create a subtree.
    grovedb::Element tree;
    BOOST_REQUIRE(grovedb::Element::EmptyTree(tree).ok());

    std::vector<grovedb::BatchOperation> ops1;
    ops1.push_back(grovedb::BatchOperation::InsertOnly(root, {'t'}, tree));
    BOOST_REQUIRE(db.ApplyBatch(ops1, cost).ok());

    // Verify subtree exists.
    bool exists{false};
    BOOST_REQUIRE(db.SubtreeExists({{'t'}}, exists, cost).ok());
    BOOST_REQUIRE(exists);

    // Second batch: insert items inside the subtree.
    grovedb::Path subtree{{'t'}};
    grovedb::Element item_x, item_y;
    BOOST_REQUIRE(grovedb::Element::Item({'X'}, item_x).ok());
    BOOST_REQUIRE(grovedb::Element::Item({'Y'}, item_y).ok());

    std::vector<grovedb::BatchOperation> ops2;
    ops2.push_back(grovedb::BatchOperation::InsertOnly(subtree, {'x'}, item_x));
    ops2.push_back(grovedb::BatchOperation::InsertOnly(subtree, {'y'}, item_y));
    BOOST_REQUIRE(db.ApplyBatch(ops2, cost).ok());

    // Verify items in subtree.
    exists = false;
    BOOST_REQUIRE(db.KeyExists(subtree, {'x'}, exists, cost).ok());
    BOOST_CHECK(exists);
    exists = false;
    BOOST_REQUIRE(db.KeyExists(subtree, {'y'}, exists, cost).ok());
    BOOST_CHECK(exists);
}

BOOST_AUTO_TEST_SUITE_END()
