// Copyright (c) 2016-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_MASTERNODELIST_H
#define BITCOIN_QT_MASTERNODELIST_H

#include <util/system.h>

#include <QMenu>
#include <QSortFilterProxyModel>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <set>

#define MASTERNODELIST_UPDATE_SECONDS 3
#define MASTERNODELIST_FILTER_COOLDOWN_SECONDS 3

namespace Ui
{
class MasternodeList;
}

class ClientModel;
class MasternodeEntry;
class MasternodeModel;
class WalletModel;

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

class MasternodeListSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum TypeFilter { All = 0, Regular = 1, Evo = 2 };

    explicit MasternodeListSortFilterProxyModel(QObject* parent = nullptr) :
        QSortFilterProxyModel(parent) {}

    void setTypeFilter(TypeFilter type) { m_type_filter = type; }
    void setShowOwnedOnly(bool show) { m_show_owned_only = show; }
    void setHideBanned(bool hide) { m_hide_banned = hide; }
    void setMyMasternodeHashes(const std::set<QString>& hashes) { m_my_mn_hashes = hashes; }
    void forceInvalidateFilter() { invalidateFilter(); }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    TypeFilter m_type_filter{All};
    bool m_show_owned_only{false};
    bool m_hide_banned{true};
    std::set<QString> m_my_mn_hashes;
};

/** Masternode Manager page widget */
class MasternodeList : public QWidget
{
    Q_OBJECT

public:
    explicit MasternodeList(QWidget* parent = nullptr);
    ~MasternodeList();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

private:
    QMenu* contextMenuDIP3;
    int64_t nTimeFilterUpdatedDIP3{0};
    int64_t nTimeUpdatedDIP3{0};
    bool fFilterUpdatedDIP3{true};

    QTimer* timer;
    Ui::MasternodeList* ui;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    MasternodeModel* m_model{nullptr};
    MasternodeListSortFilterProxyModel* m_proxy_model{nullptr};

    bool mnListChanged{true};

    const MasternodeEntry* GetSelectedEntry();

    void updateDIP3List();
    void updateMyMasternodeHashes();

Q_SIGNALS:
    void doubleClicked(const QModelIndex&);

private Q_SLOTS:
    void showContextMenuDIP3(const QPoint&);
    void on_filterLineEditDIP3_textChanged(const QString& strFilterIn);
    void on_comboBoxType_currentIndexChanged(int index);
    void on_checkBoxOwned_stateChanged(int state);
    void on_checkBoxHideBanned_stateChanged(int state);

    void extraInfoDIP3_clicked();
    void copyProTxHash_clicked();
    void copyCollateralOutpoint_clicked();
    void filterByCollateralAddress();
    void filterByPayoutAddress();
    void filterByOwnerAddress();
    void filterByVotingAddress();

    void handleMasternodeListChanged();
    void updateDIP3ListScheduled();
    void updateFilteredCount();
};
#endif // BITCOIN_QT_MASTERNODELIST_H
