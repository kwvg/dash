// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_LEGACY_H
#define BITCOIN_EVO_LEGACY_H

#include <evo/common.h>
#include <netaddress.h>
#include <serialize.h>

#include <optional>

class CService;

class UniValue;

class OldMnNetInfo
{
public:
    CService addr;

private:
    static MnNetStatus ValidateService(CService service);

public:
    OldMnNetInfo() = default;
    ~OldMnNetInfo() = default;

    bool operator==(const OldMnNetInfo& rhs) const { return addr == rhs.addr; }
    bool operator!=(const OldMnNetInfo& rhs) const { return !(*this == rhs); }
    bool operator<(const OldMnNetInfo& rhs) const { return this->addr < rhs.addr; }

    SERIALIZE_METHODS(OldMnNetInfo, obj)
    {
        READWRITE(obj.addr);
    }

    MnNetStatus AddEntry(Purpose purpose, CService service);
    MnNetStatus RemoveEntry(CService service);

    const CService& GetPrimaryService() const;
    std::vector<uint8_t> GetKey() const;

    bool IsEmpty() const;
    MnNetStatus Validate() const;
    void Clear();

    UniValue ToJson() const;
    std::string ToString() const;
};

#endif // BITCOIN_EVO_LEGACY_H
