// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// FUZZ TARGET: fuzz_tree_state
// Stateful: instruction-based program execution.
//
// Adapted from cosmos/iavl tree_fuzz_test.go
// Copyright (c) contributors to the cosmos/iavl project.
// Licensed under Apache 2.0.
//

#include <FuzzedDataProvider.h>
#include <utils/check.h>
#include <utils/grovedb.h>
#include <utils/tempdir.h>

#include <grovedb/db.h>

#include <cstdint>
#include <vector>

namespace {

enum Instruction : uint8_t {
    kSet,
    kRemove,
    kCreateSubtree,
    kDeleteTree,
    INSTRUCTION_COUNT,
};

struct Command {
    Instruction m_op;
    grovedb::Bytes m_key;
    grovedb::Bytes m_value;
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    grovedb::fuzz::TempDir tmp{"fuzz_tree_state"};
    grovedb::Db db;
    if (!grovedb::Db::Open(tmp.path(), db).ok()) return 0;

    grovedb::OperationCost cost{};
    grovedb::Path root{};

    // Generate an instruction sequence.
    auto program_len = fdp.ConsumeIntegralInRange<size_t>(1, 32);
    std::vector<Command> program;
    program.reserve(program_len);

    for (size_t i{0}; i < program_len && fdp.remaining_bytes() > 0; ++i) {
        Command cmd;
        cmd.m_op = static_cast<Instruction>(
            fdp.ConsumeIntegralInRange<uint8_t>(0, INSTRUCTION_COUNT - 1));
        cmd.m_key = grovedb::fuzz::ConsumeKey(fdp);
        cmd.m_value = grovedb::fuzz::ConsumeValue(fdp);
        program.push_back(std::move(cmd));
    }

    // Track which subtree keys we've created so we can operate within them.
    std::vector<grovedb::Bytes> subtree_keys;

    // Execute each instruction.
    for (const auto& cmd : program) {
        switch (cmd.m_op) {
        case kSet: {
            grovedb::Element item;
            if (!grovedb::Element::Item(cmd.m_value, item).ok()) break;
            (void)db.Put(root, cmd.m_key, item, cost);
            break;
        }
        case kRemove: {
            (void)db.Delete(root, cmd.m_key, cost);
            break;
        }
        case kCreateSubtree: {
            grovedb::Element tree;
            if (!grovedb::Element::EmptyTree(tree).ok()) break;
            if (db.Put(root, cmd.m_key, tree, cost).ok()) {
                subtree_keys.push_back(cmd.m_key);
            }
            break;
        }
        case kDeleteTree: {
            if (!subtree_keys.empty()) {
                // Delete the most recently created subtree.
                auto& key = subtree_keys.back();
                bool empty{false};
                if (db.IsEmptyTree(grovedb::Path{key}, empty, cost).ok() && empty) {
                    (void)db.Delete(root, key, cost);
                }
                subtree_keys.pop_back();
            }
            break;
        }
        default:
            break;
        }
    }

    // Verify DB integrity after all operations.
    bool integrity{false};
    (void)db.VerifyIntegrity(integrity);

    return 0;
}
