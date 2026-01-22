// Copyright (c) 2021-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODEMODEL_H
#define BITCOIN_QT_MASTERNODEMODEL_H

#include <evo/dmn_types.h>

#include <QAbstractTableModel>
#include <QByteArray>
#include <QString>

#include <memory>
#include <vector>

class ClientModel;

namespace interfaces {
class MnEntry;
}

///
/// MasternodeEntry wrapper - caches masternode data for display
///
class MasternodeEntry
{
private:
    ClientModel* clientModel;
    std::unique_ptr<const interfaces::MnEntry> m_dmn;

    bool m_banned{false};
    int32_t m_last_paid_height{0};
    int32_t m_next_payment_height{0};
    int32_t m_pose_penalty{0};
    int32_t m_registered_height{0};
    MnType m_type{MnType::Regular};
    QByteArray m_service_key{};
    QString m_collateral_address{};
    QString m_operator_reward{};
    QString m_owner_address{};
    QString m_payout_address{};
    QString m_protx_hash{};
    QString m_service{};
    QString m_type_description{};
    QString m_voting_address{};
    uint16_t m_operator_reward_pct{0};

public:
    explicit MasternodeEntry(ClientModel* _clientModel, std::unique_ptr<const interfaces::MnEntry>&& _dmn,
                             const QString& collateralAddress, int nextPaymentHeight);
    ~MasternodeEntry();

    // Getters for cached data
    bool isBanned() const { return m_banned; }
    int lastPaidHeight() const { return m_last_paid_height; }
    int nextPaymentHeight() const { return m_next_payment_height; }
    int posePenalty() const { return m_pose_penalty; }
    int registeredHeight() const { return m_registered_height; }
    MnType type() const { return m_type; }
    QByteArray serviceKey() const { return m_service_key; }
    QString collateralAddress() const { return m_collateral_address; }
    QString operatorReward() const { return m_operator_reward; }
    QString ownerAddress() const { return m_owner_address; }
    QString payoutAddress() const { return m_payout_address; }
    QString proTxHash() const { return m_protx_hash; }
    QString service() const { return m_service; }
    QString typeDescription() const { return m_type_description; }
    QString votingAddress() const { return m_voting_address; }
    uint16_t operatorRewardPct() const { return m_operator_reward_pct; }

    // Methods that access the underlying MnEntry
    QString toJson() const;
    QString collateralOutpoint() const;
};

using MasternodeEntryList = std::vector<std::unique_ptr<MasternodeEntry>>;

///
/// MasternodeModel - Qt model for masternode list
///
class MasternodeModel : public QAbstractTableModel
{
    Q_OBJECT

private:
    MasternodeEntryList m_data;

    bool isValidRow(int row) const { return row >= 0 && row < static_cast<int>(m_data.size()); }

public:
    enum Column : int {
        SERVICE = 0,
        TYPE,
        STATUS,
        POSE,
        REGISTERED,
        LAST_PAYMENT,
        NEXT_PAYMENT,
        PAYOUT_ADDRESS,
        OPERATOR_REWARD,
        COLLATERAL_ADDRESS,
        OWNER_ADDRESS,
        VOTING_ADDRESS,
        PROTX_HASH,
        _COUNT
    };

    explicit MasternodeModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}

    int rowCount(const QModelIndex& index = {}) const override;
    int columnCount(const QModelIndex& index = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    static int columnWidth(int section);

    void append(std::unique_ptr<MasternodeEntry>&& entry);
    void remove(int row);
    void reconcile(MasternodeEntryList&& entries);
    const MasternodeEntry* getEntryAt(const QModelIndex& index) const;
};

#endif // BITCOIN_QT_MASTERNODEMODEL_H
