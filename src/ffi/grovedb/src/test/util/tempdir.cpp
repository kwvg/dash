// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#include <test/util/fs.h>
#include <test/util/tempdir.h>

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
