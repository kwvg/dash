// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <chainparamsbase.h>
#include <net.h>
#include <net_permissions.h>
#include <netaddress.h>
#include <protocol.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

void initialize_net()
{
    static const BasicTestingSetup basic_testing_setup;
}

FUZZ_TARGET_INIT(net, initialize_net)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const std::optional<CAddress> address = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
    if (!address) {
        return;
    }
    const std::optional<CAddress> address_bind = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
    if (!address_bind) {
        return;
    }

    CNode node{fuzzed_data_provider.ConsumeIntegral<NodeId>(),
               static_cast<ServiceFlags>(fuzzed_data_provider.ConsumeIntegral<uint64_t>()),
               fuzzed_data_provider.ConsumeIntegral<int>(),
               INVALID_SOCKET,
               *address,
               fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
               fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
               *address_bind,
               fuzzed_data_provider.ConsumeRandomLengthString(32),
               fuzzed_data_provider.ConsumeBool(),
               fuzzed_data_provider.ConsumeBool()
           };
    while (fuzzed_data_provider.ConsumeBool()) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CAddrMan addrman;
                CConnman connman{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>(), addrman};
                node.CloseSocketDisconnect(&connman);
            },
            [&] {
                node.MaybeSetAddrName(fuzzed_data_provider.ConsumeRandomLengthString(32));
            },
            [&] {
                node.SetSendVersion(fuzzed_data_provider.ConsumeIntegral<int>());
            },
            [&] {
                const std::vector<bool> asmap = ConsumeRandomLengthBitVector(fuzzed_data_provider);
                if (!SanityCheckASMap(asmap)) {
                    return;
                }
                CNodeStats stats;
                node.copyStats(stats, asmap);
            },
            [&] {
                node.SetRecvVersion(fuzzed_data_provider.ConsumeIntegral<int>());
            },
            [&] {
                const CNode* add_ref_node = node.AddRef();
                assert(add_ref_node == &node);
            },
            [&] {
                if (node.GetRefCount() > 0) {
                    node.Release();
                }
            },
            [&] {
                // if (node.m_addr_known == nullptr) {
                //     break;
                // }
                const std::optional<CAddress> addr_opt = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
                if (!addr_opt) {
                    return;
                }
                node.AddAddressKnown(*addr_opt);
            },
            [&] {
                // if (node.m_addr_known == nullptr) {
                //     break;
                // }
                const std::optional<CAddress> addr_opt = ConsumeDeserializable<CAddress>(fuzzed_data_provider);
                if (!addr_opt) {
                    return;
                }
                FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
                node.PushAddress(*addr_opt, fast_random_context);
            },
            [&] {
                const std::optional<CInv> inv_opt = ConsumeDeserializable<CInv>(fuzzed_data_provider);
                if (!inv_opt) {
                    return;
                }
                // node.AddKnownTx(inv_opt->hash);
            },
            [&] {
                const std::optional<CInv> inv_opt = ConsumeDeserializable<CInv>(fuzzed_data_provider);
                if (!inv_opt) {
                    return;
                }
                node.PushInventory(*inv_opt);
            },
            [&] {
                const std::optional<CService> service_opt = ConsumeDeserializable<CService>(fuzzed_data_provider);
                if (!service_opt) {
                    return;
                }
                node.SetAddrLocal(*service_opt);
            },
            [&] {
                const std::vector<uint8_t> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                bool complete;
                node.ReceiveMsgBytes((const char*)b.data(), b.size(), complete);
            });
    }

    (void)node.GetAddrLocal();
    (void)node.GetAddrName();
    (void)node.GetId();
    (void)node.GetLocalNonce();
    (void)node.GetLocalServices();
    (void)node.GetMyStartingHeight();
    (void)node.GetRecvVersion();
    const int ref_count = node.GetRefCount();
    assert(ref_count >= 0);
    (void)node.GetSendVersion();
    (void)node.IsAddrRelayPeer();

    const NetPermissionFlags net_permission_flags = fuzzed_data_provider.ConsumeBool() ?
                                                        fuzzed_data_provider.PickValueInArray<NetPermissionFlags>({NetPermissionFlags::PF_NONE, NetPermissionFlags::PF_BLOOMFILTER, NetPermissionFlags::PF_RELAY, NetPermissionFlags::PF_FORCERELAY, NetPermissionFlags::PF_NOBAN, NetPermissionFlags::PF_MEMPOOL, NetPermissionFlags::PF_ISIMPLICIT, NetPermissionFlags::PF_ALL}) :
                                                        static_cast<NetPermissionFlags>(fuzzed_data_provider.ConsumeIntegral<uint32_t>());
    (void)node.HasPermission(net_permission_flags);
}
