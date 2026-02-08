// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef GROVEDB_UTIL_LOGGING_H
#define GROVEDB_UTIL_LOGGING_H

#include <cstdio>
#include <format>
#include <string_view>

namespace grovedb {
namespace log {
/** Log severity level. */
enum class Level : uint8_t { INFO, WARN, ERROR };
namespace detail {
/** Dispatch a formatted log message to stdout (INFO) or stderr (WARN/ERROR). */
inline void Print(Level level, const char* file, int line, const char* func, const std::string& msg)
{
  std::string_view path(file);
  auto pos = path.find_last_of('/');
  if (pos != std::string_view::npos) {
    path = path.substr(pos + 1);
  }
  auto p = static_cast<int>(path.size());
  switch (level) {
  case Level::INFO:
    std::fprintf(stdout, "%.*s:%d %s() [INFO]: %s\n", p, path.data(), line, func, msg.c_str());
    break;
  case Level::WARN:
    std::fprintf(
        stderr, "\033[33m%.*s:%d %s() [WARN]: %s\033[0m\n", p, path.data(), line, func, msg.c_str()
    );
    break;
  case Level::ERROR:
    std::fprintf(
        stderr, "\033[31m%.*s:%d %s() [ERROR]: %s\033[0m\n", p, path.data(), line, func, msg.c_str()
    );
    break;
  }
}
} // namespace detail
} // namespace log
} // namespace grovedb

/** @brief Log a message at the given level (INFO, WARN, ERROR).
 *
 * INFO prints to stdout. WARN (yellow) and ERROR (red) print to stderr
 * with ANSI color codes. Uses std::format for message formatting.
 *
 * @code
 * Log(INFO, "inserted key {} with {} bytes", key, n);
 * Log(WARN, "key not found, using default");
 * Log(ERROR, "failed to open db: {}", err.message());
 * @endcode
 */
#define Log(level, ...)                                                                            \
  grovedb::log::detail::Print(                                                                     \
      grovedb::log::Level::level, __FILE__, __LINE__, __func__, std::format(__VA_ARGS__)           \
  )

#endif // GROVEDB_UTIL_LOGGING_H
