// Copyright (c) 2017-2021 The Bitcoin Core developers
// Copyright (c) 2026-present, The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef GROVEDB_TEST_UTIL_FS_H
#define GROVEDB_TEST_UTIL_FS_H

#include <filesystem>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

/** Filesystem operations and types. */
namespace fs {

using namespace std::filesystem;

/**
 * Path class wrapper to block calls to the fs::path(std::string) implicit
 * constructor and the fs::path::string() method, which have unsafe and
 * unpredictable behavior on Windows (see implementation note in
 * PathToString for details).
 */
class path : public std::filesystem::path
{
public:
    using std::filesystem::path::path;

    // Allow path objects arguments for compatibility.
    path(std::filesystem::path path) : std::filesystem::path::path(std::move(path)) {}
    path& operator=(std::filesystem::path path) { std::filesystem::path::operator=(std::move(path)); return *this; }
    path& operator/=(const std::filesystem::path& path) { std::filesystem::path::operator/=(path); return *this; }

    // Allow literal string arguments, which are safe as long as the literals are ASCII.
    path(const char* c) : std::filesystem::path(c) {}
    path& operator=(const char* c) { std::filesystem::path::operator=(c); return *this; }
    path& operator/=(const char* c) { std::filesystem::path::operator/=(c); return *this; }
    path& append(const char* c) { std::filesystem::path::append(c); return *this; }

    // Disallow std::string arguments to avoid locale-dependent decoding on windows.
    path(std::string) = delete;
    path& operator=(std::string) = delete;
    path& operator/=(std::string) = delete;
    path& append(std::string) = delete;

    // Disallow std::string conversion method to avoid locale-dependent encoding on windows.
    std::string string() const = delete;

    /**
     * Return a UTF-8 representation of the path as a std::string, for
     * compatibility with code using std::string. For code using the newer
     * std::u8string type, it is more efficient to call the inherited
     * std::filesystem::path::u8string method instead.
     */
    std::string utf8string() const
    {
        const std::u8string& utf8_str{std::filesystem::path::u8string()};
        return std::string{utf8_str.begin(), utf8_str.end()};
    }

    // Required for path overloads in <fstream>.
    path& make_preferred() { std::filesystem::path::make_preferred(); return *this; }
    path filename() const { return std::filesystem::path::filename(); }
};

static inline path u8path(const std::string& utf8_str)
{
    return std::filesystem::path(std::u8string{utf8_str.begin(), utf8_str.end()});
}

// Allow safe path append operations.
static inline path operator/(path p1, const path& p2) { p1 /= p2; return p1; }
static inline path operator/(path p1, const char* p2) { p1 /= p2; return p1; }
static inline path operator+(path p1, const char* p2) { p1 += p2; return p1; }
static inline path operator+(path p1, path::value_type p2) { p1 += p2; return p1; }

// Disallow unsafe path append operations.
template<typename T> static inline path operator/(path p1, T p2) = delete;
template<typename T> static inline path operator+(path p1, T p2) = delete;

/**
 * Convert path object to a byte string. On POSIX, paths natively are byte
 * strings, so this is trivial. On Windows, paths natively are Unicode, so an
 * encoding step is necessary.
 */
static inline std::string PathToString(const path& path)
{
#ifdef WIN32
    return path.utf8string();
#else
    static_assert(std::is_same<path::string_type, std::string>::value, "PathToString not implemented on this platform");
    return path.std::filesystem::path::string();
#endif
}

/**
 * Convert byte string to path object. Inverse of PathToString.
 */
static inline path PathFromString(const std::string& string)
{
#ifdef WIN32
    return u8path(string);
#else
    return std::filesystem::path(string);
#endif
}

/**
 * Create directory (and if necessary its parents), unless the leaf directory
 * already exists or is a symlink to an existing directory.
 * This is a temporary workaround for an issue in libstdc++ that has been fixed
 * upstream [PR101510].
 */
static inline bool create_directories(const std::filesystem::path& p)
{
    if (std::filesystem::is_symlink(p) && std::filesystem::is_directory(p)) {
        return false;
    }
    return std::filesystem::create_directories(p);
}

bool create_directories(const std::filesystem::path& p, std::error_code& ec) = delete;

} // namespace fs

#endif // GROVEDB_TEST_UTIL_FS_H
