// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_NETINFO_H
#define BITCOIN_EVO_NETINFO_H

#include <netaddress.h>
#include <serialize.h>

class CService;

enum NetInfoStatus : uint8_t
{
    Success
};

class MnNetInfo
{
private:
    CService addr;

public:
    MnNetInfo() = default;
    ~MnNetInfo() = default;

    bool operator==(const MnNetInfo& rhs) const { return addr == rhs.addr; }
    bool operator!=(const MnNetInfo& rhs) const { return !(*this == rhs); }

    SERIALIZE_METHODS(MnNetInfo, obj)
    {
        READWRITE(obj.addr);
    }

    NetInfoStatus AddEntry(const CService& service);

    const CService& GetPrimary() const { return addr; }
    bool IsEmpty() const { return *this == MnNetInfo(); }

    void Clear() { addr = CService(); }
};

#endif // BITCOIN_EVO_NETINFO_H
