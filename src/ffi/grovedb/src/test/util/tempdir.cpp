// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/tempdir.h>

#include <test/util/fs.h>

namespace grovedb {
namespace test {
TempDir::TempDir(const char* name)
    : m_path{fs::temp_directory_path() / name}
{
    fs::remove_all(m_path);
    fs::create_directories(m_path);
}

TempDir::~TempDir()
{
    fs::remove_all(m_path);
}

std::string TempDir::PathToString() const
{
    return fs::PathToString(m_path);
}

} // namespace test
} // namespace grovedb
