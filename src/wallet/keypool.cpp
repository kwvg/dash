// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/keypool.h>

#include <coinjoin/coinjoin-client.h>
#include <wallet/wallet.h>

template <class PublicKey>
CKeyPool<PublicKey>::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
}

template <class PublicKey>
CKeyPool<PublicKey>::CKeyPool(const PublicKey& vchPubKeyIn, bool fInternalIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = fInternalIn;
}

template <class PublicKey>
bool CReserveKey<PublicKey>::GetReservedKey(PublicKey& pubkey, bool fInternalIn)
{
    if (nIndex == -1)
    {
        CKeyPool<CPubKey> keypool;
        if (!pwallet->ReserveKeyFromKeyPool(nIndex, keypool, fInternalIn)) {
            return false;
        }
        vchPubKey = keypool.vchPubKey;
        fInternal = keypool.fInternal;
    }
    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

template <class PublicKey>
void CReserveKey<PublicKey>::KeepKey()
{
    if (nIndex != -1) {
        pwallet->KeepKey(nIndex);
    }
    nIndex = -1;
    vchPubKey = PublicKey();
}

template <class PublicKey>
void CReserveKey<PublicKey>::ReturnKey()
{
    if (nIndex != -1) {
        pwallet->ReturnKey(nIndex, fInternal, vchPubKey);
    }
    nIndex = -1;
    vchPubKey = PublicKey();
}

template class CKeyPool<CPubKey>;
template class CReserveKey<CPubKey>;
