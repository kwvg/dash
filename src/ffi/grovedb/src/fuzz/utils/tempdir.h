// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_FUZZ_UTILS_TEMPDIR_H
#define GROVEDB_FUZZ_UTILS_TEMPDIR_H

#include <filesystem>
#include <string>

namespace grovedb {
namespace fuzz {

/** RAII temporary directory -- created on construction, removed on destruction. */
class TempDir
{
public:
    explicit TempDir(const char* prefix);
    ~TempDir();

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::string path() const;

private:
    std::filesystem::path m_path;
};

} // namespace fuzz
} // namespace grovedb

#endif // GROVEDB_FUZZ_UTILS_TEMPDIR_H
