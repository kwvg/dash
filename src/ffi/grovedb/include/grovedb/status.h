// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_STATUS_H
#define GROVEDB_STATUS_H

#include <cstdint>
#include <string>
#include <utility>

namespace grovedb {

/**
 * RocksDB-inspired status class for error reporting without exceptions.
 *
 * Status carries an error code and an optional diagnostic message.
 * The success state (kOk) is constexpr-constructible and carries no
 * message.
 */
class [[nodiscard]] Status
{
public:
    enum Code : uint8_t {
        kOk = 0,
        kNotFound,
        kCorruption,
        kInvalidArgument,
        kIOError,
        kNotSupported,
        kAborted,
    };

    constexpr Status() : m_code{kOk} {}

    static constexpr Status Ok() { return Status{}; }

    static Status NotFound(std::string msg) { return {kNotFound, std::move(msg)}; }
    static Status Corruption(std::string msg) { return {kCorruption, std::move(msg)}; }
    static Status InvalidArgument(std::string msg) { return {kInvalidArgument, std::move(msg)}; }
    static Status IOError(std::string msg) { return {kIOError, std::move(msg)}; }
    static Status NotSupported(std::string msg) { return {kNotSupported, std::move(msg)}; }
    static Status Aborted(std::string msg) { return {kAborted, std::move(msg)}; }

    [[nodiscard]] constexpr Code code() const { return m_code; }
    [[nodiscard]] constexpr bool ok() const { return m_code == kOk; }
    const std::string& message() const { return m_message; }

private:
    Status(Code code, std::string msg) : m_code{code}, m_message{std::move(msg)} {}

    Code m_code;
    std::string m_message;
};

} // namespace grovedb

#endif // GROVEDB_STATUS_H
