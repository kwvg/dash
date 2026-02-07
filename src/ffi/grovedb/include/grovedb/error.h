// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE.MIT or https://opensource.org/license/mit

#ifndef LIBGROVEDB_ERROR_H
#define LIBGROVEDB_ERROR_H

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace grovedb {
/**
 * Primary error discriminant for GroveDB operations.
 *
 * Every Error carries one of these codes as its primary classification.
 */
enum class ErrorCode : uint8_t {
  Ok = 0,
  NotFound,
  Corruption,
  InvalidArgument,
  IOError,
  NotSupported,
  Aborted,
};

/** @return String representation of the error code. */
constexpr std::string_view ToString(ErrorCode code)
{
  switch (code) {
  case ErrorCode::Ok:
    return "Ok";
  case ErrorCode::NotFound:
    return "NotFound";
  case ErrorCode::Corruption:
    return "Corruption";
  case ErrorCode::InvalidArgument:
    return "InvalidArgument";
  case ErrorCode::IOError:
    return "IOError";
  case ErrorCode::NotSupported:
    return "NotSupported";
  case ErrorCode::Aborted:
    return "Aborted";
  } // no default case, so the compiler can warn about missing cases
  return "Unknown";
}

/**
 * Error type for GroveDB operations, designed as a Result error payload.
 *
 * Carries an ErrorCode discriminant and an optional human-readable message.
 * Constructed via static factory methods (e.g., Error::NotFound("key missing")).
 */
class Error
{
public:
  /** Construct a default (Ok) error — represents no error. */
  Error() = default;

  /** Construct an error with a code and message. */
  Error(ErrorCode code, std::string message)
      : m_code{code}
      , m_message{std::move(message)}
  {
  }

  /** @return The error code. */
  [[nodiscard]] constexpr ErrorCode code() const
  {
    return m_code;
  }

  /** @return The human-readable error message. */
  [[nodiscard]] const std::string& message() const
  {
    return m_message;
  }

  /** @return True if this represents no error (ErrorCode::Ok). */
  [[nodiscard]] constexpr bool ok() const
  {
    return m_code == ErrorCode::Ok;
  }

  // -- Factory methods ----------------------------------------------------

  static Error NotFound(std::string msg)
  {
    return {ErrorCode::NotFound, std::move(msg)};
  }
  static Error Corruption(std::string msg)
  {
    return {ErrorCode::Corruption, std::move(msg)};
  }
  static Error InvalidArgument(std::string msg)
  {
    return {ErrorCode::InvalidArgument, std::move(msg)};
  }
  static Error IOError(std::string msg)
  {
    return {ErrorCode::IOError, std::move(msg)};
  }
  static Error NotSupported(std::string msg)
  {
    return {ErrorCode::NotSupported, std::move(msg)};
  }
  static Error Aborted(std::string msg)
  {
    return {ErrorCode::Aborted, std::move(msg)};
  }

private:
  ErrorCode m_code{ErrorCode::Ok};
  std::string m_message;
};
/** @brief Stream insertion for ErrorCode. */
inline std::ostream& operator<<(std::ostream& os, ErrorCode code)
{
  return os << ToString(code);
}

/** @brief Stream insertion for Error. */
inline std::ostream& operator<<(std::ostream& os, const Error& err)
{
  os << ToString(err.code());
  if (!err.message().empty()) {
    os << ": " << err.message();
  }
  return os;
}
} // namespace grovedb

#endif // LIBGROVEDB_ERROR_H
