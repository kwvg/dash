// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/masternodemodel.h>

#include <interfaces/node.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <qt/clientmodel.h>

#include <univalue.h>

///
/// MasternodeEntry wrapper
///

MasternodeEntry::MasternodeEntry(ClientModel* _clientModel, std::unique_ptr<const interfaces::MnEntry>&& _dmn,
                                 const QString& collateralAddress, int nextPaymentHeight) :
    clientModel{_clientModel},
    m_dmn{std::move(_dmn)},
    m_banned{m_dmn->isBanned()},
    m_last_paid_height{m_dmn->getLastPaidHeight()},
    m_next_payment_height{nextPaymentHeight},
    m_pose_penalty{m_dmn->getPoSePenalty()},
    m_registered_height{m_dmn->getRegisteredHeight()},
    m_type{m_dmn->getType()},
    m_collateral_address{collateralAddress},
    m_protx_hash{QString::fromStdString(m_dmn->getProTxHash().ToString())},
    m_service{QString::fromStdString(m_dmn->getNetInfoPrimary().ToStringAddrPort())},
    m_type_description{QString::fromStdString(std::string(GetMnType(m_type).description))},
    m_operator_reward_pct{m_dmn->getOperatorReward()}
{
    // Cache service key for sorting
    auto addr_key = m_dmn->getNetInfoPrimary().GetKey();
    m_service_key = QByteArray(reinterpret_cast<const char*>(addr_key.data()), addr_key.size());

    // Cache payout address
    CTxDestination payeeDest;
    if (ExtractDestination(m_dmn->getScriptPayout(), payeeDest)) {
        m_payout_address = QString::fromStdString(EncodeDestination(payeeDest));
    } else {
        m_payout_address = QObject::tr("UNKNOWN");
    }

    // Cache owner address
    m_owner_address = QString::fromStdString(EncodeDestination(PKHash(m_dmn->getKeyIdOwner())));

    // Cache voting address
    m_voting_address = QString::fromStdString(EncodeDestination(PKHash(m_dmn->getKeyIdVoting())));

    // Cache operator reward string
    if (m_operator_reward_pct) {
        m_operator_reward = QString::number(m_operator_reward_pct / 100.0, 'f', 2) + "% ";

        if (m_dmn->getScriptOperatorPayout() != CScript()) {
            CTxDestination operatorDest;
            if (ExtractDestination(m_dmn->getScriptOperatorPayout(), operatorDest)) {
                m_operator_reward += QObject::tr("to %1").arg(QString::fromStdString(EncodeDestination(operatorDest)));
            } else {
                m_operator_reward += QObject::tr("to UNKNOWN");
            }
        } else {
            m_operator_reward += QObject::tr("but not claimed");
        }
    } else {
        m_operator_reward = QObject::tr("NONE");
    }
}

MasternodeEntry::~MasternodeEntry() = default;

QString MasternodeEntry::toJson() const
{
    UniValue json = m_dmn->toJson();
    return QString::fromStdString(json.write(2));
}

QString MasternodeEntry::collateralOutpoint() const
{
    return QString::fromStdString(m_dmn->getCollateralOutpoint().ToStringShort());
}
