// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_FUZZ_UTILS_TEMPDIR_H
#define GROVEDB_FUZZ_UTILS_TEMPDIR_H

#include <filesystem>
#include <string>

namespace grovedb {
namespace fuzz {
/**
 * RAII temporary directory for fuzz targets.
 *
 * Creates a unique directory on construction and removes it on destruction.
 */
class TempDir
{
public:
  /**
   * Create a temporary directory with the given prefix.
   *
   * @param[in] prefix  Prefix for the directory name.
   */
  explicit TempDir(const char* prefix);

  ~TempDir();

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  /** @return Absolute path to the temporary directory. */
  [[nodiscard]] std::string path() const;

private:
  std::filesystem::path m_path;
};
} // namespace fuzz
} // namespace grovedb

#endif // GROVEDB_FUZZ_UTILS_TEMPDIR_H
