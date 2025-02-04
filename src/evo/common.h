// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_COMMON_H
#define BITCOIN_EVO_COMMON_H

#include <serialize.h>

#include <cstdint>
#include <string>

class CService;

class UniValue;

enum MnNetStatus : uint8_t
{
    // Adding entries
    Duplicate,
    BadInput,
    BadPort,
    MaxLimit,

    // Removing entries
    NotFound,

    Success
};

// TODO: Currently this corresponds to the index, is this a good idea?
enum class Purpose : uint8_t
{
    // Mandatory for all masternodes
    CORE_P2P = 0,
    // Mandatory for all EvoNodes
    PLATFORM_P2P = 1,
    // Optional for EvoNodes
    PLATFORM_API = 2
};
template<> struct is_serializable_enum<Purpose> : std::true_type {};

namespace interface {
// Interface shared between OldMnNetInfo and MnNetInfo
// !!! Remember to define the operators ==, != and < as well !!!
class MnNetInfo
{
public:
    //! Validates and adds entry to list
    virtual MnNetStatus AddEntry(Purpose purpose, CService service) = 0;

    //! Validates and removes entry from list, we don't need purpose as we shouldn't allow duplicates
    virtual MnNetStatus RemoveEntry(CService service) = 0;

    //! Returns first entry of purpose CORE_P2P
    virtual const CService& GetPrimaryService() const = 0;

    //! Gets unique identifier for object
    virtual std::vector<uint8_t> GetKey() const = 0;

    //! Returns true if object is equivalent to Clear()'ed state
    virtual bool IsEmpty() const = 0;

    //! Self-validates object
    virtual MnNetStatus Validate() const = 0;

    //! Clears object, used in reset routines.
    //! Also used to create reference object to check if object is empty
    virtual void Clear() = 0;

    //! Used by RPC code to display contents of object
    virtual UniValue ToJson() const = 0;

    //! Used by debug logging
    virtual std::string ToString() const = 0;
};
}; // namespace interface

#endif // BITCOIN_EVO_COMMON_H
