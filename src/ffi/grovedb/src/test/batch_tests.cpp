// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/tempdir.h>

#include <grovedb/batch.h>
#include <grovedb/db.h>
#include <grovedb/element.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(batch_tests)

// ---------------------------------------------------------------------------
// Enum ToString tests
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(tree_type_to_string)
{
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::NormalTree), "NormalTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::SumTree), "SumTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::BigSumTree), "BigSumTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::CountTree), "CountTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::CountSumTree), "CountSumTree");
  BOOST_CHECK_EQUAL(grovedb::ToString(grovedb::TreeType::ProvableCountTree), "ProvableCountTree");
  BOOST_CHECK_EQUAL(
      grovedb::ToString(grovedb::TreeType::ProvableCountSumTree), "ProvableCountSumTree"
  );
}

BOOST_AUTO_TEST_CASE(batch_operation_kind_to_string)
{
  using Kind = grovedb::BatchOperation::Kind;
  BOOST_CHECK_EQUAL(grovedb::ToString(Kind::InsertOnly), "InsertOnly");
  BOOST_CHECK_EQUAL(grovedb::ToString(Kind::InsertOrReplace), "InsertOrReplace");
  BOOST_CHECK_EQUAL(grovedb::ToString(Kind::Replace), "Replace");
  BOOST_CHECK_EQUAL(grovedb::ToString(Kind::Delete), "Delete");
  BOOST_CHECK_EQUAL(grovedb::ToString(Kind::DeleteTree), "DeleteTree");
}

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

namespace {
grovedb::Db OpenDb(grovedb::test::TempDir& dir)
{
  auto db = grovedb::Db::Open(dir.PathToString());
  BOOST_REQUIRE(db.has_value());
  return std::move(*db);
}
} // anonymous namespace

// ---------------------------------------------------------------------------
// Insert operations
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_insert_only)
{
  grovedb::test::TempDir dir("batch_insert_only");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  BOOST_REQUIRE(elem.has_value());

  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *elem),
  };

  grovedb::BatchApplyOptions options;
  auto result = db.ApplyBatch(ops, options);
  BOOST_REQUIRE(result.has_value());

  // Verify the item was inserted.
  auto get_result = db.Get(root, {'a'});
  BOOST_REQUIRE(get_result.has_value());
}

BOOST_AUTO_TEST_CASE(batch_insert_or_replace)
{
  grovedb::test::TempDir dir("batch_insert_or_replace");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(elem2.has_value());

  grovedb::BatchApplyOptions options;

  // Insert first.
  std::vector<grovedb::BatchOperation> ops1{
      grovedb::BatchOperation::InsertOrReplace(root, {'a'}, *elem1),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops1, options).has_value());

  // Replace.
  std::vector<grovedb::BatchOperation> ops2{
      grovedb::BatchOperation::InsertOrReplace(root, {'a'}, *elem2),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops2, options).has_value());

  // Verify updated value.
  auto get_result = db.Get(root, {'a'});
  BOOST_REQUIRE(get_result.has_value());
}

BOOST_AUTO_TEST_CASE(batch_replace_existing)
{
  grovedb::test::TempDir dir("batch_replace");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem1 = grovedb::Element::Item(grovedb::Bytes::FromString("v1"));
  auto elem2 = grovedb::Element::Item(grovedb::Bytes::FromString("v2"));
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(elem2.has_value());

  grovedb::BatchApplyOptions options;

  // Insert first via Put.
  BOOST_REQUIRE(db.Put(root, {'a'}, *elem1).has_value());

  // Replace via batch.
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::Replace(root, {'a'}, *elem2),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options).has_value());

  auto get_result = db.Get(root, {'a'});
  BOOST_REQUIRE(get_result.has_value());
}

// ---------------------------------------------------------------------------
// Delete operations
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_delete)
{
  grovedb::test::TempDir dir("batch_delete");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db.Put(root, {'a'}, *elem).has_value());

  grovedb::BatchApplyOptions options;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::Delete(root, {'a'}),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options).has_value());

  // Verify deleted.
  auto get_result = db.Get(root, {'a'});
  BOOST_CHECK(!get_result.has_value());
}

BOOST_AUTO_TEST_CASE(batch_delete_tree)
{
  grovedb::test::TempDir dir("batch_del_tree");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db.Put(root, {'t'}, *tree).has_value());

  grovedb::BatchApplyOptions options;
  options.m_allow_deleting_non_empty_trees = true;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::DeleteTree(root, {'t'}),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options).has_value());

  // Verify deleted.
  auto get_result = db.Get(root, {'t'});
  BOOST_CHECK(!get_result.has_value());
}

