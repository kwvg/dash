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

///
/// MasternodeModel
///

int MasternodeModel::rowCount(const QModelIndex& index) const
{
    return static_cast<int>(m_data.size());
}

int MasternodeModel::columnCount(const QModelIndex& index) const
{
    return Column::_COUNT;
}

QVariant MasternodeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    const auto* entry = m_data[index.row()].get();
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::SERVICE:
            return entry->service();
        case Column::TYPE:
            return entry->typeDescription();
        case Column::STATUS:
            return entry->isBanned() ? tr("POSE_BANNED") : tr("ENABLED");
        case Column::POSE:
            return QString::number(entry->posePenalty());
        case Column::REGISTERED:
            return QString::number(entry->registeredHeight());
        case Column::LAST_PAYMENT:
            return QString::number(entry->lastPaidHeight());
        case Column::NEXT_PAYMENT:
            return entry->nextPaymentHeight() > 0 ? QString::number(entry->nextPaymentHeight()) : tr("UNKNOWN");
        case Column::PAYOUT_ADDRESS:
            return entry->payoutAddress();
        case Column::OPERATOR_REWARD:
            return entry->operatorReward();
        case Column::COLLATERAL_ADDRESS:
            return entry->collateralAddress();
        case Column::OWNER_ADDRESS:
            return entry->ownerAddress();
        case Column::VOTING_ADDRESS:
            return entry->votingAddress();
        case Column::PROTX_HASH:
            return entry->proTxHash();
        default:
            return {};
        }
    } else if (role == Qt::EditRole) {
        // Edit role is used for sorting, so return the raw values where possible
        switch (index.column()) {
        case Column::SERVICE:
            return entry->serviceKey();
        case Column::TYPE:
            return static_cast<int>(entry->type());
        case Column::STATUS:
            return entry->isBanned() ? 1 : 0;
        case Column::POSE:
            return entry->posePenalty();
        case Column::REGISTERED:
            return entry->registeredHeight();
        case Column::LAST_PAYMENT:
            return entry->lastPaidHeight();
        case Column::NEXT_PAYMENT:
            return entry->nextPaymentHeight();
        case Column::PAYOUT_ADDRESS:
            return entry->payoutAddress();
        case Column::OPERATOR_REWARD:
            return entry->operatorRewardPct();
        case Column::COLLATERAL_ADDRESS:
            return entry->collateralAddress();
        case Column::OWNER_ADDRESS:
            return entry->ownerAddress();
        case Column::VOTING_ADDRESS:
            return entry->votingAddress();
        case Column::PROTX_HASH:
            return entry->proTxHash();
        default:
            return {};
        }
    }
    return {};
}

QVariant MasternodeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case Column::SERVICE:
        return tr("Service");
    case Column::TYPE:
        return tr("Type");
    case Column::STATUS:
        return tr("Status");
    case Column::POSE:
        return tr("PoSe Score");
    case Column::REGISTERED:
        return tr("Registered");
    case Column::LAST_PAYMENT:
        return tr("Last Paid");
    case Column::NEXT_PAYMENT:
        return tr("Next Payment");
    case Column::PAYOUT_ADDRESS:
        return tr("Payout Address");
    case Column::OPERATOR_REWARD:
        return tr("Operator Reward");
    case Column::COLLATERAL_ADDRESS:
        return tr("Collateral Address");
    case Column::OWNER_ADDRESS:
        return tr("Owner Address");
    case Column::VOTING_ADDRESS:
        return tr("Voting Address");
    case Column::PROTX_HASH:
        return tr("ProTx Hash");
    default:
        return {};
    }
}

int MasternodeModel::columnWidth(int section)
{
    switch (section) {
    case Column::SERVICE:
        return 200;
    case Column::TYPE:
        return 160;
    case Column::STATUS:
    case Column::POSE:
    case Column::REGISTERED:
    case Column::LAST_PAYMENT:
        return 80;
    case Column::NEXT_PAYMENT:
        return 100;
    case Column::PAYOUT_ADDRESS:
    case Column::OPERATOR_REWARD:
    case Column::COLLATERAL_ADDRESS:
    case Column::OWNER_ADDRESS:
    case Column::VOTING_ADDRESS:
        return 130;
    case Column::PROTX_HASH:
    default:
        return 80;
    }
}

void MasternodeModel::append(std::unique_ptr<MasternodeEntry>&& entry)
{
    beginInsertRows({}, rowCount(), rowCount());
    m_data.push_back(std::move(entry));
    endInsertRows();
}

void MasternodeModel::remove(int row)
{
    if (!isValidRow(row)) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_data.erase(m_data.begin() + row);
    endRemoveRows();
}

void MasternodeModel::reconcile(MasternodeEntryList&& entries)
{
    // Track which existing entries to keep. After processing new entries,
    // remove any existing entries that weren't found in the new set.
    const int original_sz{rowCount()};
    std::vector<bool> keep_index(original_sz, false);

    for (auto& entry : entries) {
        auto it{std::ranges::find_if(m_data, [&entry](const auto& existing) {
            return existing->proTxHash() == entry->proTxHash();
        })};

        if (it != m_data.end()) {
            const auto idx{static_cast<int>(std::distance(m_data.begin(), it))};
            keep_index[static_cast<size_t>(idx)] = true;
            // Check if any data has changed that requires an update
            if ((*it)->isBanned() != entry->isBanned() ||
                (*it)->posePenalty() != entry->posePenalty() ||
                (*it)->lastPaidHeight() != entry->lastPaidHeight() ||
                (*it)->nextPaymentHeight() != entry->nextPaymentHeight()) {
                // Replace entry to update data
                *it = std::move(entry);
                Q_EMIT dataChanged(createIndex(idx, 0), createIndex(idx, Column::_COUNT - 1));
            }
            // else: no changes, entry unique_ptr goes out of scope and gets deleted
        } else {
            append(std::move(entry));
        }
    }

    // Remove in reverse order to preserve indices during removal
    for (int idx{original_sz}; idx-- > 0;) {
        if (!keep_index[static_cast<size_t>(idx)]) {
            remove(idx);
        }
    }
}

const MasternodeEntry* MasternodeModel::getEntryAt(const QModelIndex& index) const
{
    if (!index.isValid() || !isValidRow(index.row())) {
        return nullptr;
    }
    return m_data[index.row()].get();
}
