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

class OldMnNetInfo : public interface::MnNetInfo
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

    MnNetStatus AddEntry(Purpose purpose, DomainPort service) override;
    MnNetStatus AddEntry(Purpose purpose, CService service) override;
    MnNetStatus RemoveEntry(DomainPort service) override;
    MnNetStatus RemoveEntry(CService service) override;

    const CService& GetPrimaryService() const override;
    std::vector<uint8_t> GetKey() const override;

    bool IsEmpty() const override;
    MnNetStatus Validate() const override;
    void Clear() override;

    UniValue ToJson() const override;
    std::string ToString() const override;
};

#endif // BITCOIN_EVO_LEGACY_H