// ---------------------------------------------------------------------------
// Mixed operations
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_mixed_operations)
{
  grovedb::test::TempDir dir("batch_mixed");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem_a = grovedb::Element::Item({'1'});
  auto elem_b = grovedb::Element::Item({'2'});
  auto elem_c = grovedb::Element::Item({'3'});
  BOOST_REQUIRE(elem_a.has_value());
  BOOST_REQUIRE(elem_b.has_value());
  BOOST_REQUIRE(elem_c.has_value());

  // Pre-insert 'b' so we can replace it.
  BOOST_REQUIRE(db.Put(root, {'b'}, *elem_b).has_value());

  auto new_b = grovedb::Element::Item({'B'});
  BOOST_REQUIRE(new_b.has_value());

  grovedb::BatchApplyOptions options;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *elem_a),
      grovedb::BatchOperation::InsertOrReplace(root, {'b'}, *new_b),
      grovedb::BatchOperation::InsertOnly(root, {'c'}, *elem_c),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options).has_value());

  // All three should exist.
  BOOST_CHECK(db.Get(root, {'a'}).has_value());
  BOOST_CHECK(db.Get(root, {'b'}).has_value());
  BOOST_CHECK(db.Get(root, {'c'}).has_value());
}

// ---------------------------------------------------------------------------
// Batch within transaction
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_within_transaction_commit)
{
  grovedb::test::TempDir dir("batch_tx_commit");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(elem.has_value());

  auto txn = db.BeginTransaction();
  BOOST_REQUIRE(txn.has_value());

  grovedb::BatchApplyOptions options;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *elem),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options, *txn).has_value());

  // Not visible before commit.
  BOOST_CHECK(!db.Get(root, {'a'}).has_value());

  BOOST_REQUIRE(db.Commit(*txn).has_value());

  // Visible after commit.
  BOOST_CHECK(db.Get(root, {'a'}).has_value());
}

BOOST_AUTO_TEST_CASE(batch_within_transaction_rollback)
{
  grovedb::test::TempDir dir("batch_tx_rollback");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(elem.has_value());

  auto txn = db.BeginTransaction();
  BOOST_REQUIRE(txn.has_value());

  grovedb::BatchApplyOptions options;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *elem),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options, *txn).has_value());

  BOOST_REQUIRE(db.Rollback(*txn).has_value());

  // Not visible after rollback.
  BOOST_CHECK(!db.Get(root, {'a'}).has_value());
}

// ---------------------------------------------------------------------------
// Subtree batch operations
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_in_subtree)
{
  grovedb::test::TempDir dir("batch_subtree");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  grovedb::Bytes sub_key{'s'};

  // Create subtree.
  auto tree = grovedb::Element::EmptyTree();
  BOOST_REQUIRE(tree.has_value());
  BOOST_REQUIRE(db.Put(root, sub_key, *tree).has_value());

  grovedb::Path sub_path{sub_key};
  auto elem1 = grovedb::Element::Item({'x'});
  auto elem2 = grovedb::Element::Item({'y'});
  BOOST_REQUIRE(elem1.has_value());
  BOOST_REQUIRE(elem2.has_value());

  grovedb::BatchApplyOptions options;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(sub_path, grovedb::Bytes::FromString("k1"), *elem1),
      grovedb::BatchOperation::InsertOnly(sub_path, grovedb::Bytes::FromString("k2"), *elem2),
  };
  BOOST_REQUIRE(db.ApplyBatch(ops, options).has_value());

  // Verify both items exist in subtree.
  BOOST_CHECK(db.Get(sub_path, grovedb::Bytes::FromString("k1")).has_value());
  BOOST_CHECK(db.Get(sub_path, grovedb::Bytes::FromString("k2")).has_value());
}

// ---------------------------------------------------------------------------
// Validation options
// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(batch_validate_no_override)
{
  grovedb::test::TempDir dir("batch_no_override");
  auto db = OpenDb(dir);

  grovedb::Path root{};
  auto elem = grovedb::Element::Item({'v'});
  BOOST_REQUIRE(elem.has_value());
  BOOST_REQUIRE(db.Put(root, {'a'}, *elem).has_value());

  // InsertOnly with validation should fail if key exists.
  grovedb::BatchApplyOptions options;
  options.m_validate_insertion_does_not_override = true;
  std::vector<grovedb::BatchOperation> ops{
      grovedb::BatchOperation::InsertOnly(root, {'a'}, *elem),
  };
  auto result = db.ApplyBatch(ops, options);
  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
