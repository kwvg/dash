// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_TEST_UTIL_TEMPDIR_H
#define GROVEDB_TEST_UTIL_TEMPDIR_H

#include <test/util/fs.h>

#include <string>

namespace grovedb {
namespace test {
class TempDir
{
public:
  explicit TempDir(const char* name);
  ~TempDir();

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  /** Return the directory path as a byte string. */
  std::string PathToString() const;

private:
  fs::path m_path;
};
} // namespace test
} // namespace grovedb

#endif // GROVEDB_TEST_UTIL_TEMPDIR_H
