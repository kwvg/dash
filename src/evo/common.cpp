// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/common.h>

std::string PurposeToString(Purpose purpose) {
    switch (purpose) {
    case Purpose::CORE_P2P:
        return "CORE_P2P";
    case Purpose::PLATFORM_P2P:
        return "PLATFORM_P2P";
    case Purpose::PLATFORM_API:
        return "PLATFORM_API";
    }  // no default case, so the compiler can warn about missing cases
    assert(false);
}
