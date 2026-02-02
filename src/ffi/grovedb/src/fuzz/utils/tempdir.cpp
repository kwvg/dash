// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <utils/tempdir.h>

namespace grovedb {
namespace fuzz {

TempDir::TempDir(const char* prefix)
    : m_path{std::filesystem::temp_directory_path() / prefix}
{
    std::filesystem::remove_all(m_path);
    std::filesystem::create_directories(m_path);
}

TempDir::~TempDir()
{
    std::filesystem::remove_all(m_path);
}

std::string TempDir::path() const
{
    return m_path.string();
}

} // namespace fuzz
} // namespace grovedb
