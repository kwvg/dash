// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fuzz/utils/tempdir.h>

#include <cstdlib>
#include <random>

namespace grovedb {
namespace fuzz {
TempDir::TempDir(const char* prefix)
{
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint64_t> dis;

  auto base = std::filesystem::temp_directory_path();
  for (int attempts = 0; attempts < 100; ++attempts) {
    auto name = std::string(prefix) + "_" + std::to_string(dis(gen));
    auto candidate = base / name;
    if (std::filesystem::create_directory(candidate)) {
      m_path = candidate;
      return;
    }
  }
  std::abort();
}

TempDir::~TempDir()
{
  if (!m_path.empty()) {
    std::filesystem::remove_all(m_path);
  }
}

std::string TempDir::path() const
{
  return m_path.string();
}
} // namespace fuzz
} // namespace grovedb
