// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/netinfo.h>

NetInfoStatus MnNetInfo::SetEntry(CService service)
{
    addr = service;
    return NetInfoStatus::Success;
}
