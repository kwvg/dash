// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
class FuzzedSignatureChecker : public BaseSignatureChecker
{
    FuzzedDataProvider& m_fuzzed_data_provider;

public:
    explicit FuzzedSignatureChecker(FuzzedDataProvider& fuzzed_data_provider) : m_fuzzed_data_provider(fuzzed_data_provider)
    {
    }

    bool CheckSig(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        return m_fuzzed_data_provider.ConsumeBool();
    }

    bool CheckLockTime(const CScriptNum& nLockTime) const override
    {
        return m_fuzzed_data_provider.ConsumeBool();
    }

    bool CheckSequence(const CScriptNum& nSequence) const override
    {
        return m_fuzzed_data_provider.ConsumeBool();
    }

    virtual ~FuzzedSignatureChecker() {}
};
} // namespace

FUZZ_TARGET(signature_checker)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const unsigned int flags = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const auto script_1{ConsumeScript(fuzzed_data_provider)};
    const auto script_2{ConsumeScript(fuzzed_data_provider)};
    std::vector<std::vector<unsigned char>> stack;
    (void)EvalScript(stack, script_1, flags, FuzzedSignatureChecker(fuzzed_data_provider), SigVersion::BASE, nullptr);
    if (!IsValidFlagCombination(flags)) {
        return;
    }
    (void)VerifyScript(script_1, script_2, flags, FuzzedSignatureChecker(fuzzed_data_provider), nullptr);
}
