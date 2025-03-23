// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_LEGACY_H
#define BITCOIN_EVO_LEGACY_H

#include <netaddress.h>
#include <serialize.h>

class CService;

class UniValue;

enum NetInfoStatus : uint8_t
{
    BadInput,
    BadPort,

    Success
};

constexpr std::string_view MNSToString(const NetInfoStatus code) {
    switch (code) {
    case NetInfoStatus::BadInput:
        return "invalid network address";
    case NetInfoStatus::BadPort:
        return "invalid port";
    case NetInfoStatus::Success:
        return "success";
    } // no default case, so the compiler can warn about missing cases
}

class MnNetInfo
{
private:
    CService addr;

private:
    static NetInfoStatus ValidateService(const CService& service);

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return addr == rhs.addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.addr);
    }

    NetInfoStatus SetEntry(const std::string service);
    std::vector<CService> GetEntries() const { return { addr }; }

    const CService& GetPrimary() const { return addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }
    NetInfoStatus Validate() const { return ValidateService(addr); }
    UniValue ToJson() const;
    std::string ToString() const;

    void Clear() { addr = CService(); }
};

/* Identical to IsDeprecatedRPCEnabled("service"). For use outside of RPC code. */
bool IsServiceDeprecatedRPCEnabled();

#endif // BITCOIN_EVO_LEGACY_H
