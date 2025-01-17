// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/address.h>

#include <bech32.h>
#include <chainparams.h>
#include <evo/deterministicmns.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <vector>

namespace {
template <typename T1>
inline std::nullopt_t error(T1& lhs, T1 rhs) { lhs = rhs; return std::nullopt; }

std::string HashToAddress(const uint256 hash) {
    std::vector<uint8_t> data = {0};
    data.reserve(1 + (hash.size() * 8 + 3) / 5);
    ConvertBits</*frombits=*/8, /*tobits=*/5, /*pad=*/true>([&](unsigned char c) { data.push_back(c); }, hash.begin(), hash.end());
    return bech32::Encode(bech32::Encoding::BECH32M, Params().MnAddrHRP(), data);
}

std::optional<uint256> AddressToHash(const std::string addr, MnAddr::DecodeStatus& status) {
    uint256 hash;
    const auto dec = bech32::Decode(addr);
    if (dec.encoding != bech32::Encoding::BECH32M) {
        return error(status, MnAddr::DecodeStatus::NotBech32m);
    }
    if (dec.data.empty()) {
        return error(status, MnAddr::DecodeStatus::DataEmpty);
    }
    if (dec.data[0] != /*expected_version*/0) {
        return error(status, MnAddr::DecodeStatus::DataVersionBad);
    }
    if (dec.hrp != Params().MnAddrHRP()) {
        return error(status, MnAddr::DecodeStatus::HRPBad);
    }
    std::vector<uint8_t> data;
    data.reserve(((dec.data.size() - 1) * 5) / 8);
    if (!ConvertBits</*frombits=*/5, /*tobits=*/8, /*pad=*/false>([&](unsigned char c) { data.push_back(c); }, dec.data.begin() + 1, dec.data.end())) {
        return error(status, MnAddr::DecodeStatus::DataPaddingBad);
    }
    if (data.size() != hash.size()) {
        return error(status, MnAddr::DecodeStatus::DataSizeBad);
    }
    std::copy(data.begin(), data.end(), hash.begin());
    status = MnAddr::DecodeStatus::Success;
    return hash;
}
} // anonymous namespace

MnAddr::MnAddr(std::string addr, MnAddr::DecodeStatus& status) :
    protx_hash{[&addr, &status]() {
        if (auto hash_opt = AddressToHash(addr, status); hash_opt.has_value()) { return hash_opt.value(); } else { return uint256::ZERO; }
    }()},
    is_valid{protx_hash != uint256::ZERO}
{}

std::string MnAddr::GetAddress() const { return HashToAddress(protx_hash); }

std::string DSToString(MnAddr::DecodeStatus status)
{
    switch (status) {
        case MnAddr::DecodeStatus::NotBech32m:
            return "bad encoding";
        case MnAddr::DecodeStatus::HRPBad:
            return "unsupported prefix or incorrect network";
        case MnAddr::DecodeStatus::DataEmpty:
            return "no data encoded";
        case MnAddr::DecodeStatus::DataVersionBad:
            return "bad version";
        case MnAddr::DecodeStatus::DataPaddingBad:
            return "bad data padding";
        case MnAddr::DecodeStatus::DataSizeBad:
            return "unexpected data size";
        case MnAddr::DecodeStatus::Success:
            return "c: yay !!!";
    }  // no default case, so the compiler can warn about missing cases

    assert(false);
}

std::optional<CService> GetConnectionDetails(CDeterministicMNManager& dmnman, const MnAddr mn_addr, std::string& error_str)
{
    if (!mn_addr.IsValid()) {
        return error(error_str, strprintf("Invalid address"));
    }
    const auto mn_list = dmnman.GetListAtChainTip();
    auto mn = mn_list.GetMN(mn_addr.GetHash());
    if (!mn) {
        return error(error_str, strprintf("Masternode not found in list"));
    }
    return Assert(mn->pdmnState)->addr;
}
