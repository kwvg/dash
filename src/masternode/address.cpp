// Copyright (c) 2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/address.h>

#include <bech32.h>
#include <chainparams.h>
#include <util/strencodings.h>

#include <vector>
#include <optional>

namespace {
inline std::nullopt_t error(std::string& dest, std::string message) { dest = message; return std::nullopt; }

std::string HashToAddress(const uint256 hash) {
    std::vector<uint8_t> data = {0};
    data.reserve(1 + (hash.size() * 8 + 3) / 5);
    ConvertBits</*frombits=*/8, /*tobits=*/5, /*pad=*/true>([&](unsigned char c) { data.push_back(c); }, hash.begin(), hash.end());
    return bech32::Encode(bech32::Encoding::BECH32M, Params().MnAddrHRP(), data);
}

std::optional<uint256> AddressToHash(const std::string addr, std::string& error_str) {
    uint256 hash;
    const auto dec = bech32::Decode(addr);
    if (dec.encoding != bech32::Encoding::BECH32M) {
        return error(error_str, "Invalid address, bad encoding");
    }
    if (dec.data.empty()) {
        return error(error_str, "Invalid address, no data encoded");
    }
    if (dec.data[0] != /*expected_version*/0) {
        return error(error_str, strprintf("Invalid address, bad version (got %s, expected %s)", dec.data[0], 0));
    }
    if (dec.hrp != Params().MnAddrHRP()) {
        return error(error_str, strprintf("Invalid address, unsupported prefix or incorrect network (got %s, expected %s)",
                                          dec.hrp, Params().MnAddrHRP()));
    }
    std::vector<uint8_t> data;
    data.reserve(((dec.data.size() - 1) * 5) / 8);
    if (!ConvertBits</*frombits=*/5, /*tobits=*/8, /*pad=*/false>([&](unsigned char c) { data.push_back(c); }, dec.data.begin() + 1, dec.data.end())) {
        return error(error_str, "Invalid address, bad padding in data section");
    }
    if (data.size() != hash.size()) {
        return error(error_str, strprintf("Invalid address, unexpected data size (got %s, expected %s)", data.size(),
                                          hash.size()));
    }
    std::copy(data.begin(), data.end(), hash.begin());
    return hash;
}
} // anonymous namespace

MnAddr::MnAddr(std::string addr, std::string& error_str) :
    protx_hash{[&addr, &error_str]() {
        if (auto hash_opt = AddressToHash(addr, error_str); hash_opt.has_value()) { return hash_opt.value(); } else { return uint256::ZERO; }
    }()},
    is_valid{protx_hash != uint256::ZERO}
{}

std::string MnAddr::GetAddress() const { return HashToAddress(protx_hash); }
